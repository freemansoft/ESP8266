#define ENABLE_HTTP_UPDATE
#define ENABLE_OTA
#define INITIAL_RELAY_STATE false // This will cause the relay to flip false/true/false on startup

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

//
//  This program was put together to exercise the LinkNode R1 as a web endpoint.
//  The node brings up an AP when it does not find a network that it previously used with valid credentials.
//  You can connect to the AP with a phone/tablet or other device by selecting the AP network.
//  The program saves the credentials in NVRAM so that it can log into the host network on restart
//  
//  You can reset the connection information by uncommenting a line documented in StartUp()
//  

// GPIO16 connected to the RELAY
const int RELAY_PIN = 16;
// ESP8266 modules have on-module LED hooped to this pin. It used to be on GPIO1 which is the TX pin
const int GPIO2_LED_PIN = 2;
// retain this so we can tell the UI the state.  No easy way to read the pin status in output mode
bool relay_state = false;
// this is global so that we can reset the manager if requested
WiFiManager wifiManager;
MDNSResponder mdns;
// global so multiple functions can see it
ESP8266WebServer server(80);
// the SSID when we bring up the configuration portal
// also our host name once we join thenetwork
String ssid = "ESP8266-" + String(ESP.getChipId());
String serverUrl = "http://"+ssid+".local";
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
  "<P>Use this form:<BR>"
  "<INPUT type=\"radio\" name=\"RELAY\" id='relay_radio_on' value=\"1\">Relay On<BR>"
  "<INPUT type=\"radio\" name=\"RELAY\" id='relay_radio_off' value=\"0\">Relay Off<BR>"
  "<INPUT type=\"submit\" value=\"Send\"> <INPUT type=\"reset\">"
  "</P>"
  "</FORM>"
  "<P>Or Click to <a href='/relayoff'>Turn Relay Off</a> or <a href='/relayon'>Turn Relay On</a></P>"
  ;

// string built this way so it is never an empty string
const char INDEX_HTTP_ADMIN_LINKS[] =
  "<HR>"
  "<p>Admin username and password required</p>"
  "<ul>"
  "<li><a href='/reset'>Reset the device</a></li>"
  "<li><a href='/resetwifi'>Reset wifi and restart into portal</a> Warning!! disconnects from current wifi</li>"
#ifdef ENABLE_HTTP_UPDATE
  "<li><a href='/update'>Update firmware via web page</a></li>"
#endif
  "</ul>"
  ;

// should this have its own <p></p>?
const char INDEX_BUILDINFO[] = 
  "<HR>"
  "Software Built: "
  __DATE__
  " "
  __TIME__
  " IP: "
  ;

const char INDEX_JAVASCRIPT_RELAY_ON[] = 
  "<script>"
  "document.getElementById('relay_radio_on').checked = true;"
  "</script>"
  ;
  
const char INDEX_JAVASCRIPT_RELAY_OFF[] = 
  "<script>"
  "document.getElementById('relay_radio_off').checked = true;"
  "</script>"
  ;

// let's end this!
const char INDEX_FOOTER[] =
  "</body>"
  "</html>";

//
// a request came in on  "/" so either return page or process submit
//
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

//
// called if handleRoot() saw a form element
//
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

//
// return 200 and the ok message in the body
//
void returnOK()
{
  server.sendHeader("Connection", "close");
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  // return to the root page
  server.send(200, "text/html", "");
  server.sendContent(INDEX_HTML);
  server.sendContent(INDEX_HTTP_ADMIN_LINKS);
  server.sendContent(INDEX_BUILDINFO);
  server.sendContent(WiFi.localIP().toString());
  // so lazy...
  if (relay_state){
    server.sendContent(INDEX_JAVASCRIPT_RELAY_ON);
  } else {
    server.sendContent(INDEX_JAVASCRIPT_RELAY_OFF);
  }
  server.sendContent(INDEX_FOOTER);
  server.client().stop();
}

// Imperative to turn the RELAY on using a non-browser http client.
// For example, using wget.
// $ wget http://LOCAL_DNS_NAME/relayon
void handleRelayOn()
{
  writeRelay(true);
  returnOK();
}

