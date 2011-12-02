#include "Arduino.h"
#include "EthernetClient.h"
#include "IPAddress.h"
#include <avr/pgmspace.h>

#define SWARM_BUFFER_SIZE       340

#define READ_STATE_LOOKING      0x0
#define READ_STATE_PAYLOAD      0x1
#define READ_STATE_SENDER       0x2
#define READ_STATE_WRAPPED      0x3

//const char produce_header[] PROGMEM = "POST /stream?swarm_id=%s&resource_id=%s HTTP/1.1\r\nHost:api.test.bugswarm.net\r\nx-bugswarmapikey:%s\r\ntransfer-encoding:chunked\r\nConnection:keep-alive\r\nContent-Type: application/json\r\n\r\n15\r\n{\"message\":\"hi mom!\"}\r\n";
const char produce_header[] PROGMEM = "POST /stream?swarm_id=%s&resource_id=%s HTTP/1.1\r\nHost:api.test.bugswarm.net\r\nx-bugswarmapikey:%s\r\ntransfer-encoding:chunked\r\nConnection:keep-alive\r\nContent-Type: application/json\r\n\r\n1\r\n\n\r\n";
const char newresource_json[] PROGMEM = "{\"name\":\"%s\",\"machine_type\":\"pc\",\"description\":\"Arduino\"}";
const char message_header_JSON[] PROGMEM = "{\"message\": {\"payload\": {\"data\":\"";
const char message_header_basic[] PROGMEM = "{\"message\": {\"payload\":";
const char message_tail_JSON[] PROGMEM = "\"}}}";
const char message_tail_basic[] PROGMEM = "}}";
const char payload_indicator_JSON[] PROGMEM = "\"payload\"";    
const char payload_indicator_basic[] PROGMEM = "\"data\"";
const char resource_indicator[] PROGMEM = "\"resource\"";

class BUGswarm : public Stream {
  public:
    BUGswarm(const char *swarm_id, const char *resource_id, const char *participation_key);
    //Connect() - opens a socket to a given address, and sends a header to begin producing.
    boolean connect(const IPAddress *serv);
    //produce - will send a buffersworth of data to the swarm as is - make sure its valid JSON!
    void produce(char * message);
    //sendData() - soon to be deprecated - a sample application that produces the state of the analog pins    
    void sendData();
    //available() - returns 1 if valid JSON is ready to be read, 0 if no valid json is on the line
    //typical usage - wait until swarm.available(), then swarm.fetchMessage()
    int available();
    //read an entire swarm message into the buffer and print it
    void printMessage();
    //when using any print or stream related functions, this changes the level of wrapping
    //true - all JSON is abstracted away from the user - can send simple strings (quotes double escaped!)
    //false - user must provide swarm payload: contents - needs to be valid JSON '{"data":"things!"}'
    void wrapJSONForMe(boolean value);
    //enable or disable raw read mode - if true, raw messages will be returned by read() calls
    //if false, read will only return swarm payloads 
    void setRawReadMode(boolean value){rawReadMode = value;};

    boolean getNewMessage(char * buff, int len);

    size_t write(uint8_t data);
    int read();
    int peek();
    void flush();
    
    //variable indicating if the last message was private
    boolean priv_message;
    
    
  private:
    void readMessage();
    void parseMessage();
    void readUntilNewline();
    void printBuffer();

    EthernetClient client;
    char swarm_buff[SWARM_BUFFER_SIZE];

    //general object settings
    const IPAddress *server;
    const char *swarm;
    const char *resource;
    const char *key;
    boolean read_payload;
    const char * message_header PROGMEM;
    const char * message_tail PROGMEM;
    const char * payload_indicator PROGMEM;
    boolean rawReadMode;
    boolean wrapJSON;

    //for write()
    unsigned char produce_idx;

    //For read()
    char read_counter;
    int read_idx;
    char read_buff[10];
    int readCountdown;
    int read_state;
    int peekbyte;
    char last_byte;
};
