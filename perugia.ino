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
  Serial.print("IP:");
  PrintAddress(Ethernet.localIP());
  Serial.println();
  Serial.print("GW:");
  PrintAddress(Ethernet.gatewayIP());
  Serial.println();
  Serial.print("IP:");
  PrintAddress(Ethernet.dnsServerIP());
  Serial.println();
}

String PrintAddress(IPAddress ip) {
  for (byte thisByte = 0; thisByte < 4; thisByte++) {
    Serial.print(ip[thisByte], DEC);
    Serial.print("."); 
  }
}
