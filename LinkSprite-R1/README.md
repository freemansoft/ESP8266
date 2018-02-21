Simple example that provides web based interface for relay control. This was bilt for the Link Sprite LinkNode R1 single relay board found at Microcenter (as of 2/2018).  It is shoudl be easily modified for the R4 quad relay board.

1) Attempts to find network that it is already configured for.  Attaches to the network fi fouind.
    * Boots into a self configuring Access Point portal if no recognized network found.  
    * Reboots after configuration onto recognized network.
1) Brings up internal web server on _esp8266-linksprite.local_ after joining network
    * GET form to turn on/off relay on /
    * GET /relayon
    * GET /relayoff

### Security ###

This device is totally **insecure** with no passwords or or DDOS protection

### Hardware ###

* ESP-87266EX
* 5V pDC power jack
* 1 5V DC port for power/configuration (?)
* 1 Digital/IO Relay
* 6 Digital I/O pins
* 1 Analog Input pin

### Programming ###

* [Arduino IDE](https://github.com/esp8266/Arduino)
* [NodeMCU] (https://en.wikipedia.org/wiki/NodeMCU) Lua based
* [Platform I/O](https://platformio.org/platforms/espressif8266)
* OTA wireless upload


### Shortcomings ###

1) Does not work on WPA2 Enterprise wireless networks.
1) There is no way to _forget_ a configured network to bring back the AP
