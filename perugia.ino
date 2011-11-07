#include <SPI.h>
#include <Ethernet.h>
#include "Streamprint.h"
#include <avr/pgmspace.h>

#define HEADER_TEMPLATE "POST /stream?swarm_id=%s&resource_id=%s HTTP/1.1\r\nHost:api.bugswarm.net\r\nx-bugswarmapikey:%s\r\ntransfer-encoding:chunked\r\nConnection:keep-alive\r\nContent-Type: application/json\r\n"
#define MESSAGE_TEMPLATE "{\"message\": {\"payload\": {\"analog\": \[%d, %d, %d, %d, %d] }}}"

byte mac[6] = { 0xAA, 0xAD, 0xBE, 0xFF, 0xFF, 0xED };
byte ip[4] = { 192,168,0,9 };
byte dns[4] = { 8,8,8,8 };
byte gw[4] = { 192,168,0,3 };
byte subnet[4] = { 255,255,0,0 };

const IPAddress server(107,20,250,52); // api.bugswarm.net
const char * swarm_id = "1d2430fb77212954a9dab3ce81fc17e125627d3f";
const char * resource_id = "fbd2906e052ec7fb477ad0707c35156557d62bbc";
const char * participation_key = "ad3710c9d01e6502ce85a441f1e85546a21ecb37";

const byte MAX_MESSAGE_SIZE=64;
char message[MAX_MESSAGE_SIZE];

const int HEADER_SIZE=320;
char header[HEADER_SIZE];


long lastConnectionTime = 0;
const int postingInterval = 1000;

EthernetClient client;
void setup()
{
  Serial.begin(9600);
  Ethernet.begin(mac, ip, dns, gw, subnet);
  delay(1000);

  // The reason we use PSTR() instead of a PGM_P variable is because the format string MUST be inline. Dont ask me why
  sprintf_P(header, PSTR(HEADER_TEMPLATE), swarm_id, resource_id, participation_key);
  sprintf_P(message, PSTR(MESSAGE_TEMPLATE), 0, 1, 2, 3, 4);
  
  Serial.println("connecting...");
  if (client.connect(server, 80)) {
     Streamprint(client,"%s\r\n%x\r\n%s\r\n", header, strlen(message), message);
  } else {
     Serial.println("connection failed");
  }
  delay(500);
}

void loop()
{
  // Read data as soon as its available
  if (client.available()) {
    char c = client.read();
    Serial.print(c);
  }

  // We loop as fast as possible, and update only when it matters
  if(millis() - lastConnectionTime > postingInterval) {
    sendData();
  }
}

void sendData() {
  if (client.connected()) {
    sprintf_P(message, PSTR(MESSAGE_TEMPLATE), analogRead(0), analogRead(1), analogRead(2), analogRead(3), analogRead(4));
    Streamprint(client, "%x\r\n%s\r\n", strlen(message), message);
  } else {
    Serial.println("disconnected");
    client.stop();
    for(;;) {
      ;
    }
  }
  lastConnectionTime = millis();
}
