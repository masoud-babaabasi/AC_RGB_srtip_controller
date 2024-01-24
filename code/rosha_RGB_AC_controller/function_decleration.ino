/************************************************
 *
 ************************************************/
uint8_t WIFI_connect(uint32_t timeout_con){
  uint32_t pre_time = millis();
  uint8_t itteration = 0;
  #if SERIAL_DEBUG >= 1
    Serial.print("\nConnecting to ");
    Serial.println(ssid);
    Serial.println(password);
  #endif

    WiFi.begin(ssid, password);
    while(WiFi.status() != WL_CONNECTED) {
      #if SERIAL_DEBUG >= 1
      Serial.print(".");
      #endif
      #if SERIAL_BT == 1
      SerialBT.print(".");
      #endif
      itteration++; 
      if( itteration >= 30 ||  millis() - pre_time >= timeout_con){
        #if SERIAL_DEBUG >= 1
        Serial.print("\ncould not connect to ");
        Serial.println(ssid);
        Serial.println(password);
        #endif

        return 0;
      }
      pre_time = millis();
      delay(250);
    }
    return 1; //succesful connection
 }
/************************************************
 * 
 ************************************************/
void set_pixel_color(String color){
  if( color.startsWith("0x") || color.startsWith("0X")){
      color.remove(0,2);
      char str[10];
      color.toCharArray(str,10);
      uint32_t rgb = strtol(str, 0, 16);
      // uint8_t red , blue , green;
      // red =   (rgb & 0xff0000) >> 16;
      // green = (rgb & 0x00ff00) >> 8;
      // blue = (rgb & 0x0000ff) >> 0;
      // rgb = (red << 16) | (blue << 8) | green;
      //rgb ^= 0xffffff;
      //Serial.println(str);
      LED_RGB.setPixelColor(0, rgb);         //  Set pixel's color (in RAM)
      LED_RGB.show();
    }else if( color.startsWith("DIM=") || color.startsWith("dim=")){
      color.remove(0,4);
      uint8_t btigh = color.toInt() * 2.55;
      //Serial.println(btigh);
      LED_RGB.setBrightness(btigh);
      LED_RGB.show();
    }
}
/************************************************
 * 
 ************************************************/
uint32_t convert_str_rgb_to_int(String color){
  if( color.startsWith("0x") || color.startsWith("0X")){
      color.remove(0,2);
      char str[10];
      color.toCharArray(str,10);
      uint32_t rgb = strtol(str, 0, 16);
      return rgb;
    }
}