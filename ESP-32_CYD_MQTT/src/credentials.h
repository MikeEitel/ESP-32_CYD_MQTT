/*Made by Mike Eitel to store device credenitals on one place
  No rights reserved when private use. Otherwise contact author.

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software 
  and associated documentation files. The above copyright notice and this permission notice shall 
  be included in all copies or substantial portions of the software.
*/

// Wifi acess definitions of network to connect to  
#define wifi_ssid "xxx"
#define wifi_password "yyy"

#define myIP "100"                                    // Device ID

// Wifi definitions of this device  
//IPAddress staticIP(192,168,x,atoi(myIP));           // IOT device IP
IPAddress staticIP(1,1,1,atoi(myIP));       // REPLACE !!!!
IPAddress subnet(255,255,255,0);                      // Network subnet size
//IPAddress gateway(192,168,x,y);                                // Network router IP
IPAddress gateway(1,1,1,1);                // REPLACE !!!!

// Raspberri Pi Mosquitto MQTT Broker definitions
#define mqtt_server    "192.168.x.z"                  // IOT MQTT server IP
#define mqtt_user      "admin"
#define mqtt_password  "admin"
#define mqtt_port      1883
#define WiFi_timeout    101                           // How many times to try before give up
#define mqtt_timeout     11                           // How many times to try before try Wifi reconnect


// MQTT Topics
#define mytype         "esp/32S-CYD-"                 // Client Typ
#define iamclient      mytype myIP                    // Client name 
#define in_topic       iamclient "/command"           // This common input is received from MQTT
#define out_param      iamclient "/signal"            // Wifi signal strength is send to MQTT
#define out_ligth      iamclient "/sensor"            // Internal light sensor is send to MQTT
#define out_status     iamclient "/status"            // This is a general message send to MQTT
#define out_watchdog   iamclient "/watchdog"          // A watchdog bit send to MQTT
#define out_button     iamclient "/button"            // The touched area nr. is send to MQTT
#define out_sensors    iamclient "/sensors"           // This is a list of usable external sensors send to MQTT
#define mqtt_out_angle iamclient "/sen/angle"         // AS5600 angle position is send to MQTT
#define mqtt_out_co2   iamclient "/sen/co2"           // ASG10 air quality is send to MQTT
#define mqtt_out_tvoc  iamclient "/sen/tvoc"          // ASG10 air quality is send to MQTT
#define mqtt_out_press iamclient "/sen/pressure"      // BMPxxx air pressure is send to MQTT
#define mqtt_out_hum   iamclient "/sen/humidity"      // Humidity sensor is send to MQTT
#define mqtt_out_temp  iamclient "/sen/temp"          // BMPxxx temperatur is send to MQTT
#define mqtt_out_tempa iamclient "/sen/temp1"         // MXL90614 ambient temp is send to MQTT
#define mqtt_out_tempo iamclient "/sen/temp2"         // MXL90614 meassured object temp is send to MQTT
#define out_topic      iamclient "/loop"              // This helper debug variable can be send to MQTT

// Errors send as values in test mode
// error =  -1    Wrong command received
// error =   0    Normal status
// error =   1    First time connected
// error =   2    Reconnect succesfull
// error =   3    Not implemented

// Constant how often the mqtt message is send
#if defined(TEST)
  const long readinterval =  1500;                    // Interval at which to ask for mqtt messages
  const long sendinterval =  3000;                    // Interval at which sensor data is send via mqtt
#else
  const long readinterval =  3000; // 5000            // Interval at which to ask for mqtt messages
  const long sendinterval =  10000;                    // Interval at which sensor data is send via mqtt
#endif