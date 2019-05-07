//-----------------------------------
//---------OOCSI SETUP --------------
//-----------------------------------
#include "OOCSI.h"

#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  200      /* Time ESP32 will go to sleep (in seconds) */

// use this if you want the OOCSI-ESP library to manage the connection to the Wifi
// SSID of your Wifi network, the library currently does not support WPA2 Enterprise networks
const char* ssid = "";
// Password of your Wifi network.
const char* password = "";

// name for connecting with OOCSI (unique handle)
const char* OOCSIName = "ddcdb5aa4642b4616";
// put the adress of your OOCSI server here, can be URL or IP address string
const char* hostserver = "oocsi.id.tue.nl";

// OOCSI reference for the entire sketch
OOCSI oocsi = OOCSI();


//-----------------------------------
//---------Noise metering setup------
//-----------------------------------
#include "QuickStats.h"
#define AUDIO_PIN A0 //ESP-12E Module has only one analog pin, A0
#define SAMPLE_SIZE 50

// Sample window width in mS (50 mS = 20Hz)
// we choose a sample window of 50 milliseconds.
// That is sufficient to measure sound levels of frequencies as low as 20 Hz
// the lower limit of human hearing.

float dB = 50;
unsigned long id = 0;
float readingArray[SAMPLE_SIZE];
float smoothReading;
float currentReading; //Save noiseReading();
QuickStats stats; //QuickStats initialization

//-----------------------------------
//-----DHT SENSOR SETUP -------------
//-----------------------------------
#include <Adafruit_Sensor.h>
#include <DHT.h>
#define DHTPIN D6     // Digital pin connected to the DHT sensor
//the type of sensor in use:
#define DHTTYPE    DHT11     // DHT 11
DHT dht(DHTPIN, DHTTYPE);

void setup()
{
  Serial.begin(115200);
  delay(1000);
  dht.begin();
  OOSCIInit();
}

//-----------------------------------
//-----------OOSCI Setup-------------
//-----------------------------------
void OOSCIInit() {

  delay(1000); //Take some time to open up the Serial Monitor
  // use this to switch off logging to Serial
  // oocsi.setLogging(false);

  // connect wifi and OOCSI to the server

  oocsi.connect(OOCSIName, hostserver, ssid, password, processOOCSI);
}

//-----------------------------------
//--------DHT Sensor    -------------
//-----------------------------------

float readDHTTemperature() {
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  Serial.println("The temperature is :");
  Serial.print(t);
  Serial.println();
  return t;
}

float readDHTHumidity() {
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  // Read temperature as Celsius (the default)
  Serial.println("The humidity is :");
  float h = dht.readHumidity();
  Serial.print(h);
  Serial.println();
  return h;
}


//-----------------------------------
//----------Noise Sensor-------------
//-----------------------------------

float noiseReading() {

  //Read <sampleSize> times and put them into one array
  for (uint8_t i = 0; i < SAMPLE_SIZE; i++) {

    int tempAdc = analogRead(AUDIO_PIN);
    //toss out spurious readings, ignoring readings larger than max 1024
    if (tempAdc < 1024 && tempAdc > 200) {

      readingArray[i] = tempAdc;
      delay(10);
    }
  }

  //Smoothing (median method)
  smoothReading = stats.median(readingArray, SAMPLE_SIZE);

  //Convert ADC value to dB using Regression values
  dB = (smoothReading + 83.2073) / 11.003;
  Serial.println("The decibel is :");
  Serial.print(dB);
  Serial.println();
  //Return noise level in the format of dB
  return dB;
}

//-----------------------------------
//--------Update Data----------------
//-----------------------------------

void updateSensor() {

  oocsi.newMessage("Group_7_2nd_Test_ESP8266");
  oocsi.addString("device_id" , "ddcdb5aa4642b4616");
  oocsi.addString("activity" , "Bedroom_noise_humidity_temp");
  oocsi.addFloat("decibel", noiseReading());
  oocsi.addFloat("humidity", readDHTHumidity());
  oocsi.addFloat("temperature", readDHTTemperature());
  oocsi.sendMessage();

}

void dataDebug() {

  readDHTTemperature();
  readDHTHumidity();
  noiseReading();
  delay(10000);
}

void loop() {

  //dataDebug();
  updateSensor();
  delay(30000);
}

void processOOCSI() {

}
