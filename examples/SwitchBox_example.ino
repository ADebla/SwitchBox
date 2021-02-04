#include "SwitchBox_V2_USB.h"

//WiFi Credentials
char* ssid = "ssid";
char* password = "password";
String mqtt_server="192.168.0.101";
char* id = "SB1";

//TCP Credentials
char * host = "192.168.0.113";
uint16_t port = 9221;

SwitchBox my_switch_box(0x20,0x21,0x78);

void setup() {
  delay(2000);

  //Initialize the device
  my_switch_box.begin(mqtt_server,ssid,password,id);

  //Connect to TCP Client - Power Supply
  //my_switch_box.connectToTcpClient(host,port,3);
}

void loop() {
  // put your main code here, to run repeatedly:
   my_switch_box.run();
}
