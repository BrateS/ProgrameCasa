#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#define RELAYS 8
#define DEVICES 8
#define DEBUG
EthernetClient client;
byte mac[] ={0x90, 0xA2, 0xDA, 0x0f, 0x25, 0xE7};
IPAddress ip(192, 168, 2, 220);
short port=5000;
IPAddress server(192, 168, 2, 55);
String datReq;
EthernetUDP Udp;
char packetBuffer[15];
#define ONE_WIRE_BUS 5
#define TEMPERATURE_PRECISION 10
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
bool relayState[8];
float t[8];
int pinRelay[8];
//==================ADRESE SENZORI============================
DeviceAddress t0 = { 0x28, 0xA9, 0xE3, 0x34, 0x04, 0x00, 0x00, 0x98 };// solar 110 
DeviceAddress t1 = { 0x28, 0xE6, 0x5E, 0x34, 0x04, 0x00, 0x00, 0x6A };// solar 220
DeviceAddress t2 = { 0x28, 0x90, 0xA8, 0xFF, 0x05, 0x00, 0x00, 0x7C };// solar apa
DeviceAddress t3 = { 0x28, 0xD5, 0x0B, 0x35, 0x04, 0x00, 0x00, 0x16 };// puffer 100
DeviceAddress t4 = { 0X28, 0X21, 0XBC, 0XD0, 0X04, 0X00, 0X00, 0X6E };// puffer 75
DeviceAddress t5 = { 0x28, 0xF0, 0x4B, 0xFD, 0x05, 0x00, 0x00, 0x2C };// puffer 50
DeviceAddress t6 = { 0x28, 0x03, 0x7A, 0xFE, 0x05, 0x00, 0x00, 0xC6 };// pufer 25
DeviceAddress t7 = { 0x28, 0x6D ,0x5A ,0x34 ,0x04 , 0x00 , 0x00 , 0x67 };// boiler
//====================================================================================================
void setup() {
  relayState[0]=true;
  Serial.begin(9600);
  sensors.begin();
  Ethernet.begin( mac, ip);
  Udp.begin(port);
  delay(1500);
  while (!Serial);
  pinRelay[0]=1;pinRelay[1]=2;pinRelay[2]=3;pinRelay[3]=4;
  pinRelay[4]=6;pinRelay[5]=7;pinRelay[6]=8;pinRelay[7]=9;
  Serial.println("");
  Serial.println("Setup done.");
  #ifdef DEBUG
  printConfig();
  #endif
}
float getTemp(DeviceAddress deviceAddress)
{
  return sensors.getTempC(deviceAddress);
}
void loop() {
  getTemps();
  #ifdef DEBUG
  printData();
  #endif
  int packetSize =Udp.parsePacket();
  if(packetSize) {
    Udp.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);
    String datReq(packetBuffer);
    Serial.println(datReq);
    Serial.print("PacketSize=");
    Serial.println(packetSize);
  // if you get a connection, report back via serial:
    if (datReq=="data")send_data();
    else if(datReq.indexOf("releu") >= 0){//comanda pentru relee
      int i=5;
      while(i){
        if(datReq.charAt(i)!='0'&&datReq.charAt(i)!='1')break;
        if(datReq.charAt(i)=='1'){
          relayState[i-5]=true;
          digitalWrite(pinRelay[i-5], HIGH);
        }
        else if(datReq.charAt(i)=='0'){
          relayState[i-5]=false;
          digitalWrite(pinRelay[i-5], LOW);
        }
        i++;
      }
    }
    else if(datReq.indexOf("delay") != -1){
      int i=5;
      char c=datReq.charAt(i);
      int aux=0;
      while(c!='\0'){
        if(isDigit(c)){
          int r = c - '0';
          aux=aux*10+r;
        }
        else Serial.println("This is not a valid command!");
        i++;
        c=datReq.charAt(i);
      }
      Serial.print("Delay set at: ");
      Serial.println(aux);
    }
    else Serial.println("This is not a valid command!");
  }
  delay(250);
  memset(packetBuffer, 0, UDP_TX_PACKET_MAX_SIZE);
}

void send_data(){
  if(client.connect(server, 80)) {
      Serial.println("-> Connected");
      // Make a HTTP request:
      client.print( "GET /add_data.php?");
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
      Serial.println("--> connection failed/n");
    }
}
void getTemps(void){
  sensors.requestTemperatures();
  delay(1000);
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

