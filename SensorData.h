// TAREN COLLYER

#include "AzureServer.h"
#include "uop_msb.h"
#include "MbedLDR.h"

// Sensor data class - for requirement 1. Uses template so any numerical format of sensor measurements can be used.
template<class SensorType, class DateType>
class SensorData {
    private:
        // Default upper and lower limits for pressure, light and temperature
        

        SensorType temperature;
        SensorType pressure;
        SensorType lightLevel;
        DateType dateTime;

        // Gets the current date and time using NTP.
        DateType acquireDateTime() {
            NTPClient ntp(_defaultSystemNetwork);
            ntp.set_server("time.google.com", 123);
            DateType timestamp = ntp.get_timestamp();

            return timestamp;
        }

    public:

        // Collects a set of sensor readings and stores it within the SensorData object.
        void setSensorReadings() {
            uop_msb::EnvSensor sensor;
            
            // LDR for light readings - uses interface and template
            MbedLDR ldr_pin(AN_LDR_PIN);
            ILightReadings<AnalogIn> &ldr = ldr_pin;
            
            SensorType temp, pres, light;            

            SensorType temps[50];
            SensorType pressures[50];
            SensorType lightLevels[50];

            SensorType tempSum = 0;
            SensorType presSum = 0;
            SensorType lightSum = 0;

            for (int i = 0; i < 50; i++) { // Take 50 samples to minimise jitter and find the average.
                temps[i] = sensor.getTemperature();
                pressures[i] = sensor.getPressure();
                lightLevels[i] = ldr.getLightReading();

                tempSum += temps[i];
                presSum += pressures[i];
                lightSum += lightLevels[i];
            }

            this->temperature = tempSum / 50;
            this->pressure = presSum / 50;
            this->lightLevel = lightSum / 50;
            this->dateTime = acquireDateTime();
        }

        // Takes temperature, light and pressure readings as parameters.
        // Checks if they are outside upper or lower thresholds.
        // Returns either true or false.
        bool outsideThreshold(SensorType TUpper, SensorType TLower, SensorType LUpper, SensorType LLower, SensorType PUpper, SensorType PLower) {
            if (this->temperature > TUpper || this->temperature < TLower || this->lightLevel > LUpper || this->lightLevel < LLower || this->pressure > PUpper || this->pressure < PLower) {
                return true;
            }

            else {
                return false;
            }
        }

        // Gets stored temperature for this sensor data object.
        SensorType fetchTemperature() {
            return this->temperature;
        }

        // Gets stored pressure for this sensor data object.
        SensorType fetchPressure() {
            return this->pressure;
        }

        // Gets stored light levels for this sensor data object.
        SensorType fetchLightLevel() {
            return this->lightLevel;
        }

        // Gets stored date and time for this sensor data object.
        DateType fetchDateTime() {
            return this->dateTime;
        }
};