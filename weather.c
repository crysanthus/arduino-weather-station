/*
   Environmental Data Sensing and Storing
   mscs crysanthus@gmail.com
*/
#include <SoftwareSerial.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <Wire.h>
#include <BH1750FVI.h>

SoftwareSerial homeStatESP(6, 7); // RX, TX // ESP Module

DHT_Unified dhtSensor(2, DHT11); /// DHT sensor

BH1750FVI bhLightSensor; // BH1750 Ambient Light Sensor
/*sensor connected
  VCC >>> 3.3V
  SDA >>> A4
  SCL >>> A5
  addr >> A3
  Gnd >>>Gnd
*/

uint32_t delayMS;

String asSensorData[3];

// SendCommand prototype
boolean SendCommand(String sATCmd, String sResponse, boolean bCRLF = true);

void setup()
{
  Serial.begin(9600);

  homeStatESP.begin(115200);

  /* --- SENSORS
  */
  // dht sensor
  dhtSensor.begin();

  sensor_t sensor;
  delayMS = sensor.min_delay / 1000;

  // light sensor
  bhLightSensor.begin();
  bhLightSensor.SetAddress(Device_Address_H);//Address 0x5C
  bhLightSensor.SetMode(Continuous_H_resolution_Mode);

  // coonect to ap
  while (!JoinAP());

}

void loop() {

  // Get Temp
  sensors_event_t event;
  dhtSensor.temperature().getEvent(&event);
  if (isnan(event.temperature)) {
    Serial.println("Error reading temperature!");
  }
  else {
    asSensorData[0] = (String)int(event.temperature);
  }

  // Get Humd
  dhtSensor.humidity().getEvent(&event);
  if (isnan(event.relative_humidity)) {
    Serial.println("Error reading humidity!");
  }
  else {
    asSensorData[1] = (String)int(event.relative_humidity);
  }

  // Get Lux / Lumn
  uint16_t lux = bhLightSensor.GetLightIntensity();
  asSensorData[2] = (String)int(lux);

  Serial.println(asSensorData[0] + "*C - " + asSensorData[1] + "% - " + asSensorData[2] + "lux");

  PostData(asSensorData);

  // Delay between measurements.
  delay(60000);
}

/* ESP Init
*/
void StartESP() {
  Serial.println("ESP init!");
  SendCommand("AT+RST", "OK");
}

/* AT Commander
*/
boolean SendCommand(String sATCmd, String sResponse, boolean bCRLF = true) {
  Serial.println("CMD rcvd!");
  Serial.println(sATCmd);
  bCRLF ? homeStatESP.println(sATCmd) : homeStatESP.print(sATCmd);
  delay(8000);
  return responseFind(sResponse); // find given response
}

/* AT Response find
*/
boolean responseFind(String sKey) {

  long lTO = millis() + 5000; // Time Out

  char cKey[sKey.length()];
  sKey.toCharArray(cKey, sKey.length());

  while (millis() < lTO) {

    if (homeStatESP.available()) {

      if (homeStatESP.find(cKey)) {
        Serial.println(sKey);
        return true;
      }
    }
  }

  return false; // timeout
}

boolean JoinAP() {
  Serial.println("Joining AP ...");

  String sAP = "AP NAME";
  String sPP = "AP PASSWORD";

  StartESP();

  if (SendCommand("AT+CWJAP=\"" + sAP + "\",\"" + sPP + "\"", "OK")) {
    Serial.println("Joined AP!");
    return true;
  }
  else {
    Serial.println("Failed joining AP ...");
    return false;
  }

}

/* Post to Web API
   a Python Flask API waiting for data
*/
void PostData(String saData[]) {

  String sServer = "WEB SERVER IP OR NAME"; // Without http part
  String sPort = "5000";
  String sURI = "/api/addstatsg?";

  if (SendCommand("AT+CIPSTART=\"TCP\",\"" + sServer + "\"," + sPort, "OK")) {
    Serial.println("TCP connection ready!");

    /**HTTP GET Sample
       GET /api/addstatsg?cert=xyz&amp;temp=34&amp;humd=45&amp;lumn=55 HTTP/1.1
       Host: 192.168.1.5:5000
    */
    String sPostRequest =  String("GET " + sURI);
            sPostRequest += String("cert=xyz&");
            sPostRequest += String("temp=" + saData[0] + "&");
            sPostRequest += String("humd=" + saData[1] + "&");
            sPostRequest += String("lumn=" + saData[2] + " HTTP/1.1\r\n");
            sPostRequest += String("Host: " + sServer + ":" + sPort + "\r\n\r\n"); // ends with 2 x CRLF - Important -
    /*This is by AT design
    */
    SendCommand("AT+CIPSEND=", "", false);
    if (SendCommand(String(sPostRequest.length()), ">")) {
      Serial.println("TCP ready to receive data ...");

      if (SendCommand(sPostRequest, "SEND OK")) {
        Serial.println("Data packets sent ok!");

        while (homeStatESP.available()) {
          Serial.println(homeStatESP.readString());
        }
        // close the connection
        CloseTCPConnection();
      }
      else {
        CloseTCPConnection();
      }
    }
    else {
      CloseTCPConnection();
    }
  } else {
    CloseTCPConnection();
  }
}

/*Close TCP connection
*/
void CloseTCPConnection() {

  while (!SendCommand("AT+CIPCLOSE", "OK")) {
    Serial.print(".");
  }
  Serial.println("TCP connection closed!");
}

/*EOF
*/
