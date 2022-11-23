// Create WebSocket connection.
var wsUrl = 'ws://localhost:8080';

var socket;
var connected = false;
var connectWS = function () {
    var interval = setInterval(() => {
        console.log("Connecting...");
        if (connected) {
            console.log("Connected.");
            clearInterval(interval);
        } else {
            // Create WebSocket connection.
            socket = new WebSocket(wsUrl);
            socketListener();
        }
    }, 1000);
}
connectWS();

var rmNode = function (node) {
    socket.send('rmNode:' + node);
}

var socketListener = function () {
    // Connection opened
    socket.addEventListener('open', (event) => {
        console.log('Connection opened');
        connected = true;
        socket.send('INIT');
    });

    // On connection close
    socket.addEventListener('close', (event) => {
        console.log('Connection closed');
        connected = false;
        socket = null;
        // Try to reconnect
        connectWS();
    });

    // Listen for messages
    socket.addEventListener('message', (event) => {
        var msg = event.data;
        console.log('Message from server ', msg);
        if (msg.indexOf('Status: ') > -1) {
            // Received a node status
            const nodeName = msg.split(' Status')[0].replace(/ /g, '_'); // Node_1
            const nodeId = parseInt(msg.replace(/[^0-9]/g, ''));
            const nodeStatus = msg.split('Status:')[1].replace(/[^🟢]/g, '');
            var buttonElement = document.createElement('button');
            buttonElement.onclick = rmNode(nodeId);
            buttonElement.innerText = "Remove";
            // Create table row
            var rowElement = document.createElement('tr');
            rowElement.id = nodeName;
            rowElement.innerHTML = `
            <td>${nodeName}</td>
            <td>${nodeStatus}</td>
            <td>${buttonElement.outerHTML}</td>
        `
            if (document.getElementById(nodeName) == null) {
                // Not in the table yet
                document.getElementById('nodes').appendChild(rowElement);
            } else {
                // Already in the table so remove first
                document.getElementById(nodeName).remove();
                document.getElementById('nodes').appendChild(rowElement);
            }
        }

        // Just append log to output
        var msgElement = document.createElement("p");
        msgElement.id = msg.split(':')[0];
        msgElement.innerHTML = msg;
        document.getElementById("output").appendChild(msgElement);
    });
}
