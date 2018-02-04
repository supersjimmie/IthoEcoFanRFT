/*
   Original Author: Klusjesman

   Tested with STK500 + ATMega328P
   GCC-AVR compiler

   Modified by supersjimmie:
   Code and libraries made compatible with Arduino and ESP8266
   Tested with Arduino IDE v1.6.5 and 1.6.9
   For ESP8266 tested with ESP8266 core for Arduino v 2.1.0 and 2.2.0 Stable
   (See https://github.com/esp8266/Arduino/ )

*/

/*
  CC11xx pins    ESP pins Arduino pins  Description
  1 - VCC        VCC      VCC           3v3
  2 - GND        GND      GND           Ground
  3 - MOSI       13=D7    Pin 11        Data input to CC11xx
  4 - SCK        14=D5    Pin 13        Clock pin
  5 - MISO/GDO1  12=D6    Pin 12        Data output from CC11xx / serial clock from CC11xx
  6 - GDO2       04=D2    Pin 2?        Serial data to CC11xx
  7 - GDO0       ?        Pin  ?        output as a symbol of receiving or sending data
  8 - CSN        15=D8    Pin 10        Chip select / (SPI_SS)
*/

#include <SPI.h>
#include "IthoCC1101.h"
#include "IthoPacket.h"
#include <Ticker.h>

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define ITHO_IRQ_PIN D2

IthoCC1101 rf;
IthoPacket packet;
Ticker ITHOticker;

//const uint8_t RFTid[] = {106, 170, 106, 101, 154, 107, 154, 86}; // my ID
const uint8_t RFTid1[] = {0x69,0x99,0x96,0x99,0x95,0x6a,0x6a,0x5a}; // my ID
const uint8_t RFTid2[] = {0x6a,0x99,0x66,0xaa,0xa9,0x66,0x6a,0xaa}; // badkamer

bool ITHOhasPacket = false;
bool ITHOhasValidPacket = false;
bool ITHOhasValidId = false;

IthoCommand RFTcommand[3] = {IthoUnknown, IthoUnknown, IthoUnknown};
byte RFTRSSI[3] = {0, 0, 0};
byte RFTcommandpos = 0;
IthoCommand RFTlastCommand = IthoLow;
IthoCommand RFTstate = IthoUnknown;
IthoCommand savedRFTstate = IthoUnknown;
bool RFTidChk[3] = {false, false, false};

const char* ssid     = "";
const char* password = "";
WiFiClient espClient;

void setupWifi()
{
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void setupRf(void)
{
  Serial.println("setup rf begin");
  rf.init();
  sendRegister();
  Serial.println("join command sent");
  pinMode(ITHO_IRQ_PIN, INPUT);
  attachInterrupt(ITHO_IRQ_PIN, ITHOinterrupt, RISING);
}

PubSubClient client(espClient);

void setupMqtt()
{
  //connect to MQTT server
  client.setServer("pi3.lan", 1883);
  client.setCallback(callback);
}


//print any message received for subscribed topic
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);

  Serial.print("] (l=");
  Serial.print(length);
  Serial.print(") ");

  for (int i=0;i<length;i++) {
    Serial.print(payload[i], HEX);
  }
  Serial.println();

  // prep send
  CC1101Packet packet;
  packet.length = length + 9;
  packet.data[0] = 170;
  packet.data[1] = 170;
  packet.data[2] = 170;
  packet.data[3] = 170;
  packet.data[4] = 170;
  packet.data[5] = 170;
  packet.data[6] = 170;				
  packet.data[7] = 171;	
  for (int i=0;i<length;i++) {
    packet.data[i+8] = payload[i];
  }

  /*
  printf ("pack %d:", packet.length);
  for (int i = 0; i < packet.length; i++) {
    printf (" %2x", packet.data[i]);
    fflush (stdout);
  }
  printf("\n");
  fflush (stdout);
  Serial.println("");
  */
  
  int delaytime = 500;
  uint8_t maxTries = 3;

  //detachInterrupt(ITHO_IRQ_PIN);
  //rf.sendPacket(packet);
  setupRf();
}


