#include "Arduino.h"
#include <string.h>
#include <avr/pgmspace.h>
#include "BUGswarm.h"
#include "SwarmMessage.h"
#include "Streamprint.h"

#define SWARM_PRODUCE_THRESHOLD  10

BUGswarm::BUGswarm(const char *swarm_id, const char *resource_id, const char *participation_key){
  swarm = swarm_id;
  resource = resource_id;
  key = participation_key;
  produce_idx = 0;
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

size_t BUGswarm::write(uint8_t data){
  if ((data == '\n')||(produce_idx > sizeof(produce_buff)-2)){
    produce(produce_buff);
    memset(produce_buff, '\0', sizeof(produce_buff));
    produce_idx = 0;
  }
  else {
    produce_buff[produce_idx++] = data;
  }
}

int BUGswarm::read(){

}
int BUGswarm::peek(){

}
void BUGswarm::flush(){

}

//NOTE, printBuffer is broken slightly by available()
//available() places '\0' in swarm_buff in a few places, which will terminate the
//string early. 
void BUGswarm::printBuffer(){
  Serial.println(swarm_buff);
}

void BUGswarm::readUntilNewline(){
  char c = client.read();
  while(c != '\n')
    c = client.read();
}

//available detects valid JSON, NOT raw characters available on the socket
//if users want to parse swarm messages directly, they can still use this
int BUGswarm::available(){
  if (!client.available()){
    return 0;
  }
  if (client.peek() != '{'){
    //disregard any invalid JSON forthwith
    readUntilNewline();
    return 0;
  }
  return 1;
}

SwarmMessage BUGswarm::fetchMessage(){
  readMessage();
  //printBuffer();
  return SwarmMessage(swarm_buff);
}

void BUGswarm::parseMessage(){
  /*payload = (char *)memmem(swarm_buff, sizeof(swarm_buff), "\"payload\"", 9);
  sender = (char *)memmem(swarm_buff, sizeof(swarm_buff), "\"resource\"", 10);
  if (sender == NULL){
    return -1;
  }
  if (memmem(sender, strlen(sender), resource, strlen(resource)) != NULL){
    return -1;
  }
  void * senderTail = *((char *)memmem(swarm_buff, sizeof(swarm_buff), "\"},\"payload\":", 13)) = '\0';
  if (senderTail != NULL)
    *((char *)senderTail) = '\0';
  if (payload == NULL){
    return 0;
  }
  if (memmem(payload, strlen(payload), ",\"public\":true", 14) != NULL)
    priv_message = false;
  else
    priv_message = true;
  //WARNING - the following will be broken if/when the consume messages are changed.
  void * tail = memmem(payload, strlen(payload), ",\"public\":", 10);
  if (tail != NULL)
    *((char *)tail) = '\0';
  return strlen(payload) - 23;*/
}

//NOTE - available() MUST be called before calling consume
//and data must be copied out of consume() before calling available() again!
//calling available() will clear the memory returned by consume()!
char * BUGswarm::consume(){
  if (payload == NULL)
    return NULL;
  return payload+10;
}

//NOTE - available() MUST be called before calling consume
//and data must be copied out of consume() before calling available() again!
//calling available() will clear the memory returned by consume()!
char * BUGswarm::getSender(){
  if (sender == NULL)
    return NULL;
  return sender+12;
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
  if (client.connected()){
    Streamprint(client, "%x\r\n%s\r\n", strlen(message), message);
  } else {
    Serialprint("Disconnected, reconnecting...\n");
    do {
      client.stop();
      connect(server);
    } while (!client.connected());
  }
}

