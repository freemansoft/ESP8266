  #define ENABLE_HTTP_UPDATE
#define ENABLE_OTA

#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include <ESP8266mDNS.h>          // So we can have $LOCAL_DNS_NAME.local
                                  // web server code from https://gist.github.com/bbx10/5a2885a700f30af75fc5
#ifdef ENABLE_OTA
  #include <ArduinoOTA.h>
#endif
#ifdef ENABLE_HTTP_UPDATE
  #include <ESP8266HTTPUpdateServer.h>
#endif

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
String ssid = "ESP8266-" + String(ESP.getChipId());
#if defined(ENABLE_HTTP_UPDATE) || defined(ENABLE_OTA)
  // used by both the http and OTA updaters
  String password = String(ESP.getChipId());
  const char* update_username = "admin";
  const char* update_password = password.c_str();
#endif
#if defined(ENABLE_HTTP_UPDATE)
  ESP8266HTTPUpdateServer  httpUpdater;
  const char* update_path = "/update";
#endif


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
  "<h1>ESP8266 LinkNode R1 Demo</h1>"
  "<FORM action=\"/\" method=\"post\">"
  "<P>"
  "<INPUT type=\"radio\" name=\"RELAY\" value=\"1\">Relay On<BR>"
  "<INPUT type=\"radio\" name=\"RELAY\" value=\"0\">Relay Off<BR>"
  "<INPUT type=\"submit\" value=\"Send\"> <INPUT type=\"reset\">"
  "</P>"
  "</FORM>"
  "<br><br>Direct: <a href='/relayoff'>Relay Off</a> or <a href='/relayon'>Relay On</a>"
#ifdef ENABLE_HTTP_UPDATE
  "<br>Update firmware: <a href='/update'>via upload</a>"
#endif
  "</body>"
  "</html>";


void handleRoot()
{
  if (server.hasArg("RELAY")) {
    handleSubmit();
  }
  else {
    returnOK();
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
  String RELAYvalue;

  if (!server.hasArg("RELAY")) {
    return returnFail("BAD ARGS");
  }
  RELAYvalue = server.arg("RELAY");
  if (RELAYvalue == "1") {
    writeRelay(true);
    returnOK();
  }
  else if (RELAYvalue == "0") {
    writeRelay(false);
    returnOK();
  }
  else {
    returnFail("Bad RELAY state value");
  }
}

/*
 * return 200 and the ok message in the bod
 */
void returnOK()
{
  server.sendHeader("Connection", "close");
  server.sendHeader("Access-Control-Allow-Origin", "*");
  // return to the root page
  server.send(200, "text/html", INDEX_HTML);
}

/*
 * Imperative to turn the RELAY on using a non-browser http client.
 * For example, using wget.
 * $ wget http://LOCAL_DNS_NAME/relayon
 */
void handleRelayOn()
{
  writeRelay(true);
  returnOK();
}

/*
 * Imperative to turn the RELAY off using a non-browser http client.
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

void runWiFiManager(){
    // https://github.com/tzapu/WiFiManager/blob/master/examples/AutoConnect/AutoConnect.ino
    // WiFiManager
    // Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wifiManager;
    // this will remove the saved wifi settings - good for development
    // can't stay active though because it will erase settings causing reboot to AP.
    //wifiManager.resetSettings();
    // set custom ip for portal
    //wifiManager.setAPStaticIPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));

    // Fetches SSID and password from eeprom and tries to connect
    // if it does not connect it starts an access point with the SSID specified here
    // and goes into a blocking loop awaiting configuration
    //wifiManager.autoConnect("ESP8266AutoConfigAP");
    // or use this for auto generated name ESP + ChipID
    wifiManager.autoConnect();
    
    // if you get here you have connected to the WiFi
    Serial.println("WiFi connected: "+WiFi.localIP().toString());  

    // register ourselves in dynamic dns under this name.  say "bonjour"
    if (mdns.begin(ssid.c_str(), WiFi.localIP())) {
      Serial.println("MDNS responder started: "+ssid);
    }
}

/**
 * Prepare the web server to handle requests
 */
void configureWebServer(String url){
    server.on("/", handleRoot);
    server.on("/relayon", handleRelayOn);
    server.on("/relayoff", handleRelayOff);
    server.onNotFound(handleNotFound);
    Serial.println("Web server enabled: "+url+" or http://"+WiFi.localIP().toString()); 
    Serial.println("Relay Form: "+url+"/");
    Serial.println("GET "+url+"/relayoff");
    Serial.println("GET "+url+"/relayon");

}

/**
 * add the httpUpdater endpoints to the http server
 */
void configureWebUpdateIfEnabled(String url){
#ifdef ENABLE_HTTP_UPDATE
    httpUpdater.setup(&server,update_path, update_username, update_password);
    Serial.printf("Web update enabled %s User:%s Pw:%s\r\n", url.c_str(), update_username, update_password);
#endif
}

/**
 * Configure direct upload OTA
 */
void activateOTAIfEnabled(){
#ifdef ENABLE_OTA
    ArduinoOTA.setPassword(update_password);
    // event handlers
    ArduinoOTA.onStart([]() {
      Serial.print("Start OTA:");
    });
    ArduinoOTA.onEnd([]() {
      Serial.println(":End OTA");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
      Serial.print(".");
    });
    ArduinoOTA.onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });
    ArduinoOTA.begin();
    Serial.println("OTA enabled");  
#endif
}

/**
 * This call back happens when network join fails and we fall into soft AP
 */
void setup()
{
    Serial.begin(115200);
    pinMode(RELAY_PIN, OUTPUT);
    writeRelay(false);

    runWiFiManager();
    String url = "http://"+ssid+".local";
    configureWebServer(url);
    configureWebUpdateIfEnabled(url);
    server.begin();
    activateOTAIfEnabled();
}


void loop()
{ 
    server.handleClient();
#ifdef ENABLE_OTA
    ArduinoOTA.handle();
#endif
}
