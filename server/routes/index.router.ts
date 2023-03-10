import express from 'express';
var router = express.Router();
import ws from 'ws';
import dotenv from 'dotenv'
import { initSerial, serialport, connected } from '../services/serial.service';
import { MeshNode, NodeList } from '../models/node.model';

dotenv.config();

/* Websocket server */
const wsServer = new ws.Server({ port: 5327 });
initSerial(wsServer);
// On listen
wsServer.on('listening', () => {
  console.log('Websocket server listening on port 5327');
});
wsServer.on('connection', socket => {
  socket.on('message', (message) => {
    console.log(message);
    // Reply
    if (message.toString('utf8') == 'INIT') {
      socket.send('Connected to websocket server');
      // Send node list
      socket.send(NodeList.toString());
      // Device status
      if (connected) {
        socket.send('device:Connected');
        serialport.write('ADV\n');
      } else {
        socket.send('device:Disconnected');
      }
    } else if (message.toString('utf8').indexOf('rmNode') > -1) {
      var nodeId = message.toString('utf8').split('rmNode:')[1];
      if (NodeList.removeNode(nodeId)) {
        serialport.write('REM' + nodeId + '\r\n');
        socket.send(NodeList.toString());
        socket.send('node:Removed');
      }
    } else if (message.toString('utf8').indexOf('SND') > -1) {
      var msg = message.toString('utf8').split('SND')[1];
      serialport.write('SND' + msg);
    } else if (message.toString('utf8').indexOf('UPT') > -1) {
      serialport.write('UPT\r\n');
    }
  });
});

/* GET home page. */
router.get('/', function (req, res, next) {
  res.render('index', { title: 'Vernetzte Strassen' });
});

export default router;
