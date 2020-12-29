/*
 * Display.h
 *
 *  Created on: 10.02.2019
 *      Author: Daniel Chiaradia
 */

#ifndef DISPLAY_H_
#define DISPLAY_H_

#include <SPI.h>
#include <Wire.h>
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

class Display {
public:
	Display() {
	}

	void showValues(float temp, float hum) {
		init();
		Logger.log("Values %f %f", temp, hum);
		if (available) {
			display.setTextSize(2); // Draw 2X-scale text
			display.setTextColor(WHITE, BLACK);
			display.setCursor(62, 0);
			display.print(String(temp).c_str());
			display.setCursor(62, 16);
			display.print(String(hum).c_str());
			display.display();
			Logger.log("Wrote to display");
		}
		else {
			Logger.log("Display not available");
		}
	}

private:
	TwoWire wire = TwoWire();
	Adafruit_SSD1306 display = Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &wire);

	void init() {
		wire.begin(DISPLAY_SDA, DISPLAY_SCL);
		if (!initialized) {

			if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
				Logger.log("Display not found!");
				available = false;
			} else {
				display.clearDisplay();
				display.dim(true);

				display.setTextSize(2); // Draw 2X-scale text
				display.setTextColor(WHITE, BLACK);
				display.setCursor(0, 0);

				display.println("Temp:");
				display.println("Hum: ");
				display.display();

				available = true;
			}
		}
		initialized = true;
	}

	void clear() {
		init();
		if (available) {
			Logger.log("Clear display");
		}
	}
	bool available = false;
	bool initialized = false;
};

#endif /* DISPLAY_H_ */
