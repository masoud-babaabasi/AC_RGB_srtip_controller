
/*
 * @brief : gets the color of the strip and saves it into R,G,B global variales
 */
void handle_RGB(){
  String arg_post = server.arg(F("plain"));
  arg_post.toLowerCase();
  if( arg_post.indexOf(F("color")) ){
    server.send(200, F("application/json"), F("{\"success\":true,\"msg\":\"RGB received\"}"));
    //arg_post.remove(0 , strlen("{ \"color\": \"#") );
    //arg_post.remove(0 , arg_post.indexOf(F("#")) + 1);
    //arg_post.remove(arg_post.length()-2,arg_post.length()-1);
    //arg_post.remove(arg_post.lastIndexOf(F("\"")),arg_post.length()-1);
    //if(arg_post.length() == 8 ) arg_post.remove(0,2);
    /*Serial.print("color");
    arg_post = "0x" + arg_post;
    Serial.println(arg_post);
    set_pixel_color(arg_post);*/
    deserializeJson(doc, arg_post);
    const char *color_str = doc[F("color")];
    arg_post = (char *)color_str;
    #if SERIAL_DEBUG >= 1
    Serial.print("color received : ");Serial.println(arg_post);
    #endif
    arg_post.remove(0 , 1);
    if(arg_post.length() == 8 ) arg_post.remove(0,2);
    arg_post = "0x" + arg_post;
    colors[ 0 ] = colors [1];
    colors[ 1 ] = convert_str_rgb_to_int( arg_post );
    R1 = ( (colors[ 0 ] & 0xff0000) >> 16 ); 
    G1 = ( (colors[ 0 ] & 0x00ff00) >> 8 ); 
    Bb1 = (  colors[ 0 ] & 0x0000ff ); 
    //Serial.print("RGB1 0x"); Serial.print(R1,HEX);Serial.print(G1,HEX);Serial.println(Bb1,HEX);
    //Serial.println(colors[0] , HEX);

    R2 = ( (colors[ 1 ] & 0xff0000) >> 16 ); 
    G2 = ( (colors[ 1 ] & 0x00ff00) >> 8 ); 
    Bb2 = (  colors[ 1 ] & 0x0000ff ); 
    #if SERIAL_DEBUG >= 1
    Serial.printf("R1:%x , G1:%x , B1:%x , R2:%x , G2:%x , B2:%x",R1 , G1 , Bb1 , R2 , G2 , Bb2);
    #endif
    //Serial.print("RGB2 0x ");Serial.print(R2,HEX);Serial.print(G2,HEX);Serial.println(Bb2,HEX);
    //Serial.println(colors[1] , HEX);
    transition = MAX_TRANSITION ;
  }
}
void handle_http_address(){
  String arg_post = server.arg(F("plain"));
  arg_post.toLowerCase();
  if( arg_post.indexOf(F("address")) ){
    server.send(200, F("application/json"), F("{\"success\":true,\"msg\":\"Http server address received.\"}"));
    deserializeJson(doc, arg_post);
    const char *add_str = doc[F("address")];
    memcpy( HTTP_SERVER_ADDRESS , add_str , 255);
    for(int i=0 ; i < 255 ; i++)
      EEPROM.write(SSID_LENGHT * 2 + i , HTTP_SERVER_ADDRESS[i]);
    EEPROM.commit();
  }
}
void handle_http_address_GET(){
  String output = "{\"http address for request\":\"";
  output+= HTTP_SERVER_ADDRESS;
  output += "\"}";
   server.send(200, F("application/json"),output);
}
/*
 * @briefe : recieves ssid and password of the wifi to connect to over Hotspot
 */