void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect, just a name to identify the client
    if (client.connect("NANO","xxxxxx","xxxxxxxxxxx")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      // client.publish("outpic","Hello World");
      // ... and resubscribe
      client.subscribe("itho/command", 0);

    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup(void) {
  Serial.begin(115200);
  delay(500);
  setupWifi();
  setupMqtt();
  delay(500);
  setupRf();
}

void loop(void) {
  // do whatever you want, check (and reset) the ITHOhasPacket flag whenever you like
  if (ITHOhasPacket) {
    showPacket();
  }
  // put your main code here, to run repeatedly:
  if (!client.connected()) {
     reconnect();
  }
  client.loop();
  //delay(100);
}

void ITHOinterrupt() {
  ITHOticker.once_ms(10, ITHOcheck);
}

void ITHOcheck() {
  if (rf.checkForNewPacket()) {
    IthoCommand cmd = rf.getLastCommand();
    if (++RFTcommandpos > 2) RFTcommandpos = 0;  // store information in next entry of ringbuffers
    RFTcommand[RFTcommandpos] = cmd;
    RFTRSSI[RFTcommandpos]    = rf.ReadRSSI();

    bool chk = rf.checkID(RFTid1) | rf.checkID(RFTid2);
    ITHOhasValidId = chk;
    RFTidChk[RFTcommandpos]   = chk;

    ITHOhasPacket = true;
    if ((cmd != IthoUnknown) && chk) {  // only act on good cmd and correct id.
      ITHOhasValidPacket = true;
    }
  }
}

void showPacket() {
  ITHOhasPacket = false;
  String id = rf.getLastIDstr();
  String msg = rf.getLastMessage2str (true);

  Serial.print(id);
  Serial.print(" -> ");
  Serial.println(msg);
  fflush(stdin);
  msg = msg.substring(10);
  msg.replace(":", "");
  int r = client.publish("itho/ventilatie", msg.c_str());
  if (r == 0) {
    Serial.println("Error publishing message");
  }
  
  if (!ITHOhasValidPacket) {
    if (ITHOhasValidId) {
      //printf("unknown command: %s\n", rf.getLastCommandStr(false).c_str());
      fflush(stdin);
    }
    return;
  }
  //printf("known command: %s\n", rf.getLastCommandStr(false).c_str());  
  printf ("\tcounter = %d\n", rf.getLastInCounter());
  fflush(stdin);

  ITHOhasValidPacket = false;
  uint8_t goodpos = findRFTlastCommand();
  if (goodpos != -1)  RFTlastCommand = RFTcommand[goodpos];
  else                RFTlastCommand = IthoUnknown;
  //show data
  Serial.print(F("RFT Current Pos: "));
  Serial.print(RFTcommandpos);
  Serial.print(F(", Good Pos: "));
  Serial.println(goodpos);
  Serial.print(F("Stored 3 commands: "));
  Serial.print(RFTcommand[0]);
  Serial.print(F(" "));
  Serial.print(RFTcommand[1]);
  Serial.print(F(" "));
  Serial.print(RFTcommand[2]);
  Serial.print(F(" / Stored 3 RSSI's:     "));
  Serial.print(RFTRSSI[0]);
  Serial.print(F(" "));
  Serial.print(RFTRSSI[1]);
  Serial.print(F(" "));
  Serial.print(RFTRSSI[2]);
  Serial.print(F(" / Stored 3 ID checks: "));
  Serial.print(RFTidChk[0]);
  Serial.print(F(" "));
  Serial.print(RFTidChk[1]);
  Serial.print(F(" "));
  Serial.print(RFTidChk[2]);
  Serial.print(F(" / Last ID: "));
  Serial.print(rf.getLastIDstr());

  Serial.print(F(" / Command = "));
  //show command
  switch (RFTlastCommand) {
    case IthoUnknown:
      Serial.print("unknown\n");
      break;
    case IthoLow:
      Serial.print("low\n");
      break;
    case IthoMedium:
      Serial.print("medium\n");
      break;
    case IthoHigh:
      Serial.print("high\n");
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
    case IthoHome1:
      Serial.print ("home1\n");
      break;
    case IthoHome2:
      Serial.print ("home2\n");
      break;
    case IthoCook:
      Serial.print ("hood\n");
      break;
    case IthoTimer:
      Serial.print ("timer\n");
      break;
    default:
      Serial.print ("-\n");
  }
}

uint8_t findRFTlastCommand() {
  if (RFTcommand[RFTcommandpos] != IthoUnknown)               return RFTcommandpos;
  if ((RFTcommandpos == 0) && (RFTcommand[2] != IthoUnknown)) return 2;
  if ((RFTcommandpos == 0) && (RFTcommand[1] != IthoUnknown)) return 1;
  if ((RFTcommandpos == 1) && (RFTcommand[0] != IthoUnknown)) return 0;
  if ((RFTcommandpos == 1) && (RFTcommand[2] != IthoUnknown)) return 2;
  if ((RFTcommandpos == 2) && (RFTcommand[1] != IthoUnknown)) return 1;
  if ((RFTcommandpos == 2) && (RFTcommand[0] != IthoUnknown)) return 0;
  return -1;
}

void sendRegister() {
  Serial.println("sending join...");
  rf.sendCommand(IthoJoin);
  Serial.println("sending join done.");
}

void sendStandbySpeed() {
  Serial.println("sending standby...");
  rf.sendCommand(IthoStandby);
  Serial.println("sending standby done.");
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

void sendHighSpeed() {
  Serial.println("sending high...");
  rf.sendCommand(IthoHigh);
  Serial.println("sending high done.");
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
