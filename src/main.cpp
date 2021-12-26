
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
} rtcData;

unsigned long startedAtMs;

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

void saveRtcData()
{
	system_rtc_mem_write(64, &rtcData, sizeof(rtcData));
}

void deepSleep(int secondToSleep)
{
	// Logger.log("Execution took %i ms.\n Deep sleep for %i seconds.", millis() - startedAtMs, secondToSleep);
	ESP.deepSleep(secondToSleep * 1e6, WAKE_RFCAL);
	delay(100);
}

void readSensorData()
{
	// pinMode(SENSOR_VCC_PIN, OUTPUT);
	// digitalWrite(SENSOR_VCC_PIN, HIGH);
	delay(100);
	Wire.begin(2, 0);
	Adafruit_Si7021 sensor = Adafruit_Si7021();
	if (!sensor.begin())
	{
		Logger.log("Did not find Si7021 sensor!");
		deepSleep(10);
	}

	system_rtc_mem_read(64, &rtcData, sizeof(rtcData));

	float hum = sensor.readHumidity();
	float temp = sensor.readTemperature();
	// digitalWrite(SENSOR_VCC_PIN, LOW);

	if (rtcData.magicNumber != 0xCAFE)
	{
		Logger.log("Powered up for the first time");
		rtcData.magicNumber = 0xCAFE;
		rtcData.counter = 0;
		rtcData.temp = -1000;
		rtcData.hum = -1000;
	}
	else
	{
		Logger.log("Waking up after reset");
		rtcData.counter++;
	}

	if (abs(temp - rtcData.temp) >= THRESHOLD_SEND_DATA_TEMPERATURE || abs(hum - rtcData.hum) >= THRESHOLD_SEND_DATA_HUMIDITY || rtcData.counter * INTERVAL_READ_SENSOR_SECS >= INTERVAL_SEND_DATA_SECS)
	{
		Logger.log("Threshold exceeded, resetting to send data...");

		rtcData.temp = temp;
		rtcData.hum = hum;
		rtcData.counter = 0;
		saveRtcData();
	}
	else
	{
		saveRtcData();
		deepSleep(INTERVAL_READ_SENSOR_SECS);
	}
}

void sendSensorData()
{
	HTTPClient http;
	http.begin(callback + ESP.getVcc() + "/" + (millis() - start) + "/" + rtcData.temp + "/" + rtcData.hum);
	http.GET();
}

void configureWiFi()
{
	IPAddress local_IP(192, 168, 178, 49);
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
		Logger.log("Could not connect to WiFi. Channel changed?");
		configureWiFi();
		deepSleep(WIFI_RECONNECT_WAITTIME_SECS);
	}

	Logger.log("...Connected! IP Address: %s", WiFi.localIP().toString().c_str());
	Logger.log("DNS IP Address: %s", WiFi.dnsIP().toString().c_str());
	Logger.log(" Signal strength: %i", WiFi.RSSI());
}

void setup()
{
	Serial.begin(74880);
	Serial.println("Setup");
	startedAtMs = millis();

	// Each of the below functions can set the ESP to deep sleep mode which stops further execution.
	readSensorData();
	connectToWifi();
	sendSensorData();
	deepSleep(INTERVAL_READ_SENSOR_SECS);
}

void loop()
{
	deepSleep(INTERVAL_READ_SENSOR_SECS);
}
