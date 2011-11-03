#include <SPI.h>
#include <Ethernet.h>

byte mac[6] = { 0xAA, 0xAD, 0xBE, 0xFF, 0xFF, 0xED };
byte ip[4] = { 192,168,0,9 };
byte dns[4] = { 8,8,8,8 };
byte gw[4] = { 192,168,0,3 };
byte subnet[4] = { 255,255,0,0 };

const IPAddress server(107,20,250,52); // api.bugswarm.net

const char * message = "21\r\n{\"message\": {\"payload\": {\"x\":1}}}";

EthernetClient client;

void setup()
{
  Serial.begin(9600);
  Serial.println("OHAIMAHK");
  Ethernet.begin(mac, ip, dns, gw, subnet);
  delay(1000);

 Serial.println("connecting...");
  if (client.connect(server, 80)) {
     Serial.println("connected");
     client.println("POST /stream?swarm_id=1d2430fb77212954a9dab3ce81fc17e125627d3f&resource_id=fbd2906e052ec7fb477ad0707c35156557d62bbc HTTP/1.1\r\nHost:api.bugswarm.net\r\nx-bugswarmapikey:ad3710c9d01e6502ce85a441f1e85546a21ecb37\r\ntransfer-encoding:chunked\r\nConnection:keep-alive\r\nContent-Type: application/json\r\n\r\n21\r\n{\"message\": {\"payload\": {\"x\":1}}}");
  } else {
     Serial.println("connection failed");
  }
}

void loop()
{
  Serial.print('.');

  if (client.connected()) {
    client.println(message);
  } else {
    Serial.println("disconnected");
    client.stop();
    for(;;) {
      ;
    }
  }
  delay(1000);
}
