/*
 *    ESP8266(ESP-01) 220V AC RGB strip 
 *    Author : Masoud Babaabasi
 *    Last Date modified : 2024-01-24 
 */
#include <Adafruit_NeoPixel.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h> 
#include <EEPROM.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
/* defines ***************************************************************************************************************************/
#define SERIAL_DEBUG          1
#define SSID_LENGHT           20
#define ssid_AP               F("RGB_controller")
#define password_AP           F("123456789")
#define FILTER_ORD            12
#define MAX_TRANSITION        100
#define TRANSITION_INTERVAL   10 //milli seconds
#define WS2811_PIN            2
#define BRIGHTNESS            (uint8_t)( 255 * 70.0 / 100.0 )
#define PROFILE1_NUM          9
/* Constants *************************************************************************************************************************/
const uint32_t profile1[PROFILE1_NUM] = { 0xff0000 , 0xffff00 , 0xffffff , 0x00ff00 , 0x00ffff , 0xffffff , 0x0000ff , 0xff00ff , 0xffffff};
/* Global Variables *******************************************************************************************************************/
char ssid[SSID_LENGHT] ;
char password[SSID_LENGHT];
char device_type[32];
char device_ID[32];
char HTTP_SERVER_ADDRESS[512];
uint64_t device_UID;
char device_UID_str[24];

uint8_t   R1,R2,G1,G2,Bb1,Bb2;
uint32_t  colors[FILTER_ORD];
uint8_t   colors_idx = 0;
uint32_t  mytime_start;
int8_t    transition = 0;
int8_t    profile_idx,profile_time;
int32_t   profile_itt , max_profile;
const    long timeoutTime = 2000;
unsigned long currentTime = millis();
unsigned long previousTime = 0;
uint32_t start_time ;
String  header;
StaticJsonDocument<1024> doc;
volatile uint8_t wifi_connected = 0;

/* Global Objects ****************************************************************************************************************/
ESP8266WebServer server(80);

Adafruit_NeoPixel LED_RGB(1, WS2811_PIN, NEO_GRB + NEO_KHZ800);
WiFiClient My_client;
HTTPClient My_http;
/* Fuction prototyps *************************************************************************************************************/
uint8_t WIFI_connect(uint32_t timeout_con);
void set_pixel_color(String color);
uint32_t convert_str_rgb_to_int(String color);
uint8_t WIFI_connect(uint32_t timout_con);
void handle_WIFI_satatus();
String get_stat_string();
void handle_http_address();

