#ifndef swarmConnector_h
#define swarmConnector_h

#define BUFF_SIZE 256
#define SCRATCH_SIZE 80

#include <Arduino.h>
#include <Ethernet.h>
#include <avr/pgmspace.h>
#include "Streamprint.h"

class swarmConnector {

   Client* client;
   // PGM_P is equivalent to 'const char * PROGMEM'
   PGM_P server;
   PGM_P _apikey;
   PGM_P _swarm;
   PGM_P _resource;
   char scratch[SCRATCH_SIZE];
   char buffer[BUFF_SIZE];

   public:
      swarmConnector(PGM_P apikey, PGM_P swarm, PGM_P resource);
      bool connect(void (*onConnect)());
      bool send(char * data);
   private:
      inline void appendHost(){
         Streamprint(*client,"Host: %s\n",server);
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
