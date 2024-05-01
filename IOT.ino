#include <Wire.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"
#include <WiFi.h>
#include <Arduino_JSON.h>
#include <assert.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

MAX30105 particleSensor;

#define MAX_BRIGHTNESS 255
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////// SPO 2 Sensor Variables ///////////////////////////////////////////////////
#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
//Arduino Uno doesn't have enough SRAM to store 100 samples of IR led data and red led data in 32-bit format
//To solve this problem, 16-bit MSB of the sampled data will be truncated. Samples become 16-bit data.
uint16_t irBuffer[100]; //infrared LED sensor data
uint16_t redBuffer[100];  //red LED sensor data
#else
uint32_t irBuffer[100]; //infrared LED sensor data
uint32_t redBuffer[100];  //red LED sensor data
#endif

int32_t bufferLength; //data length
int32_t spo2; //SPO2 value
int8_t validSPO2; //indicator to show if the SPO2 calculation is valid
int32_t heartRate; //heart rate value
int8_t validHeartRate; //indicator to show if the heart rate calculation is valid
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////     Wifi Server        ///////////////////////////////////////////////////
const char* ssid     = "The Gateway";
const char* password = "HokieHouse";
WiFiServer server(80);
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Adafruit_MPU6050 mpu;

byte redLED=13;
byte yellowLED=12;
byte greenLED=14;
byte blueLED=27;
JSONVar heartJson;

void setup()
{
  heartJson["sp"] = -1;
  heartJson["sp_valid"] = -1;
  heartJson["heart"] = -1;
  heartJson["heart_valid"] = -1;
  
  pinMode(redLED, OUTPUT);
  pinMode(yellowLED, OUTPUT);
  pinMode(greenLED, OUTPUT);
  pinMode(blueLED, OUTPUT);

  digitalWrite(redLED,HIGH);

  Serial.begin(115200); // initialize serial communication at 115200 bits per second:

  // Connect to wifi and start the server
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected.");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    
    server.begin();


  // Initialize sensor
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) //Use default I2C port, 400kHz speed
  {
    Serial.println(F("MAX30105 was not found. Please check wiring/power."));
    // Error forever loop
      while (1){  
        digitalWrite(redLED,HIGH);
        delay(1000);
        digitalWrite(redLED,LOW);
        delay(1000);
    }
  }

  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
      while (1){
        digitalWrite(redLED,HIGH);
        delay(1000);
        digitalWrite(redLED,LOW);
        delay(1000);
    }
  }
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_5_HZ);
  // Serial.println(F("Attach sensor to finger with rubber band. Press any key to start conversion"));
  // while (Serial.available() == 0) ; //wait until user presses a key
  // Serial.read();
  digitalWrite(redLED,LOW);

  // Wait for 3 seconds before starting reading
  for (int i = 0; i <=3; i++) {
    digitalWrite(yellowLED,HIGH);
    delay(900);
    digitalWrite(yellowLED,LOW);
    delay(100);
  }


  byte ledBrightness = 60; //Options: 0=Off to 255=50mA  // Default 60
  byte sampleAverage = 4; //Options: 1, 2, 4, 8, 16, 32
  byte ledMode = 2; //Options: 1 = Red only, 2 = Red + IR, 3 = Red + IR + Green
  byte sampleRate = 100; //Options: 50, 100, 200, 400, 800, 1000, 1600, 3200
  int pulseWidth = 411; //Options: 69, 118, 215, 411
  int adcRange = 4096; //Options: 2048, 4096, 8192, 16384

  particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange); //Configure sensor with these settings
}
void api(WiFiClient client, JSONVar myObject){
  if (client) {                             // if you get a client,
    Serial.println("New Client.");           // print a message out the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        if (c == '\n') {                    // if the byte is a newline character

          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();

            // the content of the HTTP response follows the header:
            client.print("<h1> ");
            client.print("Heart Rate");
            client.print("</h1> ");
            client.print("<a> ");
            client.print(myObject["heart"]);
            client.print("</a> ");


            // The HTTP response ends with another blank line:
            client.println();
            // break out of the while loop:
            break;
          } else {    // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }

        // Check to see if the client request was "GET /H" or "GET /L":
        if (currentLine.endsWith("GET /H")) {
          // digitalWrite(13, HIGH);               // GET /H turns the LED on
        }
        if (currentLine.endsWith("GET /L")) {
          // digitalWrite(13, LOW);                // GET /L turns the LED off
        }
      }
    }
    // close the connection:
    client.stop();
    Serial.println("Client Disconnected.");
  }
}
JSONVar getHeart(WiFiClient client){
  bufferLength = 100; //buffer length of 100 stores 4 seconds of samples running at 25sps
  for (byte i = 0 ; i < bufferLength ; i++)
    {
      digitalWrite(blueLED, !digitalRead(blueLED));


      while (particleSensor.available() == false) //do we have new data?
        particleSensor.check(); //Check the sensor for new data

      digitalWrite(blueLED, !digitalRead(blueLED));
      redBuffer[i] = particleSensor.getRed();
      irBuffer[i] = particleSensor.getIR();
      particleSensor.nextSample(); //We're finished with this sample so move to next sample

      // Serial.print(F("red="));
      // Serial.print(redBuffer[i], DEC);
      // Serial.print(F(", ir="));
      // Serial.println(irBuffer[i], DEC);
      api(client,heartJson);
      digitalWrite(blueLED,LOW);
    }

    //calculate heart rate and SpO2 after first 100 samples (first 4 seconds of samples)
    maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);
    Serial.print(F("SPO2 = "));
    Serial.print(spo2,DEC);
    Serial.print(F(" - "));
    Serial.println(validSPO2,DEC);

    Serial.print(F("HR = "));
    Serial.print(heartRate,DEC);
    Serial.print(F(" - "));
    Serial.println(validHeartRate,DEC);
    JSONVar myObject;
    myObject["sp"] = spo2;
    myObject["sp_valid"] = validSPO2;
    myObject["heart"] = heartRate;
    myObject["heart_valid"] = validHeartRate;
    return myObject;
}


void loop()
{
  WiFiClient client = server.available();

  heartJson = getHeart(client);
  digitalWrite(greenLED,HIGH);
  Serial.println(heartJson["sp"]);
  Serial.println(heartJson["heart"]);
  
/////////////////////////////////////////// SERVER ///////////////////////////////////////////
  digitalWrite(greenLED,HIGH);
  api(client,heartJson);
  digitalWrite(greenLED,LOW);
////////////////////////////////////////// Server /////////////////////////////////////////////
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  Serial.print("Acceleration X: ");
  Serial.print(a.acceleration.x);
  Serial.print(", Y: ");
  Serial.print(a.acceleration.y);
  Serial.print(", Z: ");
  Serial.print(a.acceleration.z);
  Serial.println(" m/s^2");

  Serial.print("Rotation X: ");
  Serial.print(g.gyro.x);
  Serial.print(", Y: ");
  Serial.print(g.gyro.y);
  Serial.print(", Z: ");
  Serial.print(g.gyro.z);
  Serial.println(" rad/s");

  Serial.print("Temperature: ");
  Serial.print(temp.temperature);
  Serial.println(" degC");

  Serial.println("");





  digitalWrite(greenLED,LOW);
  
}