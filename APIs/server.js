const express = require("express");
const path = require("path");
const net = require("net");

const app = express();
app.use(express.json());

// TCP connection to C++ app running on localhost:8080
const C_PLUS_PLUS_HOST = 'smart_mailer';
const C_PLUS_PLUS_PORT = 8081;

// Endpoint to send email command
app.post("/send", (req, res) => {
    const { command = "send_email" } = req.body;

    // Create a TCP client to send the command to the C++ app
    const client = new net.Socket();

    // Connect to the C++ server on port 8080
    client.connect(C_PLUS_PLUS_PORT, C_PLUS_PLUS_HOST, () => {
        console.log("Connected to C++ app, sending command...");
        client.write(command + "\n"); // Send command (e.g., "send_email")
    });

    // Handle response from the C++ server
    client.on('data', (data) => {
        console.log('Received from C++ app:', data.toString());
        res.send({
            message: 'Command executed successfully.',
            output: data.toString() // Send the output received from C++ app
        });

        // Close the connection after receiving the response
        client.destroy();
    });

    // Handle error in TCP connection
    client.on('error', (err) => {
        console.error('TCP Connection Error:', err.message);
        res.status(500).send({ error: 'Failed to execute command.' });
        client.destroy();
    });

    // Handle close event
    client.on('close', () => {
        console.log('Connection closed.');
    });
});

// Start server
const PORT = 3000;
app.listen(PORT, () => {
    console.log(`Server is running on http://localhost:${PORT}`);
});
