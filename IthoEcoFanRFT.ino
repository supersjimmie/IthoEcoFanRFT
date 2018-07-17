/*
 * Original Author: Klusjesman
 *
 * Tested with STK500 + ATMega328P
 * GCC-AVR compiler
 * 
 * Modified by supersjimmie:
 * Code and libraries made compatible with Arduino and ESP8266 
 * Tested with Arduino IDE v1.6.5 and 1.6.9
 * For ESP8266 tested with ESP8266 core for Arduino v 2.1.0 and 2.2.0 Stable
 * (See https://github.com/esp8266/Arduino/ )
 * 
 */

/*
CC11xx pins    ESP pins Arduino pins  Description
1 - VCC        VCC      VCC           3v3
2 - GND        GND      GND           Ground

3 - MOSI       13=D7    Pin 11        Data input to CC11xx
4 - SCK        14=D5    Pin 13        Clock pin
5 - MISO/GDO1  12=D6    Pin 12        Data output from CC11xx / serial clock from CC11xx
6 - GDO0       ?        Pin 2?        Serial data to CC11xx
7 - GDO2       ?        Pin  ?        output as a symbol of receiving or sending data
8 - CSN        15=D8    Pin 10        Chip select / (SPI_SS)
*/

#include <SPI.h>
#include "IthoCC1101.h"
#include "IthoPacket.h"
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

ESP8266WebServer server;
char* ssid = "wifissid";
char* password = "************";

IthoCC1101 rf;
IthoPacket packet;

void setup(void) {
  Serial.begin(115200);

  // connect to wifi
	
  /* Explicitly set the ESP8266 to be a WiFi-client, otherwise, it by default,
     would try to act as both a client and an access-point and could cause
     network-issues with your other WiFi-devices on your WiFi-network. */
  WiFi.mode(WIFI_STA);
	
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // server setup
  server.on("/", usage);
  server.on("/press", pressButton);
  server.begin();
  Serial.println("Server started");
  

  
  delay(500);
  //Serial.println("setup begin");
  rf.init();
  //Serial.println("setup done");
  sendRegister();
  //Serial.println("join command sent");
}

void loop(void) {
  server.handleClient();

  return;
  
	//set CC1101 registers
	rf.initReceive();
	Serial.print("start\n");
	sei();

	while (1==1) {
		if (rf.checkForNewPacket()) {
			packet = rf.getLastPacket();
			//show counter
			Serial.print("counter=");
			Serial.print(packet.counter);
			Serial.print(", ");
			//show command
			switch (packet.command) {
        case IthoUnknown:
          Serial.print("unknown\n");
          break;
        case IthoLow:
          Serial.print("low\n");
          break;
        case IthoMedium:
          Serial.print("medium\n");
          break;
        case IthoFull:
          Serial.print("full\n");
          break;
        case IthoTimer1:
          Serial.print("timer1\n");
          break;
        case IthoTimer2:
          Serial.print("timer2\n");
          break;
        case IthoTimer3:
          Serial.print("timer3\n");
          break;
        case IthoJoin:
          Serial.print("join\n");
          break;
        case IthoLeave:
          Serial.print("leave\n");
          break;
			} // switch (recv) command
		} // checkfornewpacket
	yield();
	} // while 1==1
} // outer loop

void usage() {
  server.send(200, "text/plain", "/press?button=low");
}

void pressButton() {
   if (!server.hasArg("button")) {
       return returnFail("Please provide a button, e.g. ?button=low");
   }
   
   String button = server.arg("button");
   Serial.print("Pressing button: ");
   Serial.println(button);

   if (button == "low") { 
       sendLowSpeed();
   } else if (button == "medium") { 
       sendMediumSpeed();
   } else if (button == "high") { 
       sendFullSpeed();
   } else if (button == "timer") {     
       sendTimer();
   } else {
      return returnFail("Unknown button. Buttons are: low, medium, high, timer");
   }
  
   returnJson("\"button\": \"" + button + "\"");
}

void returnJson(String msg)
{
    server.send(200, "application/json", "{\"success\": true, "+  msg + "}");
}

void returnFail(String msg)
{
    server.sendHeader("Connection", "close");
    server.send(500, "application/json", "{\"success\": false, \"message\": \""+ msg + "\"}");
}

void sendRegister() {
   Serial.println("sending join...");
   rf.sendCommand(IthoJoin);
   Serial.println("sending join done.");
}

void sendLowSpeed() {
   Serial.println("sending low...");
   rf.sendCommand(IthoLow);
   Serial.println("sending low done.");
}

void sendMediumSpeed() {
   Serial.println("sending medium...");
   rf.sendCommand(IthoMedium);
   Serial.println("sending medium done.");
}

void sendFullSpeed() {
   Serial.println("sending FullSpeed...");
   rf.sendCommand(IthoFull);
   Serial.println("sending FullSpeed done.");
}

void sendTimer() {
   Serial.println("sending timer...");
   rf.sendCommand(IthoTimer1);
   Serial.println("sending timer done.");
}

