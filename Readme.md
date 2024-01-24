# RGB Strip Controller
This project aims to control RGB strips powered by the main's high-voltage AC. The central controller is an ESP8266-01 module with WIFI capabilities. This module can receive RGB data via WIFI and control an RGB WS2811 chip. 
Users can change the strip's color via the `/API/v1/change/color` API. Color data is sent over the API body in JSON format like `{"color" : "#ff0000"}`.