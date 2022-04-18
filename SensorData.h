// TAREN COLLYER

#include "AzureServer.h"
#include "uop_msb.h"

// Sensor data class - for requirement 1
class SensorData {
    private:
        float temperature;
        float pressure;
        float lightLevel;
        time_t dateTime;

        time_t acquireDateTime() {
            NTPClient ntp(_defaultSystemNetwork);
            ntp.set_server("time.google.com", 123);
            time_t timestamp = ntp.get_timestamp();

            return timestamp;
        }

    public:
        void setSensorReadings() {
            uop_msb::EnvSensor sensor;
            // LDR for light readings
            AnalogIn ldr(AN_LDR_PIN);
            float temp, pres, light;            

            float temps[50];
            float pressures[50];
            float lightLevels[50];

            float tempSum = 0.0f;
            float presSum = 0.0f;
            float lightSum = 0.0f;

            for (int i = 0; i < 50; i++) { // Take 50 samples to minimise jitter and find the average.
                temps[i] = sensor.getTemperature();
                pressures[i] = sensor.getPressure();
                lightLevels[i] = ldr;

                tempSum += temps[i];
                presSum += pressures[i];
                lightSum += lightLevels[i];
            }

            temperature = tempSum / 50;
            pressure = presSum / 50;
            lightLevel = lightSum / 50;
            dateTime = acquireDateTime();
        }

        float fetchTemperature() {
            return temperature;
        }

        float fetchPressure() {
            return pressure;
        }

        float fetchLightLevel() {
            return lightLevel;
        }

        time_t fetchDateTime() {
            return dateTime;
        }
};