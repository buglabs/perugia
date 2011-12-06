/*
BUGswarm.cpp
Author: Andrew Tergis 2011

This is a slightly hacky / abnormal inheritance of the arduino Stream and Print
classes.  Our goal is to balance abstraction (to ease adoption with arduino
progammers) and RAM consumption/code size.  wrapJSONForMe and rawReadMode will
change the functionality of the overridden methods (namely read() write() and
peek()).  

Write() is not too bad of a stretch from normal Print classes.  Write() will
buffer writes to a swarm object until '\n' is recieved, in which case it will
prefix the correct HTML Chunked encoding format before actually transmitting 
the written data.  If wrapJSONForMe is enabled, write() will also wrap a basic
JSON payload around the data for added convinience ({"data":"<message>"}). 
This is necessary, since the entire message *must* be written at once in order
for it to be a valid chunked encoding payload.

NOTE that write() cannot process binary (non-ascii) data.  Binary data is too
application-specific to enforce.  We are also not agressively checking JSON or
escaping strings for the user: they must do that themselves.

If rawReadMode = true, read() works largely as a wrapper to the underlying
client object - mostly transparent.  However, if rawReadMode = false (the
default behavior), read() becomes stateful.  Read() will only return valid
data ( greater than -1 ) if valid data was recieved from the socket AND that
valid data is not filtered out.  Only valid swarm "message" messages will be
returned to the user if they originate from a different resource ID than the
one used by the swarm class.  If wrapJSONForMe is enabled, messages that do
not contain the specified format will be discarded.

To avoid blocking program execution, read() will only process one incoming
byte every time it is run.  That means that application code must continually
call read(), even if read() continues to return -1.  This gives the user
greater control over blocking.  If the user wants to wait until a valid
message is recieved, simply continue peek()-ing until a value greater than -1
is returned.  

See the code within /examples/ for more detailed operation examples.
*/

#include "Arduino.h"
#include <string.h>
#include <avr/pgmspace.h>
#include "BUGswarm.h"
#include "Streamprint.h"

#define SWARM_PRODUCE_THRESHOLD  10

BUGswarm::BUGswarm(const char *swarm_id, const char *resource_id, const char *participation_key){
  // store the necessary configuration string references
  swarm = swarm_id;
  resource = resource_id;
  key = participation_key;
  // set default swarm.write and swarm.read behavior:
  wrapJSONForMe(true);
  rawReadMode = false;
  // intialize a few variables
  read_counter = 0;
  read_idx = -1;
  peekbyte = -1;
}

void BUGswarm::wrapJSONForMe(boolean value){
  // When wrapJSONForMe is updated, update the header and tail references
  if (value){
      message_header = message_header_JSON;
      message_tail = message_tail_JSON;
  } else {
      message_header = message_header_basic;
      message_tail = message_tail_basic;
  }
  wrapJSON = value;
  // Initialize the swarm_buff with the header so we don't need to append it
  memset(swarm_buff, '\0', SWARM_BUFFER_SIZE);
  strcpy_P(swarm_buff, message_header);
  produce_idx = strlen(swarm_buff);
}

boolean BUGswarm::connect(const IPAddress *serv){
  server = serv;
  // Zero out and load the produce_header into swarm_buff
  // produce header needs to be sent within one client.print(), so that it is
  // contained within one TCP packet
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
  // If we encounter a \n (user finished their message)
  // OR If we run out of space in the buffer for the message_tail
  if ((data == '\n')||(produce_idx > SWARM_BUFFER_SIZE-strlen(message_tail)-1))
  {
    // Add message_tail to the message
    strcat_P(swarm_buff, message_tail);
    // Produce it to the swarm
    produce(swarm_buff);
    // Prepare the swarm_buff for the next message by appending the header
    memset(swarm_buff, '\0', SWARM_BUFFER_SIZE);
    strcpy_P(swarm_buff, message_header);
    produce_idx = strlen(swarm_buff);
  }
  // We will only accept bytes that are valid ASCII-text, no control characters
  // Any binary data will need to be encapsulated in a JSON-tolerant format
  // We are not checking if the data is escaped correctly, or JSON formatting
  else if ((data > 0x1F)&&(data < 0x80)){
    swarm_buff[produce_idx++] = data;
  }
}

int BUGswarm::read(){
  // If we have recently peek()'d and it was valid data
  if (peekbyte != -1){
    char temp = peekbyte;
    // Clear the peekbyte so this won't happen next call
    peekbyte = -1;
    // Send the peek()'d byte
    return temp;
  }  
  if (rawReadMode)
    return client.read();

  // Actually read the byte from the socket
  char c = client.read();
  switch (read_state){
  // This state is used to read data when wrapJSONForMe is false
  // This will retrieve one JSON object from the stream, allowing the object
  // to nest multiple bracketed objects (other JSON objects, arrays, etc).
  // When we are done reading, return to READ_STATE_LOOKING
  case READ_STATE_PAYLOAD:
    // Increment the counter if we hit a bracket, keep track of nesting
    if (c == '{'){
      read_counter++;
    }
    // When the last bracket has closed (the end of the JSON object)
    if (read_counter == 0){
      // Skip the rest of the message
      readUntilNewline();
      // Reset state machine for the next read()
      read_state = READ_STATE_LOOKING;
      // We return \n to signal the end of the message, even though \n was
      // technically never received.
      return '\n';
    }
    // Decrement the counter if we hit a bracket, keep track of nesting
    if (c == '}'){
      read_counter--;  
    }    
    return c;

  // This state will read data when wrapJSONForMe is true
  // This will retrieve one string, terminated in a double quote
  // but we will ignore escaped quotes
  // When we are done reading, return to READ_STATE_LOOKING
  case READ_STATE_WRAPPED:
    // If we see a terminating doublequote, that has NOT been escaped
    if ((c == '"')&&(last_byte != '\\')){
      // Skip the rest of the message
      readUntilNewline();
      // Reset state machine for the next read()
      read_state = READ_STATE_LOOKING;
      // We return \n to signal the end of the message, even though \n was
      // technically never received.      
      return '\n';
    }
    last_byte = c;
    return c;

  // This is the default state that filters the stream for certian substrings
  case READ_STATE_LOOKING:
    // If we hit a newline, make sure to reset the index for the next message
    if (c == '\n'){
      // Setting this to -1 flags that we are waiting for an opening quote
      read_idx = -1;
      return -1;
    }
    // If we are waiting for an opening quote, and we get one
    if ((c == '"') && (read_idx == -1)){
      // Flag that next byte we start reading data
      read_idx = 0;
      // Clear out the buffer for the new data.
      memset(read_buff, '\0', 10);
    }
    // If we are expecting new data (read_idx != -1)
    // AND we have room in the buffer for the data
    if ((read_idx >= 0)&&(read_idx < sizeof(read_buff)))
      read_buff[read_idx++] = c;
    // If we see a quote after having started reading
    if ((c == '"') && (read_idx > 2)){
      // Next byte, we will prepare to read again
      read_idx = -1;
      // If the read data matches the signature for wrapped data
      if ((wrapJSON)&&(strcmp_P(read_buff, payload_indicator_basic) == 0)){ 
        read_counter = 0;
        // Next byte, start reading data to the user!
        read_state = READ_STATE_WRAPPED;
        // read until we see the start of wrapped data, a doublequote
        client.find("\"",1);
      // if the read data matches the signature for unwrapped data
      }else if ((!wrapJSON)&&
                      (strcmp_P(read_buff, payload_indicator_JSON) == 0)){ 
        read_counter = 0;
        // Next byte, start reading data to the user!        
        read_state = READ_STATE_PAYLOAD;
        // Read a single byte - this assumes the next byte after is a {
        // TODO - peek until we see a {        
        client.read();
      // if the read data contains a resource
      } else if (strcmp_P(read_buff, resource_indicator) == 0){
        read_counter = 0;
        // Next byte, start comparing the resource to ours
        read_state = READ_STATE_SENDER;
        // Read until we see the start of the resource, a doublequote
        client.find("\"",1);
      }
    }
    break;

  // This state compares a resource (as it's arriving on the stream) to ours
  case READ_STATE_SENDER:
    if (read_counter >= sizeof(resource)){
      // if we get through the entire resource string without any differences,
      // this is one of our messages - SKIP IT.
      readUntilNewline();
      // next byte, resume looking for valid messages
      read_state = READ_STATE_LOOKING;
    }
    if ((c == '"')||(c != resource[read_counter])){
      // if resource varies (or quote terminates), this message is not from us!
      // returning to READ_STATE_LOOKING without readUntilNewline() will
      // let us actually read the message.
      read_state = READ_STATE_LOOKING;
      read_idx = -1;
      // read until we have terminated the opening dobulequote, so that 
      // READ_STATE_LOOKING doesn't get out of sync
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
  // First check if socket has data (fast)
  if (!client.available())
    return -1;
  // If we are in rawReadMode, relay the client peek() method
  if (rawReadMode)
    return client.peek();
  // Only read() if the previous peek was non-data
  if (peekbyte == -1)
    peekbyte = read();
  return peekbyte;
}

void BUGswarm::flush(){
  // Clear swarm_buff and preload the message_header
  memset(swarm_buff, '\0', SWARM_BUFFER_SIZE);
  strcpy_P(swarm_buff, message_header);
  produce_idx = strlen(swarm_buff); 
}

boolean BUGswarm::getNewMessage(char * buff, int len){
  // If we actually have data that is ready to be read
  if (peek() != -1){
    // Clear the target buffer
    memset(buff, '\0', len);
    // Read data right off the socket into the buffer
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
  // Keep reading until we see a '\n'
  while(c != '\n')
    c = client.read();
}

int BUGswarm::available(){
  // First check if data is available on socket
  if (!client.available()){
    return 0;
  }
  // Then check if that data has been filtered or not
  if (peek() == -1)
    return 0;
  return 1;
}

void BUGswarm::printMessage(){
  readMessage();
  printBuffer();
}

void BUGswarm::readMessage(){
  if (client.available()) {
    // Clear out swarm_buff (this might break write())
    memset(swarm_buff, '\0', sizeof(swarm_buff));
    int idx = 0;
    char c = client.read();
    // Keep reading until we hit a '\n', or we run out of space.
    while ((c != '\n')&&(idx < sizeof(swarm_buff)-1)){
      // Don't store '\r' or '\n' in the buffer.
      if ((c != '\r')&&(c != '\n')){
        swarm_buff[idx++] = c;
      }
      c = client.read();
    }
  }
}

void BUGswarm::produce(char * message){
  if (client.connected()){
    // Print the length of the message (in hex), a newline, and the message
    // This is the HTML Chunked encoding format
    // TODO - find a way to use swarm_buff for this 
    //    Maybe preallocate room for the size and \r\n in the header
    Streamprint(client, "%x\r\n%s\r\n", strlen(message), message);
  } else {
    Serialprint("Disconnected, reconnecting...\n");
    // Continually attempt to reconnect
    // This will totally block until we reconnect.
    do {
      client.stop();
      connect(server);
    } while (!client.connected());
  }
}

