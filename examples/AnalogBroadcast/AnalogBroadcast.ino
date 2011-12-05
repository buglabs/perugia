/*
AnalogBroadcast

A simple swarm demonstration of BUGswarm, which will produce analog
values from pins A0-A5 every second.

This uses a mid-level interface to swarm - abstracted from the raw socket messages,
but not fully abstracted from JSON (you need to println() JSON in a particular format)

BEFORE UPLOADING make sure to create a resource and add it to a swarm.  Then fill in
the swarm_id, resource_id and participation_key variables.  Also make sure that the
networking variables (mac, ip, gw, subnet) are valid.  Finally, uncomment the server
variable for the swarm server you wish to use (ping the server to make sure that
the IP address is still valid!)

The serial monitor will indicate what data is being transmitted, as well as any
swarm messages on the line.

*/

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
//IPAddress server(107,20,250,52); // api.bugswarm.net
const char * swarm_id =           "abcdefghijklmnopqrstuvwxyz1234567890abcd";
const char * resource_id =        "abcdefghijklmnopqrstuvwxyz1234567890abcd";
const char * participation_key =  "abcdefghijklmnopqrstuvwxyz1234567890abcd";
BUGswarm swarm(swarm_id, resource_id, participation_key);

long lastConnectionTime = 0;
const int postingInterval = 1000;
char message[50];

void setup()
{
  Serial.begin(115200);
  Ethernet.begin(mac, ip, dns, gw, subnet);
  delay(1000);
  //keep looping until we successfully connect to the swarm server
  while(!swarm.connect(&server)){}
  Serial.println("connected!\n");
  //This enables us to send our own JSON payload
  swarm.wrapJSONForMe(false);
  //This filters out presence and messages from ourselves
  swarm.setRawReadMode(false);
}

void loop()
{
  //Check for a new message.  Print it if successful
  if (swarm.getNewMessage(message, sizeof(message))){
    Serial.print("Got message: ");
    Serial.println(message);
  }

  //If postingInterval milliseconds have passed, run sendData()
  if(millis() - lastConnectionTime > postingInterval) {
    sendData();
    lastConnectionTime = millis();
  }
}

void sendData() {
  //This will clear (zero out) our "message" buffer
  memset(message, '\0', sizeof(message));
  //Write the message into the buffer - note that this must be valid JSON, since wrapJSONForMe was set to False
  sprintf(message, "{\"analog\": \[%d, %d, %d, %d, %d] }", analogRead(0), analogRead(1), analogRead(2), analogRead(3), analogRead(4));
  Serial.print("sending ");
  Serial.println(message);
  //This will produce the message to our swarm
  swarm.println(message);
}
