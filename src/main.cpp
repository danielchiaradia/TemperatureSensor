
extern "C"
{
#include <user_interface.h>
}

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Wire.h>
#include "Adafruit_Si7021.h"
#include <ArduinoOTA.h>
#include <SPI.h>
#include <ESP8266HTTPClient.h>
#include "Defines.h"
#include "Secrets.h"
#include "Logger.h"

// SI7021 I2C address is 0x40(64)
#define si7021Addr 0x40

ADC_MODE(ADC_VCC);
long start = millis();

struct
{
	int magicNumber;
	int counter;
	float temp;
	float hum;
	// Indicates if the ESP has been reset for data transmission. See https://github.com/esp8266/Arduino/issues/3072
	bool transmission;
	long lastTransmissionTime;
} rtcData;

unsigned long startedAtMs;

// Begin of function specification
void deepSleepModenOn(int secondToSleep);
void deepSleep(int secondToSleep, WakeMode wakeMode);
void readRtcData();
void saveRtcData();
uint32_t getWifiChannel(String ssid);
void deepSleepModenOn(int secondToSleep);
void readSensorData();
void sendSensorData();
void configureWiFi();
void connectToWifi();
void setup();
void loop();
bool isSSIDAvailable(String ssid);
// End of function specification

void readRtcData()
{
	system_rtc_mem_read(64, &rtcData, sizeof(rtcData));

	if (rtcData.magicNumber != 0xCAFE)
	{
		Logger.log("Powered up for the first time");
		rtcData.magicNumber = 0xCAFE;
		rtcData.counter = 0;
		rtcData.temp = -1000;
		rtcData.hum = -1000;
		rtcData.lastTransmissionTime = 0;
		rtcData.transmission = false;
	}
	else
	{
		Logger.log("Waking up after reset");
		rtcData.counter++;
	}
}

uint32_t getWifiChannel(String ssid)
{
	int networksFound = WiFi.scanNetworks();
	int i;
	for (i = 0; i < networksFound; i++)
	{
		if (ssid == WiFi.SSID(i))
		{
			return WiFi.channel(i);
		}
	}
	return -1;
}

bool isSSIDAvailable(String ssid)
{
	int networksFound = WiFi.scanNetworks();
	int i;
	for (i = 0; i < networksFound; i++)
	{
		if (ssid == WiFi.SSID(i))
		{
			return true;
		}
	}
	return false;
}

void saveRtcData()
{
	system_rtc_mem_write(64, &rtcData, sizeof(rtcData));
}

void deepSleepModenOn(int secondToSleep)
{
	deepSleep(secondToSleep, WAKE_RFCAL);
}

void deepSleep(int secondToSleep, WakeMode wakeMode = WAKE_RF_DISABLED)
{
	Logger.log("Execution took %i ms.\nDeep sleep for %i seconds.", millis() - startedAtMs, secondToSleep);
	ESP.deepSleep(secondToSleep * 1e6, wakeMode);
	delay(100);
}

void readSensorData()
{
	Wire.begin(2, 0);
	Adafruit_Si7021 sensor = Adafruit_Si7021();
	if (!sensor.begin())
	{
		Logger.log("Did not find Si7021 sensor!");
		deepSleep(10);
	}

	float hum = sensor.readHumidity();
	float temp = sensor.readTemperature();
	// digitalWrite(SENSOR_VCC_PIN, LOW);

	if (abs(temp - rtcData.temp) >= THRESHOLD_SEND_DATA_TEMPERATURE || abs(hum - rtcData.hum) >= THRESHOLD_SEND_DATA_HUMIDITY || rtcData.counter * INTERVAL_READ_SENSOR_SECS >= INTERVAL_SEND_DATA_SECS)
	{
		Logger.log("Threshold exceeded, resetting to send data...");

		rtcData.temp = temp;
		rtcData.hum = hum;
		rtcData.counter = 0;
		rtcData.transmission = true;
		saveRtcData();
		deepSleepModenOn(TRANSMISSION_REBOOT_SECS);
	}
	else
	{
		saveRtcData();
		deepSleep(INTERVAL_READ_SENSOR_SECS);
	}
}

void sendSensorData()
{
	Logger.log("Sending Sensor Data ...");
	HTTPClient http;

	http.begin(callback + ESP.getVcc() + "/" + rtcData.lastTransmissionTime + "/" + (millis() - start) + "/" + rtcData.temp + "/" + rtcData.hum);
	http.GET();
}

void configureWiFi()
{
	IPAddress local_IP(192, 168, 178, SENSOR_IP);
	IPAddress gateway(192, 168, 178, 1);
	IPAddress subnet(255, 255, 255, 0);
	WiFi.config(local_IP, gateway, subnet);
	WiFi.setAutoConnect(true);
	WiFi.setAutoReconnect(true);
	WiFi.persistent(true);

	int channel = getWifiChannel(WIFI_SSID);
	WiFi.begin(WIFI_SSID, WIFI_PASSWD, channel, bssid);
}

void connectToWifi()
{
	WiFi.hostname(HOSTNAME);
	WiFi.mode(WIFI_STA);

	if (WiFi.SSID() != WIFI_SSID)
	{
		Logger.log("WiFi changed...");
		configureWiFi();
	}

	if (WiFi.waitForConnectResult(WIFI_CONNECT_TIMEOUT_SECS * 1000) != WL_CONNECTED)
	{
		if (!isSSIDAvailable(WIFI_SSID)) {
			Logger.log("SSID not available");	
			deepSleepModenOn(INTERVAL_SEND_DATA_SECS);
		}
		Logger.log("Could not connect to WiFi. Channel changed?");
		configureWiFi();
		deepSleepModenOn(WIFI_RECONNECT_WAITTIME_SECS);
	}

	Logger.log("...Connected! IP Address: %s", WiFi.localIP().toString().c_str());
	Logger.log("DNS IP Address: %s", WiFi.dnsIP().toString().c_str());
	Logger.log(" Signal strength: %i", WiFi.RSSI());
}

void setup()
{
	Serial.begin(74880);
	Logger.setEnable(false);
	Logger.log("Setup");
	startedAtMs = millis();
	readRtcData();

	// Each of the below functions can set the ESP to deep sleep mode which stops further execution.
	if (rtcData.transmission)
	{
		connectToWifi();
		sendSensorData();
		rtcData.transmission = false;
		rtcData.lastTransmissionTime = millis() - startedAtMs;
		saveRtcData();
	}
	else
	{
		readSensorData();
	}

	deepSleep(INTERVAL_READ_SENSOR_SECS);
}

void loop()
{
	deepSleep(INTERVAL_READ_SENSOR_SECS);
}
