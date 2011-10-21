#include <Arduino.h>
#include "swarmConnector.h"
#include "Streamprint.h"
#include "cppfix.h"

swarmConnector::swarmConnector(PGM_P apikey, PGM_P swarm, PGM_P resource) {
  client = new Client(_server, 80);
  server = "api.bugswarm.net";
  _apikey = apikey;
  _swarm = swarm;
  _resource = resource;
}

bool swarmConnector::send(char * data){
  if (!client->connected()){
    Serialprint("Socket closed - cant produce\n");
    return false;
  }
  Serialprint( "%x\r\n%s\r\n", strlen(data), data );
  Streamprint( *client, "%x\r\n%s\r\n", strlen(data), data );
  return true;
}

// connect expects a pointer to a callback function which is called at the end
bool swarmConnector::connect(void *onConnect) {

  if (!client->connected() && !client->connect()){
    Serialprint("Error: Socket connection failed");
    return false;
  }
  memset(scratch, '\0', SCRATCH_SIZE);
  strcpy_P(scratch, resource);
  Streamprint(*client, "PUT /resources/%s", scratch);

  memset(scratch, '\0', SCRATCH_SIZE);
  strcpy_P(scratch, swarmid);
  Streamprint(*client, "/feeds/connector?swarm_id=%s HTTP/1.1\n",scratch);

  appendHost();

  memset(scratch, '\0', SCRATCH_SIZE);
  strcpy_P(scratch, apikey);
  Streamprint(*client,"x-bugswarmapikey:%s\n",scratch);

  appendChunkedTransfer();

  endHeaders();

  if (!readUntilCode("200",true)){
    Serialprint("Error: Unable open producer socket");
    client->stop();
    return false;
  }

  delay(2000);
  onConnect();
  return true;
}

void Print(PGM_P PROGMEM str) {
   strcpy_P(buffer, str);
   Serial.println(buffer);
}
