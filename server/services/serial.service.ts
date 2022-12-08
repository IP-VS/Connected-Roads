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
        baudRate: parseInt(process.env.BAUD_RATE ?? "115200") ?? 115200
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

    serialport.on('data', function (data) {
        console.log('Data:', data.toString('utf8'));
        var dataStr = data.toString('utf8');
        // Node added
        if (dataStr.indexOf('Added node 0x0') > -1) {
            try {
                // Set device status
                wsServer.clients.forEach(client => {
                    client.send('device:Connected');
                });
                // Get Node name
                var nodeName = dataStr.split('Added node 0x0')[1];
                var temp = nodeName.split('0x0')[1];
                if (temp != undefined) {
                    nodeName = temp;
                }
                // Remove non ascii numbers
                nodeName = nodeName.replace(/[^0-9]/g, '');
                // Create new node
                var newNode = new MeshNode(
                    parseInt(nodeName),
                    nodeName,
                    '🟢'
                );
                NodeList.addNode(newNode);
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