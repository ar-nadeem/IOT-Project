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

// Route to display the stored data
app.get('/', (req, res) => {
  const html = `
    <!DOCTYPE html>
    <html lang="en">
    <head>
      <meta charset="UTF-8">
      <meta name="viewport" content="width=device-width, initial-scale=1.0">
      <script src="https://cdn.tailwindcss.com"></script>
      <style>
      /* Custom styles */
      body {
          background-color: #000;
          color: #fff;
      }
    </style>
    </head>

    <body class="p-8">
      <div class="max-w-lg mx-auto">
          <h1 class="text-3xl font-bold mb-6">Data Display</h1>
          <div class="bg-gray-900 p-6 rounded-lg mb-4">
              <h2 class="text-xl font-bold mb-4">Heart Rate</h2>
              <p id="heartRate" class="text-3xl font-bold text-rose-400">${storedData.length > 0 ? storedData[storedData.length - 1].heartJ.heart_avg + " BPM" : ""}</p>
          </div>
          <div class="bg-gray-900 p-6 rounded-lg mb-4">
              <h2 class="text-xl font-bold mb-4">Acceleration</h2>
              <p id="acceleration" class="text-3xl font-bold text-slate-400">${storedData.length > 0 ? storedData[storedData.length - 1].acc.z + " m/s^2" : ""}</p>
          </div>
          <div class="bg-gray-900 p-6 rounded-lg">
              <h2 class="text-xl font-bold mb-4">Temperature</h2>
              <p id="temperature" class="text-3xl font-bold text-slate-400">${storedData.length > 0 ? storedData[storedData.length - 1].acc.temp + " C" : ""}</p>
          </div>
          <div class="bg-gray-900 p-6 rounded-lg">
              <h2 class="text-xl font-bold mb-4">Falls</h2>
              <p id="falls" class="text-3xl font-bold text-rose-800">${falls > 0 ? "Detected" : "NULL"}</p>
          </div>
        </div>
    </body>

    <script>
      // Function to fetch and update data
      function fetchData() {
        fetch('/')
          .then(response => response.text())
          .then(data => {
            // Extract data from the HTML response
            const parser = new DOMParser();
            const htmlDoc = parser.parseFromString(data, 'text/html');
            const heartRate = htmlDoc.getElementById('heartRate').textContent;
            const acceleration = htmlDoc.getElementById('acceleration').textContent;
            const temperature = htmlDoc.getElementById('temperature').textContent;
            const falls = htmlDoc.getElementById('falls').textContent;

            // Update the page content
            document.getElementById('heartRate').textContent = heartRate;
            document.getElementById('acceleration').textContent = acceleration;
            document.getElementById('temperature').textContent = temperature;
            document.getElementById('falls').textContent = falls;
          })
          .catch(error => console.error('Error fetching data:', error));
      }

      // Fetch data initially
      fetchData();

      // Fetch data periodically every 5 seconds
      setInterval(fetchData, 5);
    </script>

    </html>
  `;

  res.send(html);
});

// Start the server
app.listen(port, () => {
  console.log(`Server is running on http://localhost:${port}`);
});
