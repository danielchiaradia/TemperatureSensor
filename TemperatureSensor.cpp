
extern "C" {
#include <user_interface.h>
}

#define HTTP_DEBUG_PRINT(string) (Serial.print(string))

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Wire.h>
#include "Adafruit_Si7021.h"
#include "BlynkRestAPI.h"
#include <ArduinoOTA.h>
#include "define.h"
#include "Display.h"

char myhostname[16] = { "TEMP_ESP" };

BlynkRestAPI blynkRestAPI = BlynkRestAPI();
Display display = Display();

struct {
	int magicNumber;
	int counter;
	float temp;
	float hum;
	bool notificationSent;
	int refreshRate = INTERVAL_READ_SENSOR_SECS;

} rtcData;

unsigned long startedAtMs;

void saveRtcData() {
	system_rtc_mem_write(64, &rtcData, sizeof(rtcData));
}

void setupOTA() {
	ArduinoOTA.setPort(8266);
	ArduinoOTA.setHostname(myhostname);
	ArduinoOTA.onStart([]() {
		Serial.println("OTA Transfer started");
	});
	ArduinoOTA.onEnd([]() {
		Serial.println("DONE");
	});
	ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
		Serial.printf(".", (progress / (total / 100)));
	});
	ArduinoOTA.onError([](ota_error_t error) {
		Serial.printf("Error[%u]: ", error);
		if (error == OTA_AUTH_ERROR)
		Serial.println("Auth Failed");
		else
		if (error == OTA_BEGIN_ERROR)
		Serial.println("Begin Failed");
		else
		if (error == OTA_CONNECT_ERROR)
		Serial.println("Connect Failed");
		else
		if (error == OTA_RECEIVE_ERROR)
		Serial.println("Receive Failed");
		else
		if (error == OTA_END_ERROR)
		Serial.println("End Failed");
	});
	ArduinoOTA.begin();
}

bool isInOTAMode() {
	digitalWrite(D4, LOW);
	pinMode(D4, INPUT_PULLUP);

	pinMode(D3, OUTPUT);
	digitalWrite(D3, LOW);

	return digitalRead(D4) == HIGH;
}

void deepSleep(int secondToSleep) {
	Logger.log("Execution took %i ms.\n Deep sleep for %i seconds.",
			millis() - startedAtMs, secondToSleep);
	ESP.deepSleep(secondToSleep * 1e6, WAKE_RFCAL);
	delay(100);
}

bool readSensorData() {
	delay(40);// Sensor wakeup time

	TwoWire wire = TwoWire();
	Adafruit_Si7021 sensor = Adafruit_Si7021(&wire);

	wire.pins(SENSOR_SDA, SENSOR_SCL);

	if (!sensor.begin()) {
		Logger.log("Did not find Si7021 sensor!");
		deepSleep(10);
	}

	system_rtc_mem_read(64, &rtcData, sizeof(rtcData));

	float hum = sensor.readHumidity();
	float temp = sensor.readTemperature();

	if (rtcData.magicNumber != 0xCAFE) {
		Logger.log("Powered up for the first time");

		rtcData.magicNumber = 0xCAFE;
		rtcData.counter = 0;
		rtcData.temp = -1000;
		rtcData.hum = -1000;
		rtcData.notificationSent = false;
	} else {
		rtcData.counter++;
	}

	if (abs(temp - rtcData.temp) >= THRESHOLD_SEND_DATA_TEMPERATURE
			|| abs(hum - rtcData.hum) >= THRESHOLD_SEND_DATA_HUMIDITY) {
		Logger.log("Threshold exceeded, resetting to send data...");

		rtcData.temp = temp;
		rtcData.hum = hum;
		rtcData.counter = 0;
		saveRtcData();

		return true;
	}
	else {
		saveRtcData();
		return rtcData.counter * rtcData.refreshRate >= INTERVAL_SEND_DATA_SECS;
	}
}

void connectToWifi() {
	if (WiFi.status() == WL_CONNECTED) {
		Logger.log("Still connected to WiFi.");
		return;
	}

	WiFi.hostname(myhostname);
	WiFi.mode(WIFI_STA);

	IPAddress ip, gateway, subnet, dns;
	ip.fromString(IP_ADDR);
	gateway.fromString(IP_GATEWAY);
	subnet.fromString(IP_SUBNET);
	dns.fromString(IP_DNS);

	//WiFi.config(ip, dns, gateway, subnet);
	WiFi.begin(WIFI_SSID, WIFI_PASSWD);

	Logger.log("Connecting to WiFi...");

	unsigned long startWiFiConnect = millis();

	while (WiFi.status() != WL_CONNECTED) {
		delay(50);

		if ((millis() - startWiFiConnect) > WIFI_CONNECT_TIMEOUT_SECS * 1e3) {
			Logger.log("WiFi connect exceeded. Going to deep sleep...");
			deepSleep(WIFI_RECONNECT_WAITTIME_SECS);
			return;
		}
	}

	Logger.log("...Connected! IP Address: %s", WiFi.localIP().toString().c_str());
	Logger.log("DNS IP Address: %s", WiFi.dnsIP().toString().c_str());
	Logger.log(" Signal strength: %i", WiFi.RSSI());
}

void syncBlynk() {
	connectToWifi();
	blynkRestAPI.sendValue(BLYNK_HUMIDITY_PIN, rtcData.hum);
	blynkRestAPI.sendValue(BLYNK_TEMPERATURE_PIN, rtcData.temp);

	if (rtcData.hum > HUMIDITY_NOTIFICATION_THRESHOLD && !rtcData.notificationSent) {
		blynkRestAPI.sendNotify(BLYNK_HUMIDITY_MESSAGE);
		rtcData.notificationSent = true;
	}
	else if (rtcData.hum < HUMIDITY_NOTIFICATION_THRESHOLD && rtcData.notificationSent) {
		rtcData.notificationSent = false;
	}

	rtcData.refreshRate = blynkRestAPI.getIntValue(BLYNK_REFRESH_RATE_PIN);
	rtcData.counter = 0;
	saveRtcData();
}

void powerOnDevices() {
	// power up the sensor
	pinMode(SENSOR_VCC_PIN, OUTPUT);
	digitalWrite(SENSOR_VCC_PIN, HIGH);

	// power up display
	if (isInOTAMode()) {
		pinMode(DISPLAY_VCC_PIN, OUTPUT);
		digitalWrite(DISPLAY_VCC_PIN, HIGH);
	}
}

void setup() {
	Serial.begin(115200);
	Serial.println("Start");
	startedAtMs = millis();

	Logger.setEnable(true);
	Logger.log("Setup");
	powerOnDevices();
	if (isInOTAMode()) {
		Logger.log("Going into OTA Mode");
		connectToWifi();
		setupOTA();
		wifi_set_sleep_type(LIGHT_SLEEP_T);
	}
	else {
		bool aboveThreshold = readSensorData();
		display.showValues(rtcData.temp, rtcData.hum);
		if (aboveThreshold) {
			syncBlynk();
		}
		deepSleep(aboveThreshold ? INTERVAL_SHORT_READ_SENSOR_SECS : rtcData.refreshRate);
	}
}

void loop() {
	// Waiting for OTA Connection
	if (!isInOTAMode()) {
		deepSleep(2);
	}
	ArduinoOTA.handle();

	if (readSensorData()) {
		syncBlynk();
	}

	display.showValues(rtcData.temp, rtcData.hum);
	delay(2000);
}
