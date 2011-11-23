#include "Arduino.h"
#include <avr/pgmspace.h>
#include "BUGswarm.h"
#include "Streamprint.h"

#define SWARM_PRODUCE_THRESHOLD  10

BUGswarm::BUGswarm(const char *swarm_id, const char *resource_id, const char *participation_key){
  swarm = swarm_id;
  resource = resource_id;
  key = participation_key;
}

boolean BUGswarm::connect(const IPAddress *serv){
  server = serv;
  memset(swarm_buff, '\0', sizeof(swarm_buff));
  sprintf_P(swarm_buff, produce_header, swarm, resource, key);
  Serial.println("connecting...");
  if (client.connect(*server, 80)) {
     client.print(swarm_buff);
     return true;
  } else {
     Serial.println("connection failed");
     return false;
  }
}

void BUGswarm::printData(){
  // Read data as soon as its available
  if (client.available()) {
    memset(swarm_buff, '\0', sizeof(swarm_buff));
    int idx = 0;
    char c = client.read();
    while ((c != '\n')&&(idx < sizeof(swarm_buff)-1)){
      if ((c != '\r')&&(c != '\n')){
        swarm_buff[idx++] = c;
      }
      c = client.read();
    }
    //Only print what might be valid JSON, please!
    if (swarm_buff[0] == '{')
      Serial.println(swarm_buff);
  }
}

void BUGswarm::produce(char * message){
  Streamprint(client, "%x\r\n%s\r\n", strlen(message), message);
}

