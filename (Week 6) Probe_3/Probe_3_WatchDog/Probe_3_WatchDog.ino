//-----------------------------------
//---------OOCSI SETUP --------------
//-----------------------------------
#include "OOCSI.h"

// use this if you want the OOCSI-ESP library to manage the connection to the Wifi
// SSID of your Wifi network, the library currently does not support WPA2 Enterprise networks
const char* ssid = "";
const char* password = "";
// name for connecting with OOCSI (unique handle)
const char* OOCSIName = "";
// put the adress of your OOCSI server here, can be URL or IP address string
const char* hostserver = "oocsi.id.tue.nl";
// OOCSI reference for the entire sketch
const char* CHANNELName = "Group7_Informed_Prototype";
OOCSI oocsi = OOCSI();

//-----------------------------------
//-----DHT SENSOR SETUP -------------
//-----------------------------------
#include <Adafruit_Sensor.h>
#include <DHT.h>
#define DHTPIN D6     // Digital pin connected to the DHT sensor
//the type of sensor in use:
#define DHTTYPE    DHT11     // DHT 11
DHT dht(DHTPIN, DHTTYPE);
float temperature;
float humidity;


//-----------------------------------
//----------SGP30 Sensor-------------
//-----------------------------------
//SLC connected to D1
//SDA connected to D2

#include <Wire.h>
#include "Adafruit_SGP30.h"

Adafruit_SGP30 sgp;
unsigned int TVOC, eCO2;

/* return absolute humidity [mg/m^3] with approximation formula
  @param temperature [°C]
  @param humidity [%RH]
*/
uint32_t getAbsoluteHumidity() {
  // approximation formula from Sensirion SGP30 Driver Integration chapter 3.15
  const float absoluteHumidity = 216.7f * ((humidity / 100.0f) * 6.112f * exp((17.62f * temperature) / (243.12f + temperature)) / (273.15f + temperature)); // [g/m^3]
  const uint32_t absoluteHumidityScaled = static_cast<uint32_t>(1000.0f * absoluteHumidity); // [mg/m^3]
  return absoluteHumidityScaled;
}

//-----------------------------------
//-------PIR Motion Sensor-----------
//-----------------------------------
#define MOTION_SAMPLE 50
#include "QuickStats.h"
int motionPin = D0;                       // choose the input pin (for PIR sensor)
int motionValue = 0;                      // variable for reading the pin status
QuickStats stats;                         //initialize an instance of this class
float tempValue[MOTION_SAMPLE];

void setup()
{
  Serial.begin(115200);
  WPAPersonalInit();
  OOCSIInit();
  pinMode(motionPin, INPUT);              // declare sensor as input
  dht.begin();
  sgpSetup();
  errorReport();
}

void WPAPersonalInit() {

  pinMode(LED_BUILTIN, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED)
  {
    debugLed(0);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();
}

//-----------------------------------
//-----------OOSCI Setup-------------
//-----------------------------------
void OOCSIInit() {

  delay(1000); //Take some time to open up the Serial Monitor
  // use this to switch off logging to Serial
  // oocsi.setLogging(false);

  // connect wifi and OOCSI to the server
  oocsi.connect(OOCSIName, hostserver, processOOCSI);
}

void sgpSetup() {
  if (! sgp.begin()) {
    Serial.println("Sensor not found :(");
    while (1);
  }
  Serial.print("Found SGP30 serial #");
  Serial.print(sgp.serialnumber[0], HEX);
  Serial.print(sgp.serialnumber[1], HEX);
  Serial.println(sgp.serialnumber[2], HEX);
}


//-----------------------------------
//------------DHT Sensor-------------
//-----------------------------------

float readDHTTemperature() {
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  // Read temperature as Celsius (the default)
  temperature = dht.readTemperature();
}

float readDHTHumidity() {
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  // Read temperature as Celsius (the default)
  humidity = dht.readHumidity();
}

void readSgp() {

  // If you have a temperature / humidity sensor, you can set the absolute humidity to enable the humditiy compensation for the air quality signals
  sgp.setHumidity(getAbsoluteHumidity());
  if (! sgp.IAQmeasure()) {
    Serial.println("Measurement failed");
    return;
  }
  TVOC = sgp.TVOC;
  eCO2 = sgp.eCO2;

  delay(1000);
}

void readMotion() {

  for (int i = 0; i < MOTION_SAMPLE; i ++) {
    tempValue[i] = digitalRead(motionPin);
  }
  motionValue = stats.maximum(tempValue, MOTION_SAMPLE);

  Serial.println(motionValue);
}

void dataDebug() {

  readDHTTemperature();
  readDHTHumidity();
  readSgp();
  readMotion();
  Serial.println("The temperature is :");
  Serial.print(temperature);
  Serial.println();
  Serial.println("The humidity is :");
  Serial.print(humidity);
  Serial.println();
  Serial.print("TVOC "); Serial.print(TVOC); Serial.print(" ppb\t");
  Serial.print("eCO2 "); Serial.print(eCO2); Serial.print(" ppm\t");
}

//-----------------------------------
//--------Update Data----------------
//-----------------------------------

void updateSensor() {

  oocsi.newMessage(CHANNELName);
  oocsi.addString("device_id" , "ddcdb5aa4642b4616");
  oocsi.addString("activity" , "kitchen_cleaness");
  oocsi.addFloat("humidity", humidity);
  oocsi.addFloat("temperature", temperature);
  oocsi.addInt("TVOC", TVOC);
  oocsi.addInt("eCO2", eCO2);
  oocsi.addBool("presence", motionValue);
  oocsi.sendMessage();
  debugLed(2);
}

void errorReport() {

  oocsi.newMessage("error_report");
  oocsi.addString("device_id" , "ddcdb5aa4642b4616");
  oocsi.addString("activity" , "System (re)booting");
  oocsi.sendMessage();
}

void loop() {

  dataDebug();
  updateSensor();
  checkNetwork();
  delay(5000);
}

void checkNetwork() {

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Connection failed...");
    Serial.println("Rebooting...");
    ESP.restart();
  }
}

void debugLed(int errorCode) {

  switch (errorCode) {

    case 0:
      pinMode(LED_BUILTIN, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
      digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on by making the voltage LOW
      delay(50);
      digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED off by making the voltage HIGH
      delay(100);
      break;

    case 1:
      pinMode(LED_BUILTIN, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
      digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on by making the voltage LOW
      delay(2000);                      // Wait for a second
      digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED off by making the voltage HIGH
      delay(500);
      break;

    case 2:
      pinMode(LED_BUILTIN, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
      for (int i = 0; i < 10; i++) {
        digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on by making the voltage LOW
        delay(15);
        digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED off by making the voltage HIGH
        delay(20);
      }
      break;
  }
}
void processOOCSI() {
}
