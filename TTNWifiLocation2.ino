#include "ESP8266WiFi.h"
#include <WiFiClientSecure.h>
#include <TheThingsNetwork.h>
#include <SoftwareSerial.h>
#include <Wire.h>  // Include Wire if you're using I2C
#include <SFE_MicroOLED.h>  // Include the SFE_MicroOLED library
#include <ArduinoJson.h>

const char* ssid = "Ronald's iPhone";
const char* password = "12345678";

const char* host = "location.services.mozilla.com";//"/v1/geolocate?key=test";
const int httpsPort = 443;

// Use web browser to view and copy
// SHA1 fingerprint of the certificate
const char* fingerprint = "CF 05 98 89 CA FF 8E D8 5E 5C E0 C2 E4 F7 E6 C3 C7 50 DD 5C";


SoftwareSerial softSerial(13, 15); // RX, TX

#define debugSerial Serial
#define loraSerial softSerial

#define PIN_RESET 255  //
#define DC_JUMPER 0  // I2C Addres: 0 - 0x3C, 1 - 0x3D

TheThingsNetwork ttn;
MicroOLED oled(PIN_RESET, DC_JUMPER);  // I2C Example

const byte devAddr[4] = { 0x7A, 0xFD, 0xAD, 0x24 };
const byte appSKey[16] = { 0xFB, 0x9F, 0xF5, 0x04, 0x47, 0xF8, 0x5C, 0x68, 0x2D, 0xDD, 0xD6, 0x51, 0x19, 0x68, 0x4E, 0x27 };
const byte nwkSKey[16] = { 0x27, 0x28, 0x73, 0xCB, 0x96, 0xE1, 0x88, 0x1F, 0x40, 0xA4, 0xCB, 0xAB, 0xB9, 0x31, 0xA4, 0xEB };


int counter = 1;

void printOledCenter(String text, int font)
{
  int middleX = oled.getLCDWidth() / 2;
  int middleY = oled.getLCDHeight() / 2;
  
  oled.clear(PAGE);
  oled.setFontType(font);
  // Try to set the cursor in the middle of the screen
  oled.setCursor(middleX - (oled.getFontWidth() * (text.length()/2)+1),
                 middleY - (oled.getFontWidth() / 2));
  // Print the text:
  oled.print(text);
  oled.display();
  delay(1500);
  oled.clear(PAGE);
}

void printOledTop(String text, int font)
{  
  oled.clear(PAGE);
  oled.setFontType(font);
  // Try to set the cursor in the top of the screen
  oled.setCursor(0,0);
  // Print the text:
  oled.print(text);
  oled.display();
  delay(1500);
  oled.clear(PAGE);
}

String readDevice(String cmd){
  loraSerial.println(cmd);
  String lineRx = loraSerial.readStringUntil('\n');
  return lineRx.substring(0,lineRx.length()-1);
}

String bssidToString(uint8_t *bssid) {
  char mac[18] = {0};
  sprintf(mac,"%02X:%02X:%02X:%02X:%02X:%02X", bssid[0],  bssid[1],  bssid[2], bssid[3], bssid[4], bssid[5]);
  return String(mac);
}

String bssidToStringShort(uint8_t *bssid) {
  char mac[18] = {0};
  sprintf(mac,"%02X%02X%02X%02X%02X%02X", bssid[0],  bssid[1],  bssid[2], bssid[3], bssid[4], bssid[5]);
  return String(mac);
}

void wifiScan(){
  // WiFi.scanNetworks will return the number of networks found
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0)
    Serial.println("no networks found");
  else
  {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i)
    {
      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE)?" ":"*");
      Serial.println(bssidToString(WiFi.BSSID(i)));
      delay(10);
    }
  }
  Serial.println("");
}

String wifiScanToJson(){
  int n = WiFi.scanNetworks();
  String s = "{\"wifiAccessPoints\": [";
  if (n == 0)
    Serial.println("no networks found");
  else
  {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i)
    {
      // Print BSSID and RSSI for each network found
      s += "{\"macAddress\": \"";
      s += (bssidToString(WiFi.BSSID(i)));
      s += "\",\"signalStrength\": ";
      s += (WiFi.RSSI(i));
      s += i<n-1?"},":"}";
      delay(10);
    }
    s += "]}";
  } 
  return s;
}

String wifiScanToBytes(){
  int n = WiFi.scanNetworks();
  String s = "{";
  if (n == 0)
    Serial.println("no networks found");
  else
  {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i)
    {
      // Print BSSID and RSSI for each network found
      //s += "{\"macAddress\": \"";
      s += (bssidToStringShort(WiFi.BSSID(i)));
      s += ":";
      s += (WiFi.RSSI(i));
      s += i<n-1?",":"}";
      delay(10);
    }
    //s += "]}";
  } 
  Serial.print(s);
  return s;
}

