#include <ESP8266WiFi.h>          //ESP8266 Core WiFi Library (you most likely already have this in your sketch)
#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include <ESP8266mDNS.h>          // So we can have $LOCAL_DNS_NAME.local
                                  // web server code from https://gist.github.com/bbx10/5a2885a700f30af75fc5

/**
 * This program was put together to exercise the LinkNode R1 as a web endpoint.
 * The node brings up an AP when it does not find a network that it previously used with valid credentials.
 * You can connect to the AP with a phone/tablet or other device by selecting the AP network.
 * The program saves the credentials in NVRAM so that it can log into the host network on restart
 * 
 * You can reset the connection information by uncommenting a line documented in StartUp()
 * 
 */
const int RELAY_PIN = 16;
MDNSResponder mdns;
ESP8266WebServer server(80);
const char LOCAL_DNS_NAME[] = "esp8266-linksprite";


const char INDEX_HTML[] =
  "<!DOCTYPE HTML>"
  "<html>"
  "<head>"
  "<meta name = \"viewport\" content = \"width = device-width, initial-scale = 1.0, maximum-scale = 1.0, user-scalable=0\">"
  "<title>ESP8266 Web Form Demo</title>"
  "<style>"
  "\"body { background-color: #808080; font-family: Arial, Helvetica, Sans-Serif; Color: #000000; }\""
  "</style>"
  "</head>"
  "<body>"
  "<h1>ESP8266 LinkNode R1 Relay Web Form Demo</h1>"
  "<FORM action=\"/\" method=\"post\">"
  "<P>"
  "LED<br>"
  "<INPUT type=\"radio\" name=\"LED\" value=\"1\">On<BR>"
  "<INPUT type=\"radio\" name=\"LED\" value=\"0\">Off<BR>"
  "<INPUT type=\"submit\" value=\"Send\"> <INPUT type=\"reset\">"
  "</P>"
  "</FORM>"
  "<br><br>Change relay status directly: "
  "<a href='/relayoff'>Turn off relay</a> or <a href='/relayon'>Turn on relay</a>"
  "</body>"
  "</html>";


void handleRoot()
{
  if (server.hasArg("LED")) {
    handleSubmit();
  }
  else {
    server.send(200, "text/html", INDEX_HTML);
  }
}

void returnFail(String msg)
{
  server.sendHeader("Connection", "close");
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(500, "text/plain", msg + "\r\n");
}

void handleSubmit()
{
  String LEDvalue;

  if (!server.hasArg("LED")) return returnFail("BAD ARGS");
  LEDvalue = server.arg("LED");
  if (LEDvalue == "1") {
    writeRelay(true);
    server.send(200, "text/html", INDEX_HTML);
  }
  else if (LEDvalue == "0") {
    writeRelay(false);
    server.send(200, "text/html", INDEX_HTML);
  }
  else {
    returnFail("Bad LED value");
  }
}

/*
 * return 200 and the ok message in the bod
 */
void returnOK()
{
  server.sendHeader("Connection", "close");
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", "OK\r\n");
}

/*
 * Imperative to turn the LED on using a non-browser http client.
 * For example, using wget.
 * $ wget http://LOCAL_DNS_NAME/relayon
 */
void handleRelayOn()
{
  writeRelay(true);
  returnOK();
}

/*
 * Imperative to turn the LED off using a non-browser http client.
 * For example, using wget.
 * $ wget http://LOCAL_DNS_NAME/relayoff
 */
void handleRelayOff()
{
  writeRelay(false);
  returnOK();
}

void handleNotFound()
{
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void writeRelay(bool RelayOn)
{
  if (RelayOn)
    digitalWrite(RELAY_PIN, 1);
  else
    digitalWrite(RELAY_PIN, 0);
}

/**
 * This call back happens when network join fails and we fall into soft AP
 */
void setup()
{
    String s;
    Serial.begin(115200);
    pinMode(RELAY_PIN, OUTPUT);
    writeRelay(false);

    //https://github.com/tzapu/WiFiManager/blob/master/examples/AutoConnect/AutoConnect.ino
    //WiFiManager
    //Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wifiManager;

    //reset saved settings - good for development
    //can't stay active though because it will erase settings causing reboot to AP.
    //wifiManager.resetSettings();
    
    //set custom ip for portal
    //wifiManager.setAPStaticIPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));

    //Fetches ssid and pass from eeprom and tries to connect
    //if it does not connect it starts an access point with the specified name here
    //"ESP8266AutoConfigAP" and goes into a blocking loop awaiting configuration
    wifiManager.autoConnect("ESP8266AutoConfigAP");
    //or use this for auto generated name ESP + ChipID
    //wifiManager.autoConnect();

    
    //if you get here you have connected to the WiFi
    Serial.println("");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  
    if (mdns.begin(LOCAL_DNS_NAME, WiFi.localIP())) {
      Serial.println("MDNS responder started");
    }
  
    server.on("/", handleRoot);
    server.on("/relayon", handleRelayOn);
    server.on("/relayoff", handleRelayOff);
    server.onNotFound(handleNotFound);
  
    server.begin();
    Serial.print("Connect to http://");
    Serial.print(LOCAL_DNS_NAME);
    Serial.print(".local or http://");
    Serial.println(WiFi.localIP()); 
    Serial.println("Form entry available with GET on /");
    Serial.print("Direct links ");
    Serial.print("Turn off Relay /relayoff or Turn on relay");
}

void loop()
{ 
    server.handleClient(); 
}
