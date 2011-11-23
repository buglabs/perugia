#include "Arduino.h"
#include <string.h>
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
  Serialprint("connecting...");
  if (client.connect(*server, 80)) {
     client.print(swarm_buff);
     return true;
  } else {
     Serialprint("connection failed\n");
     return false;
  }
}

void BUGswarm::printBuffer(){
  Serial.println(swarm_buff);
}

//There is a race condition somewhere within...  Try removing the printBuffer(), see how that goes for ya
//crap, still happens occasionally with the printBuffer...
int BUGswarm::available(){
  if (!client.available()){
    return -1;
  }
  readMessage();
  if (swarm_buff[0] != '{')
    return -1;
  printBuffer();
  payload = (char *)memmem(swarm_buff, sizeof(swarm_buff), "\"payload\"", 9);
  sender = (char *)memmem(swarm_buff, sizeof(swarm_buff), "\"resource\"", 10);
  if (resource == NULL){
    return -1;
  }
  if (memmem(sender, strlen(sender), resource, strlen(resource)) != NULL){
    return -1;
  }
  if (payload == NULL){
    return 0;
  }
  //TODO - The next line should pattern match for ", and not \",
  //Find the next instance of ", after payload+11, and zero it out (the " specifically)
  *((char *)memchr(payload+11, '"', strlen(payload+11))) = '\0';
  return strlen(payload) - 11;
}

char * BUGswarm::consume(){
  if (payload == NULL)
    return NULL;
  return payload+11;
}

char * BUGswarm::getSender(){
  if (sender == NULL)
    return NULL;
  return sender+12;
}

void BUGswarm::printData(){
  if (!client.available())
    return;
  readMessage();
  if (swarm_buff[0] == '{')
    Serial.println(swarm_buff);
}

void BUGswarm::readMessage(){
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
  }
}

void BUGswarm::produce(char * message){
  Streamprint(client, "%x\r\n%s\r\n", strlen(message), message);
}

