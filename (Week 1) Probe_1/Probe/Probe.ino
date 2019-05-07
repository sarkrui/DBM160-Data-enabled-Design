/******************************************************************************
   Example of the OOCSI-ESP library connecting to WiFi and sending messages
   over OOCSI to the ID data foundry project site.
 ******************************************************************************/

#include "OOCSI.h"

#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  200      /* Time ESP32 will go to sleep (in seconds) */

// use this if you want the OOCSI-ESP library to manage the connection to the Wifi
// SSID of your Wifi network, the library currently does not support WPA2 Enterprise networks
const char* ssid = "";
// Password of your Wifi network.
const char* password = "";

// name for connecting with OOCSI (unique handle)
const char* OOCSIName = "dc7e66082f4ae48cb";
// put the adress of your OOCSI server here, can be URL or IP address string
const char* hostserver = "oocsi.id.tue.nl";

// OOCSI reference for the entire sketch
OOCSI oocsi = OOCSI();

//Dust sensor setup
#define DUST_SENSOR_PIN 25
unsigned long duration;
unsigned long starttime;
unsigned long sampletime_ms = 2000;
unsigned long lowpulseoccupancy = 0;
float ratio = 0;
float concentration = 0;

//Temp sensor setup
#include <Wire.h>
#include <Adafruit_MLX90614.h>
Adafruit_MLX90614 mlx = Adafruit_MLX90614();

// put your setup code here, to run once:
void setup() {

  Serial.begin(115200);
  delay(1000); //Take some time to open up the Serial Monitor

  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);

  // use this to switch off logging to Serial
  // oocsi.setLogging(false);

  // connect wifi and OOCSI to the server

  oocsi.connect(OOCSIName, hostserver, ssid, password, processOOCSI);

  //sensor setup
  pinMode(DUST_SENSOR_PIN, INPUT);
  mlx.begin();

}

float dustSensor() {

  duration = pulseIn(DUST_SENSOR_PIN, LOW);
  lowpulseoccupancy = lowpulseoccupancy + duration;
  ratio = lowpulseoccupancy / (sampletime_ms * 10.0);
  concentration = 1.1 * pow(ratio, 3) - 3.8 * pow(ratio, 2) + 520 * ratio + 0.62;
  Serial.print("Concentration = ");
  Serial.print(concentration);
  Serial.println(" pcs/0.01cf");
  lowpulseoccupancy = 0;
  return concentration;

}

float tempSensor() {

  float tempValue = mlx.readObjectTempC();
  Serial.print("*C\tObject = "); 
  Serial.print(tempValue); 
  Serial.println("*C");
  return tempValue;

}

void updateSensor() {

  oocsi.newMessage("Group_7_1st_Test");
  oocsi.addString("device_id" , "dc7e66082f4ae48cb");
  oocsi.addString("activity" , "Testing");
  oocsi.addFloat("dust_sensor", dustSensor());
  oocsi.addFloat("temp_sensor", tempSensor());
  oocsi.sendMessage();

}

void loop() {

  updateSensor();
  delay(1000); //update sensor data every second

}

void processOOCSI() {
  // don't do anything; we are sending only
}
