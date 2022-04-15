/*
 * Copyright (c) 2020 Arm Limited
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mbed.h"
#include "uop_msb.h"
#include "rtos/ThisThread.h"
#include "NTPClient.h"
#include "azure_c_shared_utility/xlogging.h"
#include <cstring>
#include <string.h>
#include <iostream>
#include <string>
#include "SDBlockDevice.h"
#include "FATFileSystem.h"

using namespace uop_msb;
using namespace std;

extern void azureDemo();
extern NetworkInterface *_defaultSystemNetwork;

// Interface for red LED
class ILED {
    public:
        virtual void lightOn() = 0;
        virtual void lightOff() = 0;
};

// Class for Mbed red LED using interface
class MbedLight : public ILED {
    private:
        DigitalOut led;

    public:
        MbedLight(PinName pin, int state) : led(pin, state) {

        }

        virtual void lightOn() {
            led = 1;
        }

        virtual void lightOff() {
            led = 0;
        }
};

// Interface for ticker
class ITick {
    public:
        virtual void attachFunc(void (*func1)(), std::chrono::microseconds time) = 0;
};

// Class for Mbed ticker using interface
class MbedTicker : public ITick {
    private:
        Ticker tm;

    public:
        virtual void attachFunc(void (*func1)(), std::chrono::microseconds time) {
            tm.attach(func1, time);
        }
};

AnalogIn ldr(AN_LDR_PIN);


// Sensor data class - for requirement 1
class SensorData {
    private:
        float temperature;
        float pressure;
        float lightLevel;

    public:
        void setSensorReadings() {
            EnvSensor sensor;
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
};


#ifdef USE_SD_CARD
#include "SDBlockDevice.h"
#include "FATFileSystem.h"
#endif
SDBlockDevice card(PB_5, PB_4, PB_3, PF_3);

// Buffer class - for requirement 3
class Buffer {
    private:
        Queue<SensorData, 10> buffer;
        ILED &redLED;

    public:
        Buffer(ILED &led) : redLED(led) {}

        void writeToBuffer(SensorData item) {
            // Write to FIFO buffer
            bool sent = buffer.try_put(&item);

            // Error occurred
            if (!sent) {
                printf("\n[!] Data could not be added to the buffer.\n");
            }

            else {
                printf("Values added to buffer!\n");
            }

            // Buffer full, light red LED
            if (buffer.full()) {
                printf("Buffer is full!\n\n");
                redLED.lightOn();
            }
        }

        void readFromBuffer() {
            SensorData* values;
            bool success = buffer.try_get_for(10s, &values); // Blocks for 10 secs if buffer empty

            if (success) {               
                // Write value to the file.
                int err;

                err = card.init();

                if (0 != err) {
                    printf("Card init failed: %d\n",err);
                }

                else {
                    FATFileSystem fs("sd", &card);
                    FILE *fp = fopen("/sd/test.txt","w");

                    if(fp == NULL) {
                        error("Could not open file for write\n");
                        card.deinit();

                    } else {
                        // Add readings to file
                        for (int i = 0; i <= buffer.count(); i++) {
                            fprintf(fp, "Temperature: %f\nPressure: %f\nLight levels: %f\n\n", values[i].fetchTemperature(), values[i].fetchPressure(), values[i].fetchLightLevel());
                        }
                        
                        fclose(fp);
                        printf("SD write done...\n");
                        card.deinit();
                    }
                }
            }

            else {
                cout << "Timeout...\n";
            }
        }
};


// Timers
MbedTicker tr1;
MbedTicker tr2;

ITick &timer = tr1;
ITick &timer2 = tr2;


// Function declarations
void getSensorData();
void setFlags();
void setFlags2();
void readBuffer();

// Mbed class objects for LED
MbedLight red(PC_2, 0);
ILED &redLED = red;

// Buffer
Buffer valuesBuffer(redLED);

// Threads
Thread t1(osPriorityAboveNormal);
Thread t2(osPriorityNormal);

bool connect()
{
    LogInfo("Connecting to the network");

    _defaultSystemNetwork = NetworkInterface::get_default_instance();
    if (_defaultSystemNetwork == nullptr) {
        LogError("No network interface found");
        return false;
    }

    int ret = _defaultSystemNetwork->connect();
    if (ret != 0) {
        LogError("Connection error: %d", ret);
        return false;
    }
    LogInfo("Connection success, MAC: %s", _defaultSystemNetwork->get_mac_address());
    return true;
}

bool setTime()
{
    LogInfo("Getting time from the NTP server");

    NTPClient ntp(_defaultSystemNetwork);
    ntp.set_server("time.google.com", 123);
    time_t timestamp = ntp.get_timestamp();
    if (timestamp < 0) {
        LogError("Failed to get the current time, error: %ud", timestamp);
        return false;
    }
    LogInfo("Time: %s", ctime(&timestamp));
    set_time(timestamp);
    return true;
}



int main() {

    // START - UNCOMMENT THE FOLLOWING TWO LINES TO TEST YOUR BOARD AND SEE THE DEMO CODE WORKING
    //UOP_MSB_TEST  board;  //This class is purely for testing. Do no use it otherwise!!!!!!!!!!!
    //board.test();         //Look inside here to see how this works
    // END

    timer.attachFunc(&setFlags, 10s);
    timer2.attachFunc(&setFlags2, 60s);

    t1.start(getSensorData);
    t2.start(readBuffer);

    if (!connect()) return -1;

    if (!setTime()) return -1;

    // The two lines below will demonstrate the features on the MSB. See uop_msb.cpp for examples of how to use different aspects of the MSB
    // UOP_MSB_TEST board;  //Only uncomment for testing - DO NOT USE OTHERWISE
    // board.test();        //Only uncomment for testing - DO NOT USE OTHERWISE

    // Write fake data to Azure IoT Center. Don't forget to edit azure_cloud_credentials.h
    printf("You will need your own connection string in azure_cloud_credentials.h\n");
    LogInfo("Starting the Demo");
    azureDemo();
    LogInfo("The demo has ended");

    return 0;
}

void getSensorData() {
    SensorData data;

    while (true) {
        // Temperature, Light Levels & Pressure
        float temp, pres, light;

        data.setSensorReadings();

        temp = data.fetchTemperature();
        pres = data.fetchPressure();
        light = data.fetchLightLevel();


        ThisThread::flags_wait_any(1);
        printf("\n10 secs passed, running sensor data collection!\n");

        // Write to terminal for now.
        cout << "Temperature: " << temp << "\n";
        cout << "Pressure: " << pres << "\n";
        cout << "Light levels: " << light << "\n";

        // Write to buffer
        valuesBuffer.writeToBuffer(data);
    }
}

// #ifdef USE_SD_CARD
// #include "SDBlockDevice.h"
// #include "FATFileSystem.h"
// #endif
// SDBlockDevice card(PB_5, PB_4, PB_3, PF_3);

void readBuffer() {
    while (true) {
        ThisThread::flags_wait_any(2);
        printf("1 minute passed, trying to get buffer and write to file!\n");
        valuesBuffer.readFromBuffer();
    }
}

void setFlags() {
    t1.flags_set(1);
}

void setFlags2() {
    t2.flags_set(2);
}