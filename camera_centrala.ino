#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#define RELAYS 8
#define DEBUG
EthernetClient client;
byte mac[] ={0x90, 0xA2, 0xDA, 0x0f, 0x25, 0xE7};
IPAddress ip(192, 168, 2, 204);
short port=5000;
IPAddress server(192, 168, 2, 55);
String datReq;
EthernetUDP Udp;
bool relayState[RELAYS];
int pinRelay[RELAYS];

void setup() {
  #ifdef DEBUG
  Serial.begin(9600);
  #endif
  sensors.begin();
  Ethernet.begin( mac, ip);
  Udp.begin(port);
  delay(1500);
  while (!Serial);
  pinRelay[0]=1;pinRelay[1]=2;pinRelay[2]=3;pinRelay[3]=4;
  pinRelay[4]=6;pinRelay[5]=7;pinRelay[6]=8;pinRelay[7]=9;
  for(int i=0;i<8;i++){
    pinMode(pinRelay[i], OUTPUT);
    digitalWrite(pinRelay[i], HIGH);
  }
  #ifdef DEBUG
  Serial.println("");
  Serial.println("Setup done.");
  printConfig();
  #endif
  #ifdef DEBUG
  printData();
  #endif
}
float getTemp(DeviceAddress deviceAddress)
{
  return sensors.getTempC(deviceAddress);
}
void loop() {
  unsigned long currentMillis = millis();
  if(currentMillis - previousMillis > interval) {
    // save the last time
    previousMillis = currentMillis;
    getTemps();
    #ifdef DEBUG
    printData();
    #endif
  }
  int packetSize =Udp.parsePacket();
  if(packetSize) {
    Udp.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);
    String datReq(packetBuffer);
    #ifdef DEBUG
    Serial.println(datReq);
    Serial.print("PacketSize=");
    Serial.println(packetSize);
    #endif
    if (datReq=="data")send_data();
    else if(datReq.indexOf("releu")>=0)setRelays(datReq);
    else {
      #ifdef DEBUG
      Serial.println("This is not a valid command!");
      #endif
    }
  }
  memset(packetBuffer, 0, UDP_TX_PACKET_MAX_SIZE);
}
void setRelays(String datReq){
  int i=5;
      while(i){
        if(i-5>7)break;//string prea mare, nu am atatea relee
        if(datReq.charAt(i)!='0'&&datReq.charAt(i)!='1')break;
        if(datReq.charAt(i)=='1'){
          relayState[i-5]=true;
          digitalWrite(pinRelay[i-5], LOW);
        }
        else if(datReq.charAt(i)=='0'){
          relayState[i-5]=false;
          digitalWrite(pinRelay[i-5], HIGH);
        }
        i++;
      }
}
void send_data(){
  if(client.connect(server, 80)) {
      #ifdef DEBUG
      Serial.println("-> Connected");
      #endif
      // Make a HTTP request:
      client.print( "GET /add_data_camera_centrala.php?");
      for(int i=0;i<DEVICES;i++){
        client.print("temp");
        client.print(i+1);
        client.print("=");
        client.print(t[i]);
        if(i!=DEVICES-1)client.print("&&");
      }
      client.print("&&");
      for(int i=0;i<RELAYS;i++){
        client.print("releu");
        client.print(i+1);
        client.print("=");
        if(relayState[i])client.print("1");
        if(i!=RELAYS-1)client.print("&&");
      }
      client.println( " HTTP/1.1");
      client.print( "Host: " );
      client.println(server);
      client.println( "Connection: close" );
      client.println();
      client.println();
      client.stop();
    }
    else {
      // you didn't get a connection to the server:
      #ifdef DEBUG
      Serial.println("--> connection failed/n");
      #endif
    }
}
void getTemps(void){
  sensors.requestTemperatures();
  delay(800);
  t[0]=getTemp(t0);
  t[1]=getTemp(t1);
  t[2]=getTemp(t2);
  t[3]=getTemp(t3);
  t[4]=getTemp(t4);
  t[5]=getTemp(t5);
  t[6]=getTemp(t6);
  t[7]=getTemp(t7);
}
#ifdef DEBUG
void printData(void){
  for(int i=0;i<DEVICES;i++){
    Serial.print(t[i]);
    Serial.print(" ");
  }
  Serial.print("  ");
  for(int i=0;i<RELAYS;i++)
    if(relayState[i]==true)Serial.print("1");
    else Serial.print("0");
  Serial.println("");
}
void printConfig(void){
  Serial.println("PLEASE CHECK!!!\nConfiguration:");
  Serial.println("|Relays|Sensors|      IP::Port    |");
  Serial.print("   ");Serial.print(RELAYS);Serial.print("\t  ");Serial.print(DEVICES);Serial.print("     ");
  Serial.print(ip);Serial.print(":");Serial.println(port);
  Serial.print("RelayPins:");
  for(int i=0;i<8;i++){
    Serial.print(" ");
    Serial.print(pinRelay[i]);
  }
  Serial.println("");
  Serial.print("oneWireBus:");
  Serial.println(ONE_WIRE_BUS);
  Serial.print("tempPrecision:");
  Serial.println(TEMPERATURE_PRECISION);
}
#endif