void wifiPost(String body) {
  // Use WiFiClientSecure class to create TLS connection
  WiFiClientSecure client;
  Serial.print("connecting to ");
  Serial.println(host);
  if (!client.connect(host, httpsPort)) {
    Serial.println("connection failed");
    return;
  }

  if (client.verify(fingerprint, host)) {
    Serial.println("certificate matches");
  } else {
    Serial.println("certificate doesn't match");
  }

  String url = "/v1/geolocate?key=test";
  Serial.print("requesting URL: ");
  Serial.println(url);

  client.println("POST /v1/geolocate?key=test HTTP/1.1");
  client.println("Host: location.services.mozilla.com");
  client.println("Cache-Control: no-cache");
  client.println("Content-Type: application/json");
  client.print("Content-Length: ");
  client.println(body.length());
  client.println();
  client.println(body);
  client.println("Connection: close");
  Serial.println("request sent");
  
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println(">>> Client Timeout !");
      client.stop();
      return;
    }
  }
  String response;
  // Read all the lines of the reply from server and print them to Serial
  while (client.available()) {
    String partialRes = client.readStringUntil('\n');
    Serial.println(partialRes);
    if (partialRes.startsWith("{\"location")){
      response = partialRes;
    }
  }
  Serial.print("Json response: ");
  Serial.println(response);
  //Parse the response
  DynamicJsonBuffer  jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(response);
  // Test if parsing succeeds.
  if (!root.success()) {
    Serial.println("parseObject() failed");
    return;
  }
  float lat = root["location"]["lat"];
  float lng = root["location"]["lng"];
  float acc = root["accuracy"];
  //send response to oled
  String coordString = "LAT:"+String(lat)+"\n"+"LNG:"+String(lng)+"\n"+"ACC:"+String(acc)+"\n"+counter;
  printOledTop(coordString,0);
  counter++;
  Serial.println();
  Serial.println("closing connection");
}

void ttnPost(String body){
  debugSerial.println("Sleeping for 2 seconds before starting sending out test packets.");
  delay(2000);
  //body = "Test";
  // Create a buffer with three bytes  
  //byte data[3] = { 0x01, 0x02, 0x03 };
  byte data[body.length()+1];
  body.getBytes(data, body.length()+1);
  // Send it to the network
  int result = ttn.sendBytes(data, sizeof(data));
  debugSerial.print("TTN Message Sent: ");
  debugSerial.println(counter);
  String msgString = "TTN Sent";
  msgString += "\nBytes:"+String(sizeof(data));
  msgString += "\nTries:"+String(counter);
  if (result < 0){
    msgString += "\nFailure";
  }
  else {
    debugSerial.print("Success but no downlink");
    msgString += "\nSuccess";
  }
  if (result > 0) {
    debugSerial.println("Received " + String(result) + " bytes");
    // Print the received bytes
    msgString += "\n";
    for (int i = 0; i < result; i++) {
      debugSerial.print(String(ttn.downlink[i]) + " ");
      msgString += String(ttn.downlink[i]);
    }
    debugSerial.println();
  }
  printOledTop(msgString,0);
}

void setup()
{
  oled.begin();
  oled.clear(ALL);
  oled.display();
  delay(1000);
  oled.clear(PAGE);
  
  debugSerial.begin(115200);
  loraSerial.begin(57600);//57600,9600
  
  //reset rn2483
  pinMode(12, OUTPUT);
  digitalWrite(12, LOW);
  delay(500);
  digitalWrite(12, HIGH);

  ttn.init(loraSerial, debugSerial);
  ttn.reset();
  ttn.personalize(devAddr, nwkSKey, appSKey);
  
  #ifdef DEBUG
  ttu.debugStatus();
  #endif
  Serial.println("Setup for The Things Network.");

  // Set WiFi to station mode and disconnect from an AP if it was previously connected
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  
//  WiFi.begin(ssid, password);
//  while(WiFi.waitForConnectResult() != WL_CONNECTED){
//    WiFi.begin(ssid, password);
//    Serial.println("WiFi failed, retrying.");
//    printOledCenter("Wifi fail",0);
//  }
//  
//  debugSerial.println("Wifi Setup done");
//  printOledCenter("Wifi conn",0);
}

void loop()
{
//  debugSerial.println("Device Information");
//  debugSerial.println();
//  ttn.showStatus();
//
//  printOledCenter("Batt:",0);
//  printOledCenter(readDevice("sys get vdd"),0);
//  printOledCenter("EUI:",0);
//  printOledCenter(readDevice("sys get hweui"),0);

//  wifiScan();
//  String body = wifiScanToJson();
//  debugSerial.println(body);
//  wifiPost(body);
  ttnPost(wifiScanToBytes());
  counter++;
  delay(10000);
}
