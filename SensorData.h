// TAREN COLLYER

#include "AzureServer.h"
#include "uop_msb.h"
//#include "ILightReadings.h"
#include "MbedLDR.h"

// Sensor data class - for requirement 1
template<class SensorType, class DateType>
class SensorData {
    private:
        SensorType temperature;
        SensorType pressure;
        SensorType lightLevel;
        DateType dateTime;

        DateType acquireDateTime() {
            NTPClient ntp(_defaultSystemNetwork);
            ntp.set_server("time.google.com", 123);
            DateType timestamp = ntp.get_timestamp();

            return timestamp;
        }

    public:
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

        float fetchTemperature() {
            return this->temperature;
        }

        float fetchPressure() {
            return this->pressure;
        }

        float fetchLightLevel() {
            return this->lightLevel;
        }

        time_t fetchDateTime() {
            return this->dateTime;
        }
};