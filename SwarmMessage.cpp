#include "SwarmMessage.h"
#include "Arduino.h"
#include <string.h>
#include "Streamprint.h"

SwarmMessage::SwarmMessage(char * buff){
   int len = strlen(buff);
   if (memmem(buff, len, "\"presence\"", 10) != NULL){
      messageType = SWARM_MTYPE_PRESENCE;   
   } else {
      messageType = SWARM_MTYPE_MESSAGE;
   }
   //Try and decode the origin of the message - swarm and resource
   getSource(buff, len);
   //"present" will only be valid for SWARM_MTYPE_PRESENCE
   if (memmem(buff, len, "\"type\":\"available\"", 18) != NULL)
      present = true;
   else
      present = false;
   getPayload(buff, len);
   if (memmem(buff, len, "\"public\":false", 14) != NULL)
      private_message = true;
   else
      private_message = false;
}

void SwarmMessage::getSource(char * buff, int len){
   memset(swarm, '\0', sizeof(swarm));
   memset(resource, '\0', sizeof(resource));
   void * fromBegin = memmem(buff, len, "\"from\":{", 8);
   if (fromBegin != NULL){
      void * start = memchr(fromBegin, '{', len-((char *)fromBegin - buff));
      if (start == NULL)
         return;
      char * end = (char *)memchr(start, '}', len-((char *)start-buff));
      char * swarmPos = (char *)memmem(start, len-((char *)start-buff), "\"swarm\":\"", 9);
      char * resourcePos = (char *)memmem(start, len-((char *)start-buff), "\"resource\":\"", 12);
      if (end == NULL)
         return;
      if (swarmPos > (char *)end)
         swarmPos = NULL;
      if (resourcePos > (char *)end)
         resourcePos = NULL;
      if (swarmPos != NULL)
         end = (char *)memccpy(swarm, swarmPos+9, '"', 40);
         if (end != NULL)
            *(end-1) = '\0';
      if (resourcePos != NULL){
         end = (char *)memccpy(resource, resourcePos+12, '"', 40);
         if (end != NULL)
            *(end-1) = '\0';
      }
   }
}

void SwarmMessage::getPayload(char * buff, int len){
   memset(payload, '\0', SWARM_PAYLOAD_LEN);
   void * fromBegin = memmem(buff, len, "\"payload\":{", 11);
   if (fromBegin != NULL){
      char * start = (char *)memchr(fromBegin, '{', len-((char *)fromBegin - buff));
      if (start == NULL)
         return;
      char * end = (char *)memchr(start, '}', len-((char *)start-buff));
      if (end == NULL)
         return;
      len = (int)(end - start)-1;
      memcpy(payload, start+1, len);
   }
}

void SwarmMessage::destroy(){
   free(swarm);
   free(resource);
}
