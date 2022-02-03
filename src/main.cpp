
#include <WiFi.h>
#include <time.h>
#include <Arduino.h>
#include <HTTPClient.h>
#include "mysecrets.h"
#include "saxml/saxml.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;
const int ONBOARDLED = LED_BUILTIN ;



String formaturl(){
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return String("");
  }
  const char *domain = "10Y1001A1001A46L" ;
  char currenttime[50] ;
  strftime(currenttime,sizeof(currenttime),"%Y%m%d%H00",&timeinfo) ; //202201271800
  Serial.println(currenttime) ;
  Serial.println();
  char url[1000] ;
  sprintf(url,"https://transparency.entsoe.eu/api?documentType=A44&in_Domain=%s&out_Domain=%s&periodStart=%s&periodEnd=%s&securityToken=%s",domain,domain, currenttime, currenttime, token) ; // token = 94055d76-ccae-4980-ae6d-f39b355ae118 domain = 10Y1001A1001A44P
  //Serial.println(url) ;
  return String(url);
}
  
String loadxml(String formatedurl){
  HTTPClient http;
  http.begin(formatedurl, root_ca);
    int httpResponseCode = http.GET();
    if (httpResponseCode==200)
    {
      String payload = http.getString();
      //Serial.println(payload);
      http.end();
      delay(100);
      return payload ;
    }
    else 
    {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
      http.end();
      delay(100);

      return String("");
    }
}


bool connect_wifi(){
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  int counter = 0 ;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (++counter > 20) 
      return false ; 
  }
  Serial.println("");
  Serial.println("WiFi connected.");
  delay(100);
  return true ;
}

void disconnect_wifi(){
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  delay(150) ;
}

void setupboardtime(){
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  delay(100) ;

}


typedef enum {
    Waiting,
    Reading_price_tag,
}Parserstate;

typedef struct{
    Parserstate currentstate;
    int index;
    int price[24]; // in euro cents
}Parserdata;

void tagopen(void *cookie, const char *str){
    Parserdata *data = (Parserdata*)cookie;
    if (strcmp(str,"price.amount")==0){
        data->currentstate = Reading_price_tag ;
    }
}

void contenthandler(void *cookie, const char *str){
    Parserdata *data = (Parserdata*)cookie;
    if (data->currentstate == Reading_price_tag){
        //Serial.println("hej");
        //Serial.println(str);
        data->currentstate = Waiting ;
        data->price [data->index ++] = (int)(100*atof(str));

    }
}

Parserdata parsexml(String s){
    Parserdata data = {
        .currentstate = Waiting,
        .index = 0,
        };

    tSaxmlContext conxop = {
        .cookie = (void*)&data,
        .tagHandler = tagopen, 
        .tagEndHandler = NULL,
        .parameterHandler = NULL,
        .contentHandler = contenthandler,
        .attributeHandler = NULL,
    };

    tSaxmlParser parse = saxml_Initialize (&conxop,69);
    
    for(auto it = s.begin();it != s.end(); it++){     
      saxml_HandleCharacter(parse,*it);
    }
    return data ;    
}

void printdata(Parserdata *p){
  for (int i=0; i<24; i++){
      Serial.println("price: \t Timme:");
      Serial.printf("%i \t\t %i \n",p->price[i],i);

    }
}

void tandlampa(){
  Serial.println("ifunktion");
  digitalWrite(ONBOARDLED,HIGH);
  delay(50000);
  digitalWrite(ONBOARDLED,LOW);
}

void setup(){
  Serial.begin(115200);
  pinMode(ONBOARDLED,OUTPUT);
}


void loop(){
  if (connect_wifi()){
      setupboardtime() ;
      String url = formaturl();
      Serial.println(url);
      String data = loadxml(url);
      disconnect_wifi();
      Serial.println(data);
      Parserdata pdata = parsexml(data) ;
      struct tm timeinfo;
      if(!getLocalTime(&timeinfo)){
        Serial.println("Failed to obtain time in loop");
        delay(69);
        return ;
      }
      Serial.printf("Time is %i and elpris is %i \n", timeinfo.tm_hour, pdata.price[timeinfo.tm_hour]);
      
      if (pdata.price[timeinfo.tm_hour] > 10000){
        Serial.println("tänd");
        tandlampa();
      }

      //printdata (&pdata);
    }
  
  delay(42069);

}
