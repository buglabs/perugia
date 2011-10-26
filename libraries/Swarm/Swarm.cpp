#include <Arduino.h>
#include <Ethernet.h>
#include "Swarm.h"
#include "utility/StreamPrint.h"

Swarm::Swarm(Config * _config) {
  EthernetClient client(80);
  config = _config;
}

bool Swarm::send(char * data){
  if (!client->connected()){
    Serialprint("Socket closed - cant produce\n");
    return false;
  }
  Serialprint( "%x\r\n%s\r\n", strlen(data), data );
  Streamprint( *client, "%x\r\n%s\r\n", strlen(data), data );
  return true;
}

// connect expects a pointer to a callback function which is called at the end
bool Swarm::connect(void (*onConnect)()) {

  if (!client->connected() && !client->connect(config->server, 80)){
    Serialprint("Error: Socket connection failed");
    return false;
  }
  Streamprint(*client, "PUT /resources/%s", config->resourceid);
  Streamprint(*client, "/feeds/connector?swarm_id=%s HTTP/1.1\n",config->swarmid);
  appendHost();
  Streamprint(*client,"x-bugswarmapikey:%s\n",config->apikey);

  appendChunkedTransfer();

  endHeaders();

  /*
  if (!readUntilCode("200",true)){
    Serialprint("Error: Unable open producer socket");
    client->stop();
    return false;
  }
  */

  delay(200);
  onConnect();
  return true;
}

void Print(char * string) {
   Serial.println(string);
}
