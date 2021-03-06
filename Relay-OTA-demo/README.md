## Software Modules ##

The working part of this program is the HTTP bound functions that process inbound form requests and the GET requesets that
manipulate the state of a Relay connected to an ESP8266 GPIO.  

This is **not a true REST application** because it manipulates relay state on a GET call. I know that and am comfortable with how easy this makes it to test. 

Several major libraries are used to support the network and web infrastructure and the ability to do wireless firmware updates.

![Software Block Diagram](docs/BlockDiagram.png?raw=true "Major Components")
###### sigh: if only you could size images with markdown ######

## Features ##
### Web interface for relay control ### 
This application was built for the 
[Link Sprite LinkNode R1](http://linksprite.com/wiki/index.php5?title=LinkNode_R1:_Arduino-compatible_WiFi_relay_controller
) 
ESP8266 based single relay board [found at Microcenter](http://www.microcenter.com/product/484708/linknode_r1_wifi_relay_controller) (as of 2/2018).  It is should be easily modified for the R4 quad relay board.

1) Attempts to find network that it is already configured for.  Attaches to the network iv found.
1) If a known network is not found or it cannot login
    * Boots into a self-configuring Access Point portal. Provides a web interface to select a network and enter the network password.  
    * Reboots after configuration and joins the configured network.
1) Brings up internal web server on _esp8266-\<chip-id\>.local_ after joining network with the following web URLs
    * **/** GET: form
      * POST: to turn on/off relay on
    * **/relayon** GET: turn relay on
    * **/relayoff** GET: turn relay off

### Usage ###
First time on a new network.
1) The program next door  boots the LinkSprite/ESP8266 into a Soft Access Point mode.
1) Use some computer, phone or mobile device to connect to this Access Point
    1)  Disconnect your computer/phone/mobile its current network.
    1) Run the device connection manager on your phone tablet or computer.
    1) Let the device search for the access point network. It will be called "????"
    1) Connect to the access  point.
    1) Enter the name of your local network and it spassword. The ESP8266 wil restart and join to that network: LinkSprite...
1) The esp8266 wil automatically restart and join the configured network.
1) Re-connect your mobile dvice to the legacy network.
1) Open a web broser and connect to http://esp8266-\<chipId\>.local

After the first time on a new network
1) Power up the LinkSprite ESP8266 device
1) Open your browser to http://esp8266-\<chipId\>.local

### Administrative Endpoints ###
The web server adds the following administrative endpoints that require a username and password

* **/reset** Reset/reboot the device.
* **/resetwifi** Reset the wifimanager configuration and reboot into the wifi configuration portal
* **/update** go to the web based firmware update page when this feature is enabled

### Firmware updates without Wires ###
There are two _#define_ macros in the code that enable network based firmware upgrades.  They are enabled by default

1) Firmware update via web form on _/update_ if enabled with "#define" in program
1) Firmware Over The Air (OTA) update directly to port 8266 if enabled with _"#define"_ in program

### Shortcomings ###

1) Does not work on WPA2 Enterprise wireless networks.
1) There is no way to _forget_ a configured network to bring back the AP without reflashing the program.

### Security ###

* The relay control portion of this program is **insecure** with no access control.
* The firmware update feature, for web server form and direct OTA, is **insecure**
with 
    1) username: _admin_ 
    2) password: _\<chipid\>_

## Hardware ##
Built on Link Spring LinkNode Rx that emulates the WeMos D1 (retired)

![An ESP8266 board](docs/LinkNodeR1.jpg?raw=true "Link Sys LinkNode R1")



* ESP-87266EX
* 5V pDC power jack
* 1 Digital/IO Relay
* 6 Digital I/O pins
* 1 Analog Input pin