/*Set Up function******************************************************************************************************************/
void setup() {
  String temp;
  device_UID = (uint64_t)(ESP.getChipId()) | (uint64_t)(ESP.getFlashChipId());
  sprintf(device_UID_str , "%08x%08x", (uint32_t)(device_UID >> 32) ,  (uint32_t)(device_UID & 0xffffffff));
  #if SERIAL_DEBUG >= 1
  Serial.begin(115200);
  Serial.println("\nStarting RGB controller");
  Serial.print("device UID : ");
  Serial.println(device_UID_str);
  Serial.setTimeout(10000);
  #endif
  EEPROM.begin(SSID_LENGHT * 2 + 255 + 32);
  LED_RGB.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  LED_RGB.setPixelColor(0, 0);
  LED_RGB.show();            // Turn OFF all pixels ASAP
  LED_RGB.setBrightness(BRIGHTNESS);

  set_pixel_color("0x000000");
  for(int i=0 ; i < SSID_LENGHT ; i++)
    ssid[i] = EEPROM.read(i);
  for(int i=0 ; i < SSID_LENGHT ; i++)
    password[i] = EEPROM.read(SSID_LENGHT + i);
  for(int i=0 ; i < 255 ; i++)
    HTTP_SERVER_ADDRESS[i] = EEPROM.read(SSID_LENGHT * 2 + i);
  for(int i=0 ; i < 32 ; i++)
    device_ID[i] = EEPROM.read(SSID_LENGHT * 2  + 255 + i);
  strcat(HTTP_SERVER_ADDRESS , "/api/v1/devices/");
  strcat(HTTP_SERVER_ADDRESS , device_ID);
  strcat(HTTP_SERVER_ADDRESS , "/update");
  //strcpy(ssid,"RS");
  //strcpy(password,"00000");

  if( WIFI_connect(3000) ){
    wifi_connected = 1;
  //   for(int i=0 ; i < SSID_LENGHT ; i++)
  //      EEPROM.write(i , ssid[i]);
  //  for(int i=0 ; i < SSID_LENGHT ; i++)
  //    EEPROM.write(SSID_LENGHT + i , password[i]);
  //  EEPROM.commit();
    #if SERIAL_DEBUG >= 1
    Serial.println(F("WiFi connected."));
    Serial.println(ssid);
    Serial.println(password);
    Serial.print(F("IP address: "));
    Serial.println(WiFi.localIP());
    #endif
    
    if (My_http.begin(My_client, HTTP_SERVER_ADDRESS )) { 
      #if SERIAL_DEBUG >= 1
      Serial.print(F("[HTTP] POST...\n"));
      #endif
      My_http.addHeader(F("Content-Type"), F("application/json"));
      String post_body = F("{\"field\": \"device_ip\",\"ip\":\"");
      post_body += (WiFi.localIP()).toString();
      post_body += F("\",\"ssid\":\"");
      post_body += ssid;
      post_body += F("\",\"device_type\":\"");
      post_body += "strip";
      post_body += F("\",\"unique_id\":\"");
      post_body += device_UID_str;
      post_body += F("\"}");
      #if SERIAL_DEBUG >= 1
      Serial.print("Post body:");Serial.println(post_body);
      #endif
      int httpCode = My_http.POST(post_body);
      if (httpCode > 0) {
        #if SERIAL_DEBUG >= 1
        Serial.printf_P(PSTR("[HTTP] POST... code: %d\n"), httpCode);
        #endif
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
          String payload = My_http.getString();
          #if SERIAL_DEBUG >= 1
          Serial.println(payload);
          #endif
        }
      } else {
        #if SERIAL_DEBUG >= 1
        Serial.printf_P(PSTR("[HTTP] GET... failed, error: %s\n"), My_http.errorToString(httpCode).c_str());
        #endif
      }
      My_http.end();
    }
  }
  if( wifi_connected != 1){
    WiFi.mode(WIFI_STA);
    WiFi.softAP(ssid_AP, password_AP);
    #if SERIAL_DEBUG >= 1
    Serial.println(F("WiFi  not connected."));
    Serial.println(F("Hot spot created:"));
    Serial.print("ssid_AP : ");
    Serial.println(ssid_AP);
    Serial.print("password_AP : ");
    Serial.println(password_AP);
    Serial.print("local IP : ");
    Serial.println(WiFi.softAPIP());
    #endif
  }
  server.on(F("/api/v1/change/color"),HTTP_POST ,  handle_RGB);
  server.on(F("/api/v1/wifi/config")  ,HTTP_POST ,  handle_WIFICONF);
  server.on(F("/api/v1/wifi/status")  ,HTTP_GET ,  handle_WIFI_satatus);
  server.on(F("/api/v1/wifi/address")  ,HTTP_POST ,  handle_http_address);
  server.on(F("/api/v1/wifi/address")  ,HTTP_GET ,  handle_http_address_GET);
  // server.on(F("/api/v1/wifi/update")  ,HTTP_POST ,  handle_WIFIUPDATE);
  server.begin();  
  start_time = millis();
}
/********************************************************************************************************************************/
void loop() {
  server.handleClient();
  if( transition >= 0 && millis() - mytime_start >= TRANSITION_INTERVAL ){
    uint8_t Rr,G,Bb;
    Rr = ( R2 - R1 ) * ( MAX_TRANSITION - transition ) / ( MAX_TRANSITION ) + R1 ;
    G = ( G2 - G1 ) * ( MAX_TRANSITION - transition ) / ( MAX_TRANSITION ) + G1 ;
    Bb = ( Bb2 - Bb1 ) * ( MAX_TRANSITION - transition ) / ( MAX_TRANSITION ) + Bb1 ;
    transition--;
    uint32_t color_RGB = ((uint32_t)Rr << 16) | ((uint32_t)G << 8) | (uint32_t)Bb;
    LED_RGB.setPixelColor(0, color_RGB);         //  Set pixel's color (in RAM)
    LED_RGB.show();
    mytime_start = millis();
  }
}
