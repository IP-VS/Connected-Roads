import numpy as np
import pandas as pd
import filterpy.kalman, filterpy.common
import plotly.express as px
import plotly.graph_objects as go
import collections
import math
import datetime

import socket
import struct
import io

import argparse


class BackgroundFilter:
    def __init__(self, avg_factor=0.98, delay=100, detection_margin=1e-4, initial_background_level=2e-4):
        self.avg_factor = avg_factor
        self.detection_margin = detection_margin
        self.background_level = initial_background_level
        self.intensities = collections.deque([], delay)
        
    def next(self, intensity):
        if intensity <= self.background_level + self.detection_margin:
            self.intensities.append(intensity)
            self.background_level = self.avg_factor*self.background_level + (1-self.avg_factor)*self.intensities[0]
            return False
        else:
            return True


class Signals:
    def __init__(self, average_window=50):
        self.x = collections.deque([],average_window)
        self.y = collections.deque([],average_window)
        
    def next(self, x, y):
        self.x.append(x)
        self.y.append(y)
        
        if len(self.x) == self.x.maxlen:
            return (np.mean(self.x), np.mean(self.y))


class EventDetector:
    def __init__(self, signals=Signals(), min_width=151):
        self.signals = signals
        self.min_width = min_width
        self.background_filter = BackgroundFilter()
        self.event_active = False
    
    def next(self, x, y):
        intensities = self.signals.next(x,y)
        if not intensities:
            return None
        
        x,y = intensities
        total = math.sqrt(x**2+y**2)
        
        if self.background_filter.next(total):
            if not self.event_active:
                self.x = []
                self.y = []
                self.total = []
                self.event_active = True
                
            self.x.append(x)
            self.y.append(y)
            self.total.append(total)
                
        elif self.event_active:
            self.event_active = False
            return self.x, self.y, self.total, self.split_event()
        
    def split_event(self):
        peaks = []
        valleys = [0]
        events = []
        
        for i in range(0, len(self.total)-self.min_width+1):
            if np.argmax(self.total[i:i+self.min_width]) == self.min_width//2:
                peaks.append(i+self.min_width//2)
                
        for i in range(0, len(peaks)-1):
            valleys.append(np.argmin(self.total[peaks[i]:peaks[i+1]+1])+peaks[i])
        valleys.append(len(self.total)-1)
            
        for (i, peak) in enumerate(peaks):
            events.append((valleys[i], peak, valleys[i+1]))
            
        return events


def position_velocity(xs,ys):
    f = filterpy.kalman.KalmanFilter(dim_x=2, dim_z=1)
    f.x = np.array([xs[0]/ys[0], 0]) # initial position/velocity
    f.F = np.array([[1,1],[0,1]]) # state matrix
    f.H = np.array([[1,0]]) # measuring function
    f.P *= 0.028 # covariance matrix
    f.R = 69.4 # measuring noise variance
    f.Q = filterpy.common.Q_discrete_white_noise(dim=2, var=1.1e-5)
    
    pos = []
    vel = []
    
    for x,y in zip(xs,ys):
        f.predict()
        f.update(x/y)
        pos.append(f.x[0])
        vel.append(f.x[1])
        
    return (pos, vel)


def subevents(event):
    # event: ((idx_min, idx_max), xs, ys, totals, [(left, peak, right)])
    
    evs = []
    
    (idx_min, idx_max), xs, ys, total, subevents = event
    for left, peak, right in subevents:
        evs.append(((idx_min + left, idx_min + right), total[left:right+1], xs[left:right+1], ys[left:right+1], peak-left))
            
    return evs


def features(event, v_max=0.04):
    # event: ((idx_min, idx_max), intensities, positions, velocities, idx_peak_rel) 
    idx, intensities, positions, velocities, peak = event
    pos = [p for p,v in zip(positions, velocities) if v <= v_max]
    return (idx, (min(pos), max(pos)), max(intensities), velocities[peak])


def decision(events, n_thr=100, i_thr=5e-4, s_thr=0.2):
    # events: [((idx_min, idx_max), (pos_min, pos_max), intensity_max, velocity)]
    
    merged_events = [events[0]]
    
    for event in events[1:]:
        (pre_idx_min, pre_idx_max), (pre_pos_min, pre_pos_max), pre_intensity_max, pre_vel = merged_events[-1]
        (idx_min, idx_max), (pos_min, pos_max), intensity_max, vel = event
        
        if pre_pos_min < pos_max and pre_pos_max > pos_min:
            merged_events[-1] = ((pre_idx_min, idx_max), (min(pre_pos_min, pos_min), max(pre_pos_max, pos_max)), max(pre_intensity_max, intensity_max), pre_vel)
            
        else:
            merged_events.append(event)
            
    final_events = []
    for event in merged_events:
        (idx_min, idx_max), (pos_min, pos_max), intensity_max, vel = event
        if idx_max - idx_min <= n_thr:
            continue
            
        if intensity_max <= i_thr:
            continue
            
        if pos_max - pos_min <= s_thr:
            continue
            
        final_events.append((idx_min, idx_max, vel))
        
    return final_events



argparser = argparse.ArgumentParser()
argparser.add_argument('--addr', type=str, required=False, default='0.0.0.0')
argparser.add_argument('--port', type=int, required=True)

args = argparser.parse_args()


# big endian
# int32 left channel
# int32 right channel
# int64 time
MSG_FORMAT = '>iiiq'

unpacker = struct.Struct(MSG_FORMAT)
msg_buffer = io.BytesIO()

ed = EventDetector()
i = 0

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.bind((args.addr, args.port))
sock.listen()

# socket clients
clients = []

conn, addr = sock.accept()
print('Connected by', addr)
clients.append(conn)

# max of 20 nodes from client
values =  [None] * 20
event_group = [None] * 100000

# TODO error handling
# TODO handle re-connects
while True:

    bytes_read = 0
    bytes_expected = unpacker.size - bytes_read

    if bytes_expected > 0:
        data = conn.recv(bytes_expected)
        msg_buffer.write(data)
        bytes_read += len(data)

    if bytes_read >= unpacker.size:
        msg_buffer.seek(0)
        unpacked_data = unpacker.unpack(msg_buffer.read(unpacker.size))

        # print msg buffer
        #print(unpacked_data)

        msg_buffer.seek(0)
        bytes_read = 0

        
        nodeid, left, right, time = unpacked_data
    
        left = left / 1000000
        right = right / 1000000

        values[nodeid] = ed.next(left, right)
        i += 1
        if values[nodeid]:
            event_group[nodeid] = [event for event in subevents(((i-1-len(values[nodeid][0]), i-1), values[nodeid][0], values[nodeid][1], values[nodeid][2], values[nodeid][3]))]
    
            feats = []
            for event in event_group[nodeid]:
                pos, vel = position_velocity(event[2], event[3])
                feats.append(features((event[0], event[1], pos, vel, event[4])))
                
            if len(feats) < 1:
                continue
            
            for event in decision(feats):
                print(event[0]) # index of first sample
                print(event[1]) # index of last sample
                print(event[2]) # velocity
                print()
                # send velocity to server
                for client in clients:
                    client.send(bytes(f"{nodeid},{event[2]}", encoding='utf-8'))
                
