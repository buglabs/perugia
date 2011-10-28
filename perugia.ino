#include <SPI.h>
#include <Ethernet.h>
#include <Swarm.h>

// Mac address printed on sticker on ethershield
static byte mac[6] = { 0x9D, 0xA2, 0xDA, 0x00, 0x03, 0x4E };
static byte ip[4] = { 192, 168, 1, 18 };
static byte dns[4] = { 192, 168, 1, 1 };

// Setup the swarm
Config config;
Swarm swarm(&config);

void setup() {
  config.apikey = "";
  config.swarmid = "";
  config.resourceid = "";

  Serial.begin(9600);
  Ethernet.begin(mac, ip, dns);
  delay(1000);
  printNetworkInfo();
  Serial.println('ICSEEYOU');
  swarm.connect(onConnect);
}

void loop() {
  Serial.print('.');
  delay(2000);
}

void onConnect(void) {
  swarm.send("OH HI MAHK");
  return;
}

void printNetworkInfo() {
  PrintAddress("IP", Ethernet.localIP());
  PrintAddress("GW", Ethernet.gatewayIP());
  PrintAddress("DNS", Ethernet.dnsServerIP());
}

void PrintAddress(char * label, IPAddress _ip) {
  Serialprint("%s: ", label);
  PrintAddress(_ip);
  Serial.println();
}

void PrintAddress(IPAddress _ip) {
  Serialprint("%d.%d.%d.%d", _ip[0], _ip[1], _ip[2], _ip[3]);
}
