#include "Arduino.h"
#include "EthernetClient.h"
#include "IPAddress.h"
#include <avr/pgmspace.h>
#include "SwarmMessage.h"

#define SWARM_INPUT_BUFFER_SIZE       340
#define SWARM_OUTPUT_BUFFER_SIZE        120

const char produce_header[] PROGMEM = "POST /stream?swarm_id=%s&resource_id=%s HTTP/1.1\r\nHost:api.test.bugswarm.net\r\nx-bugswarmapikey:%s\r\ntransfer-encoding:chunked\r\nConnection:keep-alive\r\nContent-Type: application/json\r\n\r\n15\r\n{\"message\":\"hi mom!\"}\r\n";
const char newresource_json[] PROGMEM = "{\"name\":\"%s\",\"machine_type\":\"pc\",\"description\":\"Arduino\"}";

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
    //
    char * consume();
    //resource() - returns a pointer to the resource of the current message
    char * getSender();
    //read an entire swarm message into the buffer and parse it
    SwarmMessage fetchMessage();

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

    const IPAddress *server;
    const char *swarm;
    const char *resource;
    const char *key;
    char swarm_buff[SWARM_INPUT_BUFFER_SIZE];
    char produce_buff[SWARM_OUTPUT_BUFFER_SIZE];
    unsigned char produce_idx;
    EthernetClient client;
    char * payload;
    char * sender;
};
