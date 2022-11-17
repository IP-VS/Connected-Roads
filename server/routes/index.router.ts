import express from 'express';
var router = express.Router();
import ws from 'ws';
import dotenv from 'dotenv'
import {initSerial, serialport, nodeList} from '../services/serial.service';

dotenv.config();

/* Websocket server */
const wsServer = new ws.Server({ port: 8080 });
initSerial(wsServer);
wsServer.on('connection', socket => {
  socket.on('message', (message) => {
    console.log(message);
    // Reply
    if (message.toString('utf8') == 'INIT') {
      socket.send('Connected to websocket server');
    } else if (message.toString('utf8').indexOf('rmNode') > -1) {
      var nodeId = parseInt(message.toString('utf8').split('rmNode:')[1]);
      if (nodeList.indexOf(nodeId) > -1) {
        serialport.write(nodeId + '\r\n');
        delete nodeList[nodeList.indexOf(nodeId)];
        socket.send('Removed node ' + nodeId);
      }
    }
  });
});

/* GET home page. */
router.get('/', function (req, res, next) {
  res.render('index', { title: 'Vernetzte Strassen' });
});

export default router;
