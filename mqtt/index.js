const express = require('express');
const bodyParser = require('body-parser');
const cors = require('cors');
const mqtt = require('mqtt');
const brokerUrl = 'mqtt://localhost';

const app = express();
const port = 3000;
app.use(cors());

// Middleware to parse JSON data
app.use(bodyParser.json());
let storedData = [];
let falls = 0;

function sendMQTT(topic,message){
  const client = mqtt.connect(brokerUrl);


  client.on('connect', () => {
    console.log('Connected to MQTT broker');

    // Publish the message to the topic
    client.publish(topic, message, (error) => {
        if (error) {
            console.error('Error publishing message:', error);
        } else {
            console.log(`Published message "${message}" to topic "${topic}"`);
            // Disconnect from MQTT broker after publishing
            client.end();
        }
    });
  });
}

// Route to handle POST requests and store the data
app.post('/data', (req, res) => {
  const { heartJ, acc } = req.body;

  // Store the received data
  storedData.push({ heartJ, acc });
  // Handle connection events
  sendMQTT('data/heart', JSON.stringify({heartJ}));
  sendMQTT('data/acc', JSON.stringify({acc}));


  // Check for falls in the last 10 data entries
  falls = 0;
  for (let i = Math.max(storedData.length - 10, 0); i < storedData.length; i++) {
    const accZ = storedData[i].acc.z;
    if (accZ > 12) {
      falls++;
    }
  }

  res.status(200).send('Data received and stored successfully!');
});



// Start the server
app.listen(port, () => {
  console.log(`Server is running on http://localhost:${port}`);
});
