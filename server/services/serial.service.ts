import ws from 'ws';
import { SerialPort } from 'serialport'
import dotenv from 'dotenv'

dotenv.config();
var serialport: SerialPort;
var nodeList: any = [];

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
    });

    // On open
    serialport.on('open', () => {
        console.log('Connected to serial port');
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
            // Get Node name
            var nodeName = dataStr.split('Added node 0x0')[1].split('0x0')[1];
            // Remove non ascii numbers
            nodeName = nodeName.replace(/[^0-9]/g, '');
            var nodeId = parseInt(nodeName);
            nodeList.push(nodeId);
            // Send nodeID to the client
            wsServer.clients.forEach(client => {
                client.send("Node " + nodeId + " Status: 🟢");
            });
        }
    });
    function testNode() {
        // Get Node name
        var nodeName = 'Node 0x0002';
        // Remove non ascii numbers
        nodeName = nodeName.replace(/[^0-9]/g, '');
        var nodeId = parseInt(nodeName);
        nodeList.push(nodeId);
        // Send nodeID to the client
        wsServer.clients.forEach(client => {
            client.send("Node " + nodeId + " Status: 🟢");
        });
    }
    setTimeout(testNode, 2000);
}

export {
    initSerial,
    serialport,
    nodeList
};