import express from 'express';
var router = express.Router();
import ws from 'ws';
import dotenv from 'dotenv'
import initSerial from '../services/serial.service';

dotenv.config();

/* Websocket server */
const wsServer = new ws.Server({ port: 8080 });
initSerial(wsServer);
wsServer.on('connection', socket => {
  socket.on('message', (message) => {
    console.log(message);
    // Reply
    socket.send('Hello back!');
  });
});

/* GET home page. */
router.get('/', function (req, res, next) {
  res.render('index', { title: 'Vernetzte Strassen' });
});

export default router;