void handle_WIFICONF(){
  String arg_post = server.arg(F("plain"));
  //String ssid_string,pass_string;
  //uint8_t qutation_idx[8];
  if( arg_post.indexOf(F("ssid")) &&  arg_post.indexOf(F("pass")) ){
    //server.send(200, F("application/json"), F("{\"success\":true,\"msg\":\"SSID received\"}"));
    deserializeJson(doc, arg_post);
    const char *s = doc[F("ssid")];
    const char *p = doc[F("pass")];
    const char *id= doc[F("id")];
    if( s != NULL ) strcpy( ssid , s );
    if( p != NULL ) strcpy( password , p );
    if( id != NULL ) strcpy(device_ID , id);
    if( WIFI_connect(3000) ){
      wifi_connected = 1;
      String string_request_response = F("{\"success\":true,\"msg\":\"WIFI connected.\" , \"data\":\"");
      string_request_response += (WiFi.localIP()).toString() + F("\"}");
      for(int i =0; i < 8 ; i++) {
        server.send(200, F("application/json"), string_request_response);
        delay(50);
      }
      //WiFi.softAPdisconnect(true);
      for(int i=0 ; i < SSID_LENGHT ; i++)
         EEPROM.write(i , ssid[i]);
      for(int i=0 ; i < SSID_LENGHT ; i++)
        EEPROM.write(SSID_LENGHT + i , password[i]);
      EEPROM.commit();
      #if SERIAL_DEBUG >= 1
      Serial.println(F("WiFi connected."));
      Serial.println(ssid);
      Serial.println(password);
      Serial.print(F("IP address: "));
      Serial.println(WiFi.localIP());
      #endif

      #if SERIAL_BT == 1
      SerialBT.println(F("WiFi connected."));
      SerialBT.println(ssid);
      SerialBT.println(password);
      SerialBT.print(F("IP address: "));
      SerialBT.println(WiFi.localIP());
      #endif
      #if SERIAL_DEBUG >= 1
      Serial.print(F("Device id"));
      Serial.println(device_ID);
      Serial.println(HTTP_SERVER_ADDRESS);
      #endif
      memset(HTTP_SERVER_ADDRESS , 0 , 255);      
      for(int i=0 ; i < 255 ; i++)
        HTTP_SERVER_ADDRESS[i] = EEPROM.read(SSID_LENGHT * 2 + i);
      strcat(HTTP_SERVER_ADDRESS , "/api/v1/devices/");
      strcat(HTTP_SERVER_ADDRESS , device_ID);
      strcat(HTTP_SERVER_ADDRESS , "/update");
      for(int i=0 ; i < 32 ; i++){
         EEPROM.write(SSID_LENGHT * 2  + 255 + i , device_ID[i]);
      }
      EEPROM.commit();
      if (My_http.begin(My_client, HTTP_SERVER_ADDRESS )) { 
      #if SERIAL_DEBUG >= 1
      Serial.print(F("[HTTP] POST...\n"));
      #endif
      #if SERIAL_BT == 1
      SerialBT.print(F("[HTTP] POST...\n"));
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
        #if SERIAL_BT == 1
        SerialBT.printf_P(PSTR("[HTTP] POST... code: %d\n"), httpCode);
        #endif
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
          String payload = My_http.getString();
          #if SERIAL_DEBUG >= 1
          Serial.println(payload);
          #endif
          #if SERIAL_BT == 1
          SerialBT.println(payload);
          #endif
        }
      } else {
        #if SERIAL_DEBUG >= 1
        Serial.printf_P(PSTR("[HTTP] GET... failed, error: %s\n"), My_http.errorToString(httpCode).c_str());
        #endif
        #if SERIAL_BT == 1
        SerialBT.printf_P(PSTR("[HTTP] GET... failed, error: %s\n"), My_http.errorToString(httpCode).c_str());
        #endif
      }
      My_http.end();
      WiFi.softAPdisconnect(true);
    }
    }else{
      server.send(200, F("application/json"), F("{\"success\":false,\"msg\":\"Fail to connect to WIFI please try again.\"}"));
    }
  }
}
void handle_WIFI_satatus(){
  String output = get_stat_string();
  server.send(200, "application/json", output);
}
String get_stat_string(){
  String output= "{\"SSID\":\"";
  output += ssid;
  output += "\",\"password\":\"";
  output += password;
  output += "\",\"hotspot\":\"";
  if( wifi_connected ) output += "Connected to wifi";
  else output += "Hot spot ON";
  output += "\",\"IP\":\"";
  if( wifi_connected ) output += (WiFi.localIP()).toString();
  else output += (WiFi.softAPIP()).toString();
  output += "\"}";
  return output;
}
/*
 * @briefe : recieves updated command to update the firmware of the device over the internet.
 *           POST request the the body contains the url of the connection
 */
//  void handle_WIFIUPDATE(){
//   String arg_post = server.arg("plain");
//   if( arg_post.indexOf("update_url") ){
//     server.send(200, F("application/json"), F("{\"success\":true,\"msg\":\"update\"}"));
//     deserializeJson(doc, arg_post);
//     const char *update_url = doc["update_url"];
//     #if REMOTE_DEBUG == 1
//     ESPhttpUpdate.closeConnectionsOnUpdate(false);
//     #endif
//     ESPhttpUpdate.onStart(update_started);
//     ESPhttpUpdate.onEnd(update_finished);
//     ESPhttpUpdate.onProgress(update_progress);
//     ESPhttpUpdate.onError(update_error);
    
//     t_httpUpdate_return ret = ESPhttpUpdate.update(My_client, (char *)update_url);
//   }
//  }
