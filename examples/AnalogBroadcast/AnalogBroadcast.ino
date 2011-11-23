#include <SPI.h>
#include <Ethernet.h>
#include "Streamprint.h"
#include "BUGswarm.h"
#include <avr/pgmspace.h>

byte mac[6] = { 0xDE, 0xAD, 0xBE, 0xFF, 0xFF, 0xED };
byte ip[4] = { 192,168,0,12 };
byte dns[4] = { 8,8,8,8 };
byte gw[4] = { 192,168,0,3 };
byte subnet[4] = { 255,255,0,0 };

IPAddress server(64,118,81,28); // api.test.bugswarm.net
const char * swarm_id =           "abcdefghijklmnopqrstuvwxyz1234567890abcd";
const char * resource_id =        "abcdefghijklmnopqrstuvwxyz1234567890abcd";
const char * participation_key =  "abcdefghijklmnopqrstuvwxyz1234567890abcd";
BUGswarm swarm(swarm_id, resource_id, participation_key);

const char message_template[] PROGMEM = "{\"message\": {\"payload\": {\"analog\": \[%d, %d, %d, %d, %d] }}}";
long lastConnectionTime = 0;
const int postingInterval = 1000;
char message[50];

void setup()
{
  Serial.begin(115200);
  Ethernet.begin(mac, ip, dns, gw, subnet);
  delay(1000);
  while(!swarm.connect(&server)){}
  Serialprint("connected!\n");
}

void loop()
{
  int avail = swarm.available();
  if (avail>0){
    Serialprint("%s\n",swarm.consume());
  }

  if(millis() - lastConnectionTime > postingInterval) {
    sendData();
    lastConnectionTime = millis();
  }
}

void sendData() {
  Serial.println("out");
  memset(message, '\0', sizeof(message));
  sprintf_P(message, message_template, analogRead(0), analogRead(1), analogRead(2), analogRead(3), analogRead(4));
  swarm.produce(message);
  lastConnectionTime = millis(); 
}
