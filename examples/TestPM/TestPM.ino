/*
This is modified code for the AirGradient library to read and average PM values.
Reads three samples 1 second apart, averages them, and sleeps for 5 seconds.
*/

#include <AirGradient.h>

#ifdef ESP8266
AirGradient ag = AirGradient(DIY_BASIC);
#else
AirGradient ag = AirGradient(ONE_INDOOR);
// AirGradient ag = AirGradient(OPEN_AIR_OUTDOOR);
#endif

enum SamplingState { WAITING, SAMPLING };
SamplingState samplingState = WAITING;
uint32_t lastSampleTime = 0;
uint8_t samplesCollected = 0;
uint32_t pmSum = 0;

void failedHandler(String msg);

void setup() {
  Serial.begin(115200);
#ifdef ESP8266
  if (ag.pms5003.begin(&Serial) == false) {
    failedHandler("Init PMS5003 failed");
  }
#else
  if (ag.getBoardType() == OPEN_AIR_OUTDOOR) {
    if (ag.pms5003t_1.begin(Serial0) == false) {
      failedHandler("Init PMS5003T failed");
    }
  } else {
    if (ag.pms5003.begin(Serial0) == false) {
      failedHandler("Init PMS5003 failed");
    }
  }
#endif
}

void loop() {
  // Handle sensor communication
  if (ag.getBoardType() == OPEN_AIR_OUTDOOR) {
    ag.pms5003t_1.handle();
  } else {
    ag.pms5003.handle();
  }

  // State machine for sampling
  switch (samplingState) {
    case WAITING:
      if (millis() - lastSampleTime >= 5000) {
        // Start sampling phase
        samplesCollected = 0;
        pmSum = 0;
        samplingState = SAMPLING;
        lastSampleTime = millis() - 1000; // First sample immediately
      }
      break;

    case SAMPLING:
      if (millis() - lastSampleTime >= 1000) {
        bool readResult = false;
        int PM2 = -1;

        // Read PM2.5 based on board type
        if (ag.getBoardType() == OPEN_AIR_OUTDOOR) {
          if (ag.pms5003t_1.connected()) {
            PM2 = ag.pms5003t_1.getPm25Ae();
            readResult = true;
          }
        } else {
          if (ag.pms5003.connected()) {
            PM2 = ag.pms5003.getPm25Ae();
            readResult = true;
          }
        }

        if (readResult) {
          pmSum += PM2;
          samplesCollected++;
          lastSampleTime = millis();

          if (samplesCollected >= 3) {
            // Calculate average and output
            int averagePM = pmSum / 3;
            Serial.printf("Average PM2.5: %d µg/m³\r\n", averagePM);
            
            // Calculate AQI
            int aqi;
            if (ag.getBoardType() == OPEN_AIR_OUTDOOR) {
              aqi = ag.pms5003t_1.convertPm25ToUsAqi(averagePM);
            } else {
              aqi = ag.pms5003.convertPm25ToUsAqi(averagePM);
            }
            Serial.printf("US AQI: %d\r\n", aqi);

            // Reset to waiting state
            samplingState = WAITING;
            lastSampleTime = millis();
          }
        } else {
          Serial.println("Sensor read failed, resetting...");
          samplingState = WAITING;
          lastSampleTime = millis();
        }
      }
      break;
  }
}

void failedHandler(String msg) {
  while (true) {
    Serial.println(msg);
    delay(1000);
  }
}
