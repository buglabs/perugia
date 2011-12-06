/*
BUGswarm.h
Author: Andrew Tergis 2011

BUGswarm - an arduino library for the Swarm system by Buglabs.
This library is intented to be neither minimum-viable nor 100% complete.  It
aims to preform the minimum amount of abstraction to get an average arduino
programmer up and running with the Swarm platform.  Any abstraction is
intended to be optional - the library shouldn't force users to abstract away
from custom JSON payloads, or from receiving presence messages.

Programmers should be very mindful of the limited RAM and code space of the 
arduino platform.  As much memory should be stored in program memory as
possible, without adding extraneous retreival overhead.  This is a constant
balance of functionality versus RAM consumption.  For sample usage of this
library, see the "examples" folder.

DEPENDENCIES:
-Arduino release 1.0 or later
-A Wiznet based EthernetShield (preferably the stock arduino shield)

*/

#ifndef BUGSWARM_H_
#define BUGSWARM_H_

#include "Arduino.h"
#include "EthernetClient.h"
#include "IPAddress.h"
#include <avr/pgmspace.h>

// The size of the only large buffer for this library
#define SWARM_BUFFER_SIZE       340

// States for the read() statemachine
#define READ_STATE_LOOKING      0x0
#define READ_STATE_PAYLOAD      0x1
#define READ_STATE_SENDER       0x2
#define READ_STATE_WRAPPED      0x3


// PROGMEM string space:
// Swarm specific messages (or parts of messages):
//
// produce_header - the HTML request, headers AND 1st payload needed to open
// a Swarm connection that can produce and consume.
const char produce_header[] PROGMEM = "POST /stream?swarm_id=%s&"
  "resource_id=%s HTTP/1.1\r\nHost:api.test.bugswarm.net\r\n"
  "x-bugswarmapikey:%s\r\ntransfer-encoding:chunked\r\nConnection:keep-alive"
  "\r\nContent-Type: application/json\r\n\r\n1\r\n\n\r\n";
  
// text preceeding an outgoing message.
// _JSON wraps a fixed JSON payload around the outgoing message
const char message_header_JSON[] PROGMEM = "{\"message\": {\"payload\": "
  "{\"data\":\"";
  
// _basic only contains the necessary swarm message format
const char message_header_basic[] PROGMEM = "{\"message\": {\"payload\":";

// text following an outgoing message, varies depending on wrapJSONForMe
const char message_tail_JSON[] PROGMEM = "\"}}}";
const char message_tail_basic[] PROGMEM = "}}";

// search string for a valid payload, varies depending on wrapJSONForMe
const char payload_indicator_JSON[] PROGMEM = "\"payload\"";    
const char payload_indicator_basic[] PROGMEM = "\"data\"";

// search string for a valid resource 
const char resource_indicator[] PROGMEM = "\"resource\"";


// class BUGswarm:
//
// a rough implementation of the Stream and Print classes allowing programmers 
// to communicate with Swarm.  A basic swarm session could look like this:
//
// BUGswarm swarm(swarm_id, resource_id, participation_key)
// IPAddress server(64,118,81,28); // api.test.bugswarm.net
// swarm.connect(server)
// swarm.wrapJSONForMe(true);
// swarm.println("Hello world!");
//

class BUGswarm : public Stream {
  public:
    // Initializes a new swarm object, see code in /examples/
    BUGswarm(const char *swarm_id, const char *resource_id, const char *participation_key);
    
    // Opens a connection to a swarm server and sends appropriate header.
    boolean connect(const IPAddress *serv);
    
    // Directly produces data to the swarm.
    // This can be used if your message is already compiled into a buffer.
    // Otherwise, swarm.print() and swarm.println() can be used.
    void produce(char * message);
    
    // Determines if valid data is available to be read
    // Effectively, if peek() > -1
    int available();
    
    // DEPRECATED
    // Reads a line from the socket into the internal buffer, and prints it
    void printMessage();
    
    // Changes the amount of wrapping on both read() and write()
    // If true, outgoing data will be wrapped into the following structure:
    // {"data":"<message>"}
    // incoming data will only be detected if it matches this structure.
    // If false, outoing data MUST be a valid JSON object.
    void wrapJSONForMe(boolean value);
    
    // Enables the raw reception of swarm data
    // If true, all swarm messages (including presence, and messages produced
    // by this resource) will be returned by read()
    // If false, read will only return swarm payloads NOT owned by me
    void setRawReadMode(boolean value){rawReadMode = value;};

    // Checks for new data (peeks()) and copies returned data into buff
    // Returns true if a message was actually copied into buff
    boolean getNewMessage(char * buff, int len);

    // The following functions allow us to implement Stream and Print.
    // Any write operations that end with a '\n' will be produce()'d.
    // Read and Peek operations will return -1 if no data is available, OR if
    // the current data is filtered out by rawReadMode or wrapJSONForMe
    // NOTE - read() or peek() must be called repeatedly in order for the 
    // swarm messages to be filtered.  This allows us to remain nonblocking,
    // but to preform the necessary processing to filter out unwanted messages.
    size_t write(uint8_t data);
    int read();
    int peek();
    void flush();    
    
  private:
    // DEPRECATED
    // Clears the swarm_buff, and reads one line from client into swarm_buff
    void readMessage();
    
    // Reads from client until a newline is reached.
    // Essentially skips over the rest of a message waiting to be read.
    void readUntilNewline();
    
    // Prints the contents of swarm_buff - helper debug function.
    void printBuffer();

    // An internal instance of EthernetClient - used by all swarm operations
    EthernetClient client;
    
    // The primary internal string buffer used by the BUGswarm library.
    // This is used by both read() and write(), so those operations are not
    // parallelizeable!
    char swarm_buff[SWARM_BUFFER_SIZE];

    // A reference to the IP Address of the swarm server to connect to.
    // IPAddress object provided by the arduino EthernetClient library.
    const IPAddress *server;
    
    // A reference to the swarm ID, a 40 character string.
    const char *swarm;
    
    // A reference to the resource ID, a 40 character string.
    const char *resource;
    
    // A reference to the producer API Key, a 40 character string.
    const char *key;
    
    // A reference to the current message_header (see PROGMEM above)
    const char * message_header PROGMEM;
    
    // A reference to the current message_tail (see PROGMEM above)
    const char * message_tail PROGMEM;
    
    // A reference to the current payload search string (see PROGMEM above)
    const char * payload_indicator PROGMEM;
    
    // Stores the rawReadMode setting (see setRawReadMode() above)
    boolean rawReadMode;
    
    // Stores the wrapJSON setting (see wrapJSONForMe() above)
    boolean wrapJSON;

    // The last empty byte within swarm_buff when write()-ing
    int produce_idx;

    // A token counter for brackets when reading
    char read_counter;
    
    // The number of bytes read while in READ_STATE_LOOKING
    int read_idx;
    
    // A small buffer used to detect where we are in the incoming byte stream
    char read_buff[10];
    
    // The main state variable for read()
    // This will contain one of the READ_STATE defines declared above
    int read_state;
    
    // The last byte peek()'d - returned to user if a user peeks before reading
    int peekbyte;
    
    // A single helper byte to detect escaped quotes
    char last_byte;
};

#endif
