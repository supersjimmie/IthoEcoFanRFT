/* 
*    Created trough the colaboration of Tech enthousiasts.
*    More info at: https://gathering.tweakers.net/forum/list_messages/1690945
*    Thanks to all involved for making this work
*/

#include <SPI.h>
#include "IthoCC1101.h"
#include "IthoPacket.h"
#include <ESP8266WiFi.h>

//////////////////////
// WiFi Definitions //
//////////////////////
 const char WiFiSSID[] = "Your-SSID";
 const char WiFiPSK[] = "Your-PSK";
// Commands to send (trough webbrowser or Domoticz)
// <ip-adress-nodemcu>/away
// <ip-adress-nodemcu>/low
// <ip-adress-nodemcu>/medium
// <ip-adress-nodemcu>/high
// <ip-adress-nodemcu>/power
// <ip-adress-nodemcu>/timer



IthoCC1101 rf;
IthoPacket packet;
WiFiServer server(80);

void setup() 
{

  Serial.begin(115200);
  delay(500);
  Serial.println("");
  Serial.println("ESP8266 remote for Itho ventilation unit");
  Serial.println("See https://gathering.tweakers.net/forum/list_messages/1690945 for more information!");
  Serial.println("Setting up CC1101 module...");
  rf.init();
  Serial.println("CC1101 transmitter is now ready!");
  Serial.println("");
  sendRegister();
  connectWiFi();
  Serial.println("");
  // Start the server
  Serial.println("Starting webserver...");
  server.begin();
  Serial.println("Webserver is now running!");
  
}

void loop() 
{
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    return;
  }

  // Read the first line of the request
  String req = client.readStringUntil('\r');
  Serial.println(req);
  client.flush();

  // Match the request
  const char* val;

  if (req.indexOf("/away") != -1) {
    val = "away";
    sendAway();
  }
  else if (req.indexOf("/low") != -1) {
    val = "low";
    sendLowSpeed();
  }
  else if (req.indexOf("/medium") != -1) {
    val = "medium";
    sendMediumSpeed();
  }
  else if (req.indexOf("/high") != -1) {
    val = "high";
    sendHighSpeed();    
  }
  else if (req.indexOf("/power") != -1) {
    val = "power";
    sendFullPower();
  }
  else if (req.indexOf("/timer") != -1) {
  val = "timer";
    sendTimer();    
  }
  client.flush();

  // Prepare the response. Start with the common header:
  String s = "HTTP/1.1 200 OK\r\n";
  s += "Content-Type: text/html\r\n\r\n";
  s += "<!DOCTYPE HTML>\r\n<html>\r\n";
  // If we're setting the LED, print out a message saying we did
  if (val != NULL)
  {
    s += "Fan is now: ";
    s += val;
  }
  else
  {
    s += "Invalid Request.<br> Try /standby, /low, /medium, /high, /power or /timer.";
  }
  s += "</html>\n";

  // Send the response to the client
  client.print(s);
  delay(1);
  Serial.println("Client disconnected");

  // The client will actually be disconnected 
  // when the function returns and 'client' object is detroyed
}

void connectWiFi()
{
  //byte ledStatus = LOW;

  // Set WiFi mode to station (as opposed to AP or AP_STA)
  WiFi.mode(WIFI_STA);
  // WiFI.begin([ssid], [passkey]) initiates a WiFI connection
  // to the stated [ssid], using the [passkey] as a WPA, WPA2,
  // or WEP passphrase.
  Serial.println();
  Serial.print("Connecting to wifi-network: ");
  Serial.print(WiFiSSID);
  WiFi.begin(WiFiSSID, WiFiPSK);

  // Use the WiFi.status() function to check if the ESP8266
  // is connected to a WiFi network.
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("I am now connected to '");
  Serial.print(WiFiSSID);
  Serial.print("' and my IP-address is: ");
  Serial.println(WiFi.localIP());
}

void sendRegister() {
  Serial.println("Sending command 'join' to Itho ventilation unit...");
   rf.sendCommand(IthoJoin);
   Serial.println("Command 'join' has been sent!");
}

void sendAway() {
   Serial.println("Sending command 'away' to Itho ventilation unit...");
   rf.sendCommand(IthoAway);
   Serial.println("Command 'away' has been sent!");
}

void sendLowSpeed() {
   Serial.println("Sending command 'low' to Itho ventilation unit...");
   rf.sendCommand(IthoLow);
   Serial.println("Command 'low' has been sent!");
}

void sendMediumSpeed() {
   Serial.println("Sending command 'medium' to Itho ventilation unit...");
   rf.sendCommand(IthoMedium);
   Serial.println("Command 'medium' has been sent!");
}

void sendHighSpeed() {
   Serial.println("Sending command 'fullspeed' to Itho ventilation unit...");
   rf.sendCommand(IthoHigh);
   Serial.println("Command 'fullspeed' has been sent!");
}

void sendFullPower() {
   Serial.println("Sending command 'power' to Itho ventilation unit...");
   rf.sendCommand(IthoFullPower);
   Serial.println("Command 'power' has been sent!");
}

void sendTimer() {
   Serial.println("Sending command 'timer' to Itho ventilation unit...");
   rf.sendCommand(IthoTimer1);
   Serial.println("Command 'timer' has been sent!");
}