The Arduino pinout and ESP8266 pin mapping can be found in [pins_arduino.h on github](https://github.com/pcduino/LinkNodeD1/blob/master/variants/linknoded1/pins_arduino.h)
The Arduino pin naming convention doesn't make sense in this context. It is simpler to just refer directly to the ESP8266 pin number itself. The following table shows both representations.

|GPIO	 |Fcn           |Alt Fcn |Arduino pin|
|--------|--------------|--------|-----------| 
|GPIO 04 | SDA?         |        |        D4 |
|GPIO 05 | SCL?         |        |        D3 |
|GPIO 12 | HMISO        |        |        D6 |
|GPIO 13 | HMOSI        | RXD2   |        D7 |
|GPIO 14 | HSCLK        |	     |        D5 |
|GPIO 15 | HCS          | TXD2   |	      D10|
|GPIO 16 |Relay         |        |        D2 |
|GPIO 02 |LED on ESP8266|        |        ?  |
			
## Programming and Updating ## 
### Configuring Arduino IDE ###
Short form instructions 
1) Install the Arduino IDE.
1) Load the .ino file in the current working directory.
1) Add ESP8266 support
    1) Add the ESP8266 board manager repo location because the Arduino doesn't know about ESP8266 boards
        1) File --> Preferences
        1) Add the board manager http://arduino.esp8266.com/stable/package_esp8266com_index.json
    1) Now download the ESP8266 Boards configuration information using the previously configured Board Manager
        1) Tools-->Board-->Board Manager
        1) Type in _esp8266_ in the search field.  This should filter down to fewer choices.
        1) Highlight the _esp8266_ box 
        1) Select the version you want. I used 2.3.0 at the time of this note
        1) Click _install_
        1) Close the window.
        You should now have a package directory in your local profile: c:\users\<username>\AppData\Local\Arduino15\...  It may use your roaming profile depending on the OS version
1) Select a specific ESP8266 board as your target
    1) Tools-->board
    1) Scroll down to you find your board and select it.  The LinkSprite-R1/4 have the same profile as the _WeMos D1(Retired)_. Note that this entry appears through version 2.3.0.
1) Add extended / non-bundled libraries used in this program
    1) Sketch --> include libraries --> Manage Libraries
    1) Add wifi manager to the set of installed libraries.  It can be found at https://github.com/tzapu/WiFiManager
        1) Find the WifiManager. 
            * Type in _wifimanager_
        1) Find the entry by "tzapu"
        1) Click install 
            * I accepted library version 0.12 as of 2/33/2018
            * This will install the WifiManger package on your machine
    1) Add the Over The Air library
        1) Find the ArduinOta. 
            * Type in _arduinoota_
        1) Find the entry by "Grokhotkov" and "Ajo"
        1) Click install 
            * I accepted library version 1.0.0 as of 2/33/2018
            * This will install the WifiManger package on your machine
1) Close the library manager
1) Build this program and Test
    1) the project should now build and run

### ESP8266 Programming ###
* [Arduino IDE](https://github.com/esp8266/Arduino)
* [NodeMCU](https://en.wikipedia.org/wiki/NodeMCU) Lua based
* [Platform I/O](https://platformio.org/platforms/espressif8266)
* OTA wireless upload from Arduino not supported
* OTA wireless upload of .ino.bin via HTTP supporte on /update url

### Programming weirdness ###
I ran into the following issues / oddities during development

* The default serial monitor prior to programming runs 74880 bps. I found that in some situations I had to go back to that speed.
* Close the serial monitor before trying to program.
* Remember to move the pgm/run jumper to the correct position and reset via pin or power.
* I had to unplug/plg the power to get to reliably switch between _program_ and _run_
* The upload speed was 115200.
* Compilation may be slow on my Windows 10 machine. Dispabling Windows Defender for the compilation folders sped up the operation
* Could never get Arduino IDE to recognize my device over the network on PC or Mac. Was forced to directly run the espota.py.


### OTA Programming ###

* Upload via web page _http://esp-8266-\<chipId\>.local/update_
* Upload via OTA direct connection, usually via python /espota.py

#### OTA Python Upload ####
I couldn't get the IDE to recognize the device and make it show up in the port menu. I could do direct upload via the command line.

These examples assume 
* the ESP8266 board is named _esp8266-2253881.local_
* the project name is _Relay-OTA-demo.ino.bin_ 
* the location of espota.py based on where they were on my personal machines
* the location of the compiled file based on my personal machines as printed out in the arduino build console

| Platform | Command |
|-----|-----|
| General |  python \<path\>/espota.py -i \<device-dns-name\> -f \<project-path>/\<project\>.ino.bin   -a \<password\> |
| Mac | python Users/joefreeman/Library/Arduino15/packages/esp8266/hardware/esp8266/2.3.0/tools/espota.py -i esp8266-2253881.local -a 2253881 -f /var/folders/j9/x_01h_yx26l0v7x1sffsvr440000gn/T/arduino_build_439217/Relay-OTA-demo.bin |
| PC | python C:\\Users\\joe\\AppData\\Local\\Arduino15\\packages\\esp8266\\hardware\\esp8266\\2.3.0\\tools\\espota.py -i esp8266-2253881.local -a 2253881 -f C:\\Users\\joe\\AppData\\Local\\Temp\\arduino_build_471516\\Relay-OTA-demo.ino.bin |

## Notes ##
Intentionally left blank.