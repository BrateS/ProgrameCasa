#include <DHT.h>
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>

//static const uint8_t D4   = 2;
#define DHTPINA 2

String host = "192.168.2.55";
DHT dht(DHTPINA, DHT22);
float dhtUmid,dhtTemp;

ESP8266WiFiMulti WiFiMulti;
 
void setup() {
  Serial.begin(115200);
  // Serial.setDebugOutput(true);

  Serial.println();
  Serial.println();
  Serial.println();

  for (uint8_t t = 4; t > 0; t--) {
    Serial.printf("[SETUP] WAIT %d...\n", t);
    Serial.flush();
    delay(1000);
  }

  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP("andy", "1234setrobrate97");
  
  dht.begin();
}
void loop() {
  readData();
  Serial.print("Temp,umid:");
  Serial.print(dhtTemp);
  Serial.print(",");
  Serial.println(dhtUmid);
  int rc = send_data();
  if( rc == 0 ){
    Serial.println("Going to sleep.");
    ESP.deepSleep(60e6*10);
  }
}
void readData(void){
  dhtTemp = dht.readTemperature();
  dhtUmid  = dht.readHumidity();
}
int send_data(){
  
  if ((WiFiMulti.run() == WL_CONNECTED)) {

    WiFiClient client;

    HTTPClient http;

    Serial.print("[HTTP] begin...\n");
    String url = "/add_data_termostatAndy.php?";
    Serial.print("Requesting URL: ");
    url = url + "umid=" + String(dhtUmid) + "&&" + "temp=" +String(dhtTemp);
    Serial.println(url);
    if (http.begin(client, "http://" + host + url)) {  // HTTP


      Serial.print("[HTTP] GET...\n");
      // start connection and send HTTP header
      int httpCode = http.GET();

      // httpCode will be negative on error
      if (httpCode > 0) {
        // HTTP header has been send and Server response header has been handled
        Serial.printf("[HTTP] GET... code: %d\n", httpCode);

        // file found at server
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
          String payload = http.getString();
          Serial.println(payload);
        }
      } else {
        Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
      }

      http.end();
    } else {
      Serial.printf("[HTTP} Unable to connect\n");
    }
  }else{
    Serial.printf("[HTTP] Not connected to wifi, waiting...\n");
    delay(3000);
    return -1;
  }
  return 0;
}
