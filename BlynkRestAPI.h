/*
 * BlynkRestAPI.h
 *
 *  Created on: 27.01.2019
 *      Author: Daniel Chiaradia
 */

#ifndef BLYNKRESTAPI_H_
#define BLYNKRESTAPI_H_

#include <RestClient.h>
#include "define.h"
#include "Logger.h"

class BlynkRestAPI {
public:
	template<typename T>
	void sendValue(String pin, T value) {

		if (BLYNK_ACTIVATE_TRANSMISSION) {
			char str[150];
			sprintf(str, "/%s/update/%s?value=%s", BLYNK_AUTH, pin.c_str(),
					String(value).c_str());
			Logger.log("Call get on %s ", str);

			int statusCode = client.get(str);
			Logger.log("HTTP Status: %i", statusCode);
		}
	}

	void sendNotify(String message) {
		if (BLYNK_ACTIVATE_TRANSMISSION) {
			char str[message.length() + 40];
			sprintf(str, "/%s/notify", BLYNK_AUTH);

			Logger.log("Call notify on %s with message %s ", str,
					message.c_str());

			client.setContentType("application/json");
			String response = "";
			int statusCode = client.post(str, message.c_str(), &response);

			Logger.log("Retrieved status code %i with response %s", statusCode, response.c_str());
		}
	}

		int getIntValue(String pin) {
		if (BLYNK_ACTIVATE_TRANSMISSION) {
			char str[150];
			sprintf(str, "/%s/get/%s", BLYNK_AUTH, pin.c_str());
			Logger.log("Call get on %s ", str);

			String response = "";
			int statusCode = client.get(str, &response);

			Logger.log("HTTP Status: %i", statusCode);
			Logger.log("Response: %s", response.c_str());

			if (statusCode == 200) {
				String number = response.substring(response.indexOf('"') + 1, response.lastIndexOf('"'));
				Logger.log("Refresh Rate retrieved: %s", number.c_str());
				Logger.log("Refresh Rate parsed: %i", number.toInt());
				return number.toInt();
			}
		}

		return -1;
	}

private:
	RestClient client = RestClient(BLYNK_CLOUD_ADDRESS);
};


#endif /* BLYNKRESTAPI_H_ */
