#include "Arduino.h"
#include "EthernetClient.h"
#include "IPAddress.h"
#include <avr/pgmspace.h>

#define SWARM_BUFFER_SIZE       340

const char produce_header[] PROGMEM = "POST /stream?swarm_id=%s&resource_id=%s HTTP/1.1\r\nHost:api.test.bugswarm.net\r\nx-bugswarmapikey:%s\r\ntransfer-encoding:chunked\r\nConnection:keep-alive\r\nContent-Type: application/json\r\n\r\n15\r\n{\"message\":\"hi mom!\"}\r\n";
const char newresource_json[] PROGMEM = "{\"name\":\"%s\",\"machine_type\":\"pc\",\"description\":\"Arduino\"}";

class BUGswarm {
  public:
    BUGswarm(const char *swarm_id, const char *resource_id, const char *participation_key);
    //Connect() - opens a socket to a given address, and sends a header to begin producing.
    boolean connect(const IPAddress *serv);
    //loop() - will be needed to process incoming swarm messages
    void printData();
    //produce - will send a buffersworth of data to the swarm as is - make sure its valid JSON!
    void produce(char * message);
    //sendData() - soon to be deprecated - a sample application that produces the state of the analog pins    
    void sendData();
    //available() - returns number of bytes remaining
    int available();
    //consume() - returns a pointer to the message retrieved by swarm
    //check available() first!  Otherwise this will block until we get a valid message (not from us)
    char * consume();
    //resource() - returns a pointer to the resource of the current message
    char * getSender();
    //print out the message buffer without getting a new packet
    void printBuffer();
  private:
    void readMessage();

    const IPAddress *server;
    const char *swarm;
    const char *resource;
    const char *key;
    char swarm_buff[SWARM_BUFFER_SIZE];
    EthernetClient client;
    char * payload;
    char * sender;
};


