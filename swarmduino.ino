#include <Ethernet.h>
#include <EthernetDHCP.h>
#include <swarmConnector.h>

const byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

const PGM_P api_key[] = "";
const PGM_P swarm_id[] = "";
const PGM_P resource_id[] = "";

swarmClient swarm(api_key, swarm_id, resource_id);

const char* ip_to_str(const uint8_t*);

const void onConnect();

void setup()
{
  Serial.begin(9600);
  EthernetDHCP.begin(mac, 1);
}

void loop()
{
  static DhcpState prevState = DhcpStateNone;
  static unsigned long prevTime = 0;
  
  DhcpState state = EthernetDHCP.poll(); // call poll/maintain every loop to keep address

  if (prevState != state && state == DhcpStateLeased) {
    // We have a network address, connect to swarm
    printNetworkInfo();
    swarm.connect(&onConnect);
  } else if (state != DhcpStateLeased && millis() - prevTime > 300) {
    // Print a . every 300ms we don't have a lease
    prevTime = millis();
    Serial.print('.'); 
  }

  prevState = state;
}

const void onConnect() {
  swarm.send("OH HI MAHK");
  return;
}

// Just a utility function to nicely print the network info
const void printNetworkInfo() {
  const byte* ipAddr = EthernetDHCP.ipAddress();
  const byte* gatewayAddr = EthernetDHCP.gatewayIpAddress();
  const byte* dnsAddr = EthernetDHCP.dnsIpAddress();

  Serial.print("My IP address is ");
  Serial.println(ip_to_str(ipAddr));

  Serial.print("Gateway IP address is ");
  Serial.println(ip_to_str(gatewayAddr));

  Serial.print("DNS IP address is ");
  Serial.println(ip_to_str(dnsAddr));
  Serial.println();
}

// Just a utility function to nicely format an IP address.
const char* ip_to_str(const uint8_t* ipAddr)
{
  static char buf[16];
  sprintf(buf, "%d.%d.%d.%d\0", ipAddr[0], ipAddr[1], ipAddr[2], ipAddr[3]);
  return buf;
}
