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

//NOTE, printBuffer is broken slightly by available()
//available() places '\0' in swarm_buff in a few places, which will terminate the
//string early. 
void BUGswarm::printBuffer(){
  Serial.println(swarm_buff);
}

//available is in context to swarm messages, NOT the underlying socket
//so, if any data is available on the stream, available() will go and retrieve
//an entire packet so that it can be parsed and determined a valid JSON packet
//Be Careful!  Calling available() when data is waiting in the input buffer
//will cause swarm_buff to be cleared.  Make sure consume() has been called first!
//note that a return value of 0 is for non-payload messages - mostly presence
int BUGswarm::available(){
  if (!client.available()){
    return -1;
  }
  readMessage();
  if (swarm_buff[0] != '{')
    return -1;
  payload = (char *)memmem(swarm_buff, sizeof(swarm_buff), "\"payload\"", 9);
  sender = (char *)memmem(swarm_buff, sizeof(swarm_buff), "\"resource\"", 10);
  if (sender == NULL){
    return -1;
  }
  void * senderTail = memmem(swarm_buff, sizeof(swarm_buff), "\"},\"payload\":", 13);
  if (senderTail != NULL)
    *((char *)senderTail) = '\0';
  if (memmem(sender, strlen(sender), resource, strlen(resource)) != NULL){
    return -1;
  }
  if (payload == NULL){
    return 0;
  }
  //WARNING - the following will be broken if/when the consume messages are changed.
  void * tail = memmem(payload, strlen(payload), ",\"public\":true", 14);
  if (tail != NULL)
    *((char *)tail) = '\0';
  return strlen(payload) - 23;
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

