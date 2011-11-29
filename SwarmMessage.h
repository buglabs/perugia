#ifndef SWARMMESSAGE_H
#define SWARMMESSAGE_H

#define SWARM_PAYLOAD_LEN        120

#define SWARM_MTYPE_PRESENCE     0x1
#define SWARM_MTYPE_MESSAGE      0x2


class SwarmMessage {
   public:
      SwarmMessage(char * buff);
      void destroy();
      
      char messageType;
      char swarm[41];
      char resource[41];
      bool present;
      bool private_message;
      char payload[SWARM_PAYLOAD_LEN];
   private:
      void getSource(char * buff, int len);
      void getPayload(char * buff, int len);
};

#endif