//
// Imperative to turn the RELAY off using a non-browser http client.
// For example, using wget.
// $ wget http://LOCAL_DNS_NAME/relayoff
//
void handleRelayOff()
{
  writeRelay(false);
  returnOK();
}

//
// return 404 with info
//
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

// force a reboot
// reset request requires authentication
void handleReset(){
  if (!server.authenticate(update_username,update_password)){
    return server.requestAuthentication();
  }
  server.send(200, "text/html", "<html><head></head><body>Resetting device. <a href='/'>Click here after reset<a> to return to home page</body></html>");
  deviceReset();
}

// clear wifi passwords and reboot
// reset request requires authentication
void handleResetWiFi(){
  if (!server.authenticate(update_username,update_password)){
    return server.requestAuthentication();
  }
  server.send(200, "text/html", "<html><head></head><body>Resetting wifi passwords and device. Connect wifi to soft AP</body></html>");
  wifiManager.resetSettings();
  deviceReset();
}

// lifted from
// https://github.com/chrislbennett/ESP8266-WemosRelay/blob/master/src/main.ino
void deviceReset()
{
  delay(3000);
  ESP.reset();
  delay(5000);
}

void writeRelay(bool RelayOn)
{
  if (RelayOn){
    digitalWrite(RELAY_PIN, 1);
    relay_state=true;
  }else {
    digitalWrite(RELAY_PIN, 0);
    relay_state=false;
  }
}

void runWiFiManager(){
    // https://github.com/tzapu/WiFiManager/blob/master/examples/AutoConnect/AutoConnect.ino
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

//
// Prepare the web server to handle requests
//
void configureWebServer(String url){
    server.on("/", handleRoot);
    server.on("/relayon", handleRelayOn);
    server.on("/relayoff", handleRelayOff);
    server.onNotFound(handleNotFound);
    server.on("/reset", handleReset);
    server.on("/resetwifi", handleResetWiFi);
    
    Serial.println("Web server enabled: "+url+" or http://"+WiFi.localIP().toString()); 
    Serial.println("Relay Form: "+url+"/");
    Serial.println("GET "+url+"/relayoff");
    Serial.println("GET "+url+"/relayon");

}

//
// add the httpUpdater endpoints to the http server
//
void configureWebUpdateIfEnabled(String url, const char *updatePath, const char* username, const char* password){
#ifdef ENABLE_HTTP_UPDATE
    httpUpdater.setup(&server,updatePath, username, password);
    Serial.printf("Web update enabled %s User:%s Pw:%s\r\n", url.c_str(), username, password);
#endif
}

//
// Configure direct upload OTA
//
void activateOTAIfEnabled(const char* password){
#ifdef ENABLE_OTA
    ArduinoOTA.setPassword(password);
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
    Serial.print("OTA enabled pw:");  
    Serial.println(password);
#endif
}

//
// Configure the components.  The component setup calls are responsible forknowing if their feature is enabled
//
void setup()
{
    Serial.begin(115200);
    Serial.print("BuildInfo: ");
    Serial.print(__DATE__);
    Serial.print(" ");
    Serial.println(__TIME__);
    pinMode(RELAY_PIN, OUTPUT);
    writeRelay(INITIAL_RELAY_STATE);

    runWiFiManager();
    configureWebServer(serverUrl);
    configureWebUpdateIfEnabled(serverUrl,update_path,update_username,update_password);
    server.begin();
    activateOTAIfEnabled(update_password);

    pinMode(GPIO2_LED_PIN, OUTPUT);

}

// loop counter
long counter = 0;
// current output state
int output = 0;

void loop()
{ 
    counter++;
    // toggle pin two (onmodule LED) every 10,000 times through this loop
    if (counter % 100000 == 0){
      digitalWrite(GPIO2_LED_PIN, (++output % 2));
    }
    server.handleClient();
#ifdef ENABLE_OTA
    ArduinoOTA.handle();
#endif
}
