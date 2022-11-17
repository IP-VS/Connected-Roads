// Create WebSocket connection.
var wsUrl = 'ws://localhost:8080';
var socket = new WebSocket(wsUrl);

var isOpening = false;
var connectWS = function () {
    if (isOpening) {
        return;
    }
    var interval = setInterval(() => {
        if (socket.OPEN) {
            isOpening = false;
            clearInterval(interval);
        } else {
            isOpening = true;
            // Create WebSocket connection.
            socket = new WebSocket(wsUrl);
        }
    }, 1000);
}

var rmNode = function (node) {
    socket.send('rmNode:' + node);
}


// Connection opened
socket.addEventListener('open', (event) => {
    socket.send('INIT');
});

// On connection close
socket.addEventListener('close', (event) => {
    console.log('Connection closed');
    // Try to reconnect
    connectWS();
});


// Listen for messages
socket.addEventListener('message', (event) => {
    console.log('Message from server ', event.data);
    var msgElement = document.createElement("p");
    msgElement.id = event.data.split(':')[0];
    msgElement.innerHTML = event.data;
    document.getElementById("output").appendChild(msgElement);
});
