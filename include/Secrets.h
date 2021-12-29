#ifndef SECRETS_H_
#define SECRETS_H_

#define XSTR(x) #x
#define STR(x) XSTR(x)

#define WIFI_SSID "X"
#define WIFI_PASSWD "Y"
#define HOSTNAME "Z"

const byte bssid[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
const String callback = "http://callback.url";

#endif /* SECRETS_H_ */