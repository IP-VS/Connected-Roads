import ws from 'ws';
import { SerialPort } from 'serialport'
import { MeshNode, NodeList } from '../models/node.model';
import dotenv from 'dotenv'

dotenv.config();
var serialport: SerialPort;
var connected = false;

// Serial functions
function initSerial(wsServer: ws.Server) {
    let isOpening = false;
    /* Connect serial port */
    serialport = new SerialPort({
        path: process.env.SERIAL_PORT ?? "/dev/ttyUSB0",
        baudRate: parseInt(process.env.BAUD_RATE ?? "115200") ?? 115200,
        // flowControl RTS/CTS
        rtscts: true,
        dataBits: 8,
        stopBits: 1,
        parity: 'none'
    });

    serialport.on('error', (err) => {
        // console.log("Could not connect.");
        connectSerial();
    });
    // Reconnect on disconnect
    serialport.on('close', () => {
        console.log('Serial port closed');
        // Try reconnecting 
        connectSerial();
        // Send to socket
        wsServer.clients.forEach(client => {
            client.send('device:Disconnected');
        });
        connected = false;
        NodeList.clear();
    });

    // On open
    serialport.on('open', () => {
        console.log('Connected to serial port');
        wsServer.clients.forEach(client => {
            client.send('device:Connected');
        });
        connected = true;
    });

    /* Open serial */
    const connectSerial: any = () => {
        if (isOpening) {
            return;
        }
        var interval = setInterval(() => {
            if (serialport.isOpen) {
                isOpening = false;
                clearInterval(interval);
            } else {
                isOpening = true;
                serialport.open();
            }
        }, 1000);
    }
    connectSerial();

    // connect to tcp socket for analysis
    var net = require('net');
    var detectorClient = new net.Socket();
    detectorClient.connect(1234, '127.0.0.1', function () {
        console.log('Connected to detection server');
    });
    detectorClient.on('data', function (data: any) {
        data = data.toString('utf8');
        console.log('Received: ' + data);
        // Send data to WS clients
        wsServer.clients.forEach(client => {
            client.send('micdata_detector:' + data);
        });
    });

    // received data from serial (UART)
    serialport.on('data', function (data) {
        console.log('Data:', data.toString('utf8'));
        var dataStr = data.toString('utf8').trim();
        // Node added
        if (dataStr.indexOf('got heartbeat message') > -1) {
            try {
                // Set device status
                wsServer.clients.forEach(client => {
                    client.send('device:Connected');
                });
                // Get Node name
                var nodeName = dataStr.split('got heartbeat message: ')[1];

                // Remove other chars
                nodeName = nodeName.replace(/[^0-9a-z]/g, '');
                // e.g. a6b3987620c88052
                if (nodeName.length != 16) {
                    return;
                }
                let node = NodeList.getNode(nodeName);
                if (node) {
                    node.status = '🟢';
                    try {
                        clearTimeout(node.timer);
                    } catch (e) {
                        // Do nothing
                    }
                } else {
                    // Create new node
                    node = new MeshNode(
                        nodeName,
                        nodeName,
                        '🟢'
                    );
                }
                // Start timeout for node status
                node.timer = setTimeout(() => {
                    node!.status = '🔴';
                    // Send nodelist to client
                    wsServer.clients.forEach((client: any) => {
                        client.send(NodeList.toString());
                    });
                }, 10000);

                NodeList.addNode(node);
                // Send nodeID to the client
                wsServer.clients.forEach(client => {
                    client.send(NodeList.toString());
                });
            } catch (e) {
                console.log(e);
            }
        }
        // Press button for being a provisioner
        else if (dataStr.indexOf('Press Button') > -1) {
            wsServer.clients.forEach(client => {
                client.send('device:Press Button');
            });
        }
        // New device found
        else if (dataStr.indexOf('detected') > -1) {
            // Send nodeID to the client
            wsServer.clients.forEach(client => {
                client.send('device:Press Button');
            });
        }
        // Microphone data
        else if (dataStr.indexOf('micdata') > -1) {
            dataStr = dataStr.replace(/[^0-9,]/g, '');

            // send data
            var buffer = Buffer.alloc(128);
            // send data to tcp socket
            var tmpData0 = parseInt(dataStr.split(',')[0].replace(/[^0-9]/g, ''));
            buffer.writeInt32BE(tmpData0, 0);
            var tmpData1 = parseInt(dataStr.split(',')[1].replace(/[^0-9]/g, ''));
            buffer.writeInt32BE(tmpData1, 0);
            var tmpData2 = BigInt(dataStr.split(',')[2].replace(/[^0-9]/g, ''));
            buffer.writeBigInt64BE(tmpData2, 0);
            detectorClient.write(buffer);

            wsServer.clients.forEach(client => {
                client.send('micdata_raw:' + dataStr);
            });
        }
    });

    // function testNode(num: number) {
    //     // Get Node name
    //     var nodeName = 'Node 0x000'+num;
    //     // Remove non ascii numbers
    //     nodeName = nodeName.replace(/[^0-9]/g, '');
    //     var newNode: Node = {
    //         id: parseInt(nodeName),
    //         name: nodeName,
    //         status: '🟢'
    //     }
    //     nodeList.push(newNode);
    //     // Send nodelist to client
    //     wsServer.clients.forEach(client => {
    //         client.send(JSON.stringify(nodeList));
    //     });
    // }
    // setTimeout(() => testNode(1), 3000);
    // setTimeout(() => testNode(2), 4000);
    // setTimeout(() => testNode(3), 5000);
}

export {
    initSerial,
    serialport,
    connected
};