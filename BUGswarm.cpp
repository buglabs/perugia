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
  //set default swarm.print behavior:
  wrapJSONForMe(true);
  rawReadMode = false;
  read_counter = 0;
  read_idx = -1;
  peekbyte = -1;
}

void BUGswarm::wrapJSONForMe(boolean value){
  if (value){
      message_header = message_header_JSON;
      message_tail = message_tail_JSON;
  } else {
      message_header = message_header_basic;
      message_tail = message_tail_basic;
  }
  wrapJSON = value;
  memset(swarm_buff, '\0', SWARM_BUFFER_SIZE);
  strcpy_P(swarm_buff, message_header);
  produce_idx = strlen(swarm_buff);
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
  if ((data == '\n')||(produce_idx > SWARM_BUFFER_SIZE-strlen(message_tail)-1)){
    strcat_P(swarm_buff, message_tail);
    produce(swarm_buff);
    memset(swarm_buff, '\0', SWARM_BUFFER_SIZE);
    strcpy_P(swarm_buff, message_header);
    produce_idx = strlen(swarm_buff);
  }
  //Strict enforcement of valid text-ascii.  You can still shoot yourself in the
  //foot with JSON compliance, though.
  else if ((data > 0x1F)&&(data < 0x80)){
    swarm_buff[produce_idx++] = data;
  }
}

int BUGswarm::read(){
  if (peekbyte != -1){
    char temp = peekbyte;
    peekbyte = -1;
    return temp;
  }
  if (rawReadMode)
    return client.read();
  //block until available
  while (!available()) {}
  char c = client.read();
  //Serialprint("%i%c", read_state, c);
  if (readCountdown > 0){
    readCountdown--;
    return -1;
  }
  switch (read_state){
  case READ_STATE_PAYLOAD:
    if (c == '{'){
      read_counter++;
    }
    if (read_counter == 0){
      readUntilNewline();
      read_state = READ_STATE_LOOKING;
      return '\n';
    }
    if (c == '}'){
      read_counter--;  
    }    
    return c;

  case READ_STATE_WRAPPED:
    if ((c == '"')&&(last_byte != '\\')){
      readUntilNewline();
      read_state = READ_STATE_LOOKING;
      return '\n';
    }
    last_byte = c;
    return c;

  case READ_STATE_LOOKING:
    if (c == '\n'){
      read_idx = -1;
      return -1;
    }
    if ((c == '"') && (read_idx == -1)){
      read_idx = 0;
      memset(read_buff, '\0', 10);
    }
    if ((read_idx >= 0)&&(read_idx < sizeof(read_buff)))
      read_buff[read_idx++] = c;
    if ((c == '"') && (read_idx > 2)){
      //Serialprint(" %s ",read_buff);
      read_idx = -1;
      if ((wrapJSON)&&(strcmp_P(read_buff, payload_indicator_basic) == 0)){ //check if this is a "data":
        read_counter = 0;
        read_state = READ_STATE_WRAPPED;
        client.find("\"",1);
      }else if ((!wrapJSON)&&(strcmp_P(read_buff, payload_indicator_JSON) == 0)){ //check if this is a "payload":
        read_counter = 0;
        read_state = READ_STATE_PAYLOAD;
        client.read();
      } else if (strcmp_P(read_buff, resource_indicator) == 0){
        read_counter = 0;
        read_state = READ_STATE_SENDER;
        client.find("\"",1);
      }
    }
    break;

  case READ_STATE_SENDER:
    if (read_counter >= sizeof(resource)){
      //if we get through the entire resource string without any differences,
      //this is one of our messages - SKIP IT.
      //Serialprint("Skip!\n");
      readUntilNewline();
      read_state = READ_STATE_LOOKING;
    }
    if ((c == '"')||(c != resource[read_counter])){
      //if resource varies (or quote terminates), this message is not from us!
      //Serialprint("Don't Skip!\n");
      read_state = READ_STATE_LOOKING;
      read_idx = -1;
      //read until the next ", so that we don't confuse the poor READ_STATE_LOOKING
      while (c != '"')
        c = client.read();
      c = client.read();
    }
    read_counter++;
    break;
  }
  return -1;
}
int BUGswarm::peek(){
  if (!available())
    return -1;
  peekbyte = read();
  return peekbyte;
}
void BUGswarm::flush(){

}

boolean BUGswarm::getNewMessage(char * buff, int len){
  if (peek() != -1){
    memset(buff, '\0', len);
    readBytesUntil('\n', buff, len-1);
    return true;
  }
  return false;
}

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
  //if (client.peek() != '{'){
  //  //disregard any invalid JSON forthwith
  //  readUntilNewline();
  //  return 0;
  //}
  return 1;
}

void BUGswarm::printMessage(){
  readMessage();
  printBuffer();
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

