#ifndef DEFINES_H_
#define DEFINES_H_

#define WIFI_SSID  "tbd"
#define WIFI_PASSWD  "secret"

#define IP_ADDR             "192.168.0.74"
#define IP_GATEWAY          "192.168.0.1"
#define IP_DNS              "192.168.0.1"
#define IP_SUBNET           "255.255.255.0"

// BLYNK Configuration Parameter
#define BLYNK_CLOUD_ADDRESS 	"blynk-cloud.com"//139.59.206.133
#define BLYNK_AUTH 				"secret too"
#define BLYNK_HUMIDITY_PIN 		"V7"
#define BLYNK_TEMPERATURE_PIN 	"V8"
#define BLYNK_REFRESH_RATE_PIN 	"V0"
#define BLYNK_HUMIDITY_MESSAGE 	"{ \"body\": \"Humidity is too high in livingroom.\"}"
#define BLYNK_ACTIVATE_TRANSMISSION true

#define ROOM_NAME "livingroom"

#define WIFI_CONNECT_TIMEOUT_SECS    10
#define WIFI_RECONNECT_WAITTIME_SECS  5

#define INTERVAL_READ_SENSOR_SECS   60
#define INTERVAL_SHORT_READ_SENSOR_SECS   15
#define INTERVAL_SEND_DATA_SECS     600


#define THRESHOLD_SEND_DATA_TEMPERATURE 1
#define THRESHOLD_SEND_DATA_HUMIDITY  1

#define HUMIDITY_NOTIFICATION_THRESHOLD  63

#define ADJUST_TEMPERATURE        0.0
#define ADJUST_HUMIDITY         0.0

#define SENSOR_VCC_PIN D7
#define SENSOR_SDA D2
#define SENSOR_SCL D1

#define DISPLAY_VCC_PIN D8
#define DISPLAY_SDA D6
#define DISPLAY_SCL D5

#endif /* DEFINES_H_ */
