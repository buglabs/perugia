#ifndef Swarm_h
#define Swarm_h

#define BUFF_SIZE 256
#define SCRATCH_SIZE 80

#include <Arduino.h>
#include <Ethernet.h>
#include <avr/pgmspace.h>
#include "utility/StreamPrint.h"

struct Config {
  char *apikey;
  char *swarmid;
  char *resourceid;
  char *server;
  Config():server("api.bugswarm.net") { };
};

class Swarm {

   EthernetClient *client;
   Config * config;
   char buffer[BUFF_SIZE];

   public:
      Swarm(Config * config);
      bool connect(void (*onConnect)());
      bool send(char * data);
   private:
      inline void appendHost(){
         Streamprint(*client,"Host: %s\n",config->server);
      };
      inline void appendContentTypeJSON(){
         Streamprint(*client,"Content-Type: application/json\n");
      };
      inline void appendContentLength(int len){
         Streamprint(*client,"Content-Length: %d\n",len);
      };
      inline void appendChunkedTransfer(){
         Streamprint(*client,"Transfer-Encoding: chunked\n");
      };
      inline void endHeaders(){ Streamprint(*client,"\n");};

};

#endif
