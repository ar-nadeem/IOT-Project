#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"
#include <WiFi.h>
#include <Arduino_JSON.h>
#include <assert.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

MAX30105 particleSensor;

#define MAX_BRIGHTNESS 255
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////// SPO 2 Sensor Variables ///////////////////////////////////////////////////
const byte RATE_SIZE = 4; //Increase this for more averaging. 4 is good.
byte rates[RATE_SIZE]; //Array of heart rates
byte rateSpot = 0;
long lastBeat = 0; //Time at which the last beat occurred

float beatsPerMinute;
int beatAvg;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////     Wifi Server        ///////////////////////////////////////////////////
const char* ssid     = "The Gateway";
const char* password = "HokieHouse";
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

  heartJson["heart"] = -1;
  heartJson["heart_avg"] = -1;
  
  pinMode(redLED, OUTPUT);
  pinMode(yellowLED, OUTPUT);
  pinMode(greenLED, OUTPUT);
  pinMode(blueLED, OUTPUT);

  digitalWrite(redLED,HIGH);

  Serial.begin(115200); // initialize serial communication at 115200 bits per second:

  // Connect to wifi and start the server
  // Serial.print("Connecting to ");
  // Serial.println(ssid);
  // WiFi.begin(ssid, password);

  //   while (WiFi.status() != WL_CONNECTED) {
  //       delay(500);
  //       Serial.print(".");
  //   }
  //   Serial.println("");
  //   Serial.println("WiFi connected.");
  //   Serial.println("IP address: ");
  //   Serial.println(WiFi.localIP());
    
  // server.begin();



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

  particleSensor.setup(); //Configure sensor with default settings
  particleSensor.setPulseAmplitudeRed(0x0A); //Turn Red LED to low to indicate sensor is running
  particleSensor.setPulseAmplitudeGreen(0); //Turn off Green LED

  
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



  digitalWrite(redLED,LOW);

  // Wait for 3 seconds before starting reading
  for (int i = 0; i <=3; i++) {
    digitalWrite(yellowLED,HIGH);
    delay(900);
    digitalWrite(yellowLED,LOW);
    delay(100);
  }


}


void getAccTemp(){
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
}

JSONVar getHeart(){
    long irValue = particleSensor.getIR();

    if (checkForBeat(irValue) == true) {
      //We sensed a beat!
      long delta = millis() - lastBeat;
      lastBeat = millis();

      beatsPerMinute = 60 / (delta / 1000.0);

      if (beatsPerMinute < 255 && beatsPerMinute > 20) {
        rates[rateSpot++] = (byte)beatsPerMinute; //Store this reading in the array
        rateSpot %= RATE_SIZE; //Wrap variable

        //Take average of readings
        beatAvg = 0;
        for (byte x = 0 ; x < RATE_SIZE ; x++)
          beatAvg += rates[x];
        beatAvg /= RATE_SIZE;
      }
    }

    Serial.print("IR=");
    Serial.print(irValue);
    Serial.print(", BPM=");
    Serial.print(beatsPerMinute);
    Serial.print(", Avg BPM=");
    Serial.print(beatAvg);

    if (irValue < 50000)
      Serial.print(" No finger?");

    Serial.println();

    JSONVar myObject;

    myObject["heart"] = beatsPerMinute;
    myObject["heart_avg"] = beatAvg;
    return myObject;
}



void loop()
{

  // heartJson = getHeart();
  // digitalWrite(greenLED,HIGH);
  // Serial.println(heartJson["sp"]);
  // Serial.println(heartJson["heart"]);
  
  getAccTemp();






  digitalWrite(greenLED,LOW);
  
}