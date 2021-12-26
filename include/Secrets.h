#ifndef DEFINES_H_
#define DEFINES_H_

#define WIFI_SSID "SSID"
#define WIFI_PASSWD "PASSWORD"
#define HOSTNAME "HOST"

const byte bssid[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
const String callback = "http://callback.url";

#define WIFI_CONNECT_TIMEOUT_SECS    10
#define WIFI_RECONNECT_WAITTIME_SECS  5

#define INTERVAL_READ_SENSOR_SECS   60
#define INTERVAL_SEND_DATA_SECS     1200

#define THRESHOLD_SEND_DATA_TEMPERATURE 	1
#define THRESHOLD_SEND_DATA_HUMIDITY  		1

 #define SENSOR_VCC_PIN 1
// #define SENSOR_GND_PIN D3

#endif /* DEFINES_H_ */