import ws from 'ws';
import { SerialPort } from 'serialport'
import dotenv from 'dotenv'

dotenv.config();

// Serial functions
function initSerial(wsServer: ws.Server) {
    let isOpening = false;
    /* Connect serial port */
    const serialport = new SerialPort({
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
        console.log('Data:', data)
        // Send data to the client
        wsServer.clients.forEach(client => {
            client.send(data.toString('utf8'));
        });
    });
}

export default initSerial;