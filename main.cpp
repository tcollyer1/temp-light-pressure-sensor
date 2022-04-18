/*
 * Copyright (c) 2020 Arm Limited
 * SPDX-License-Identifier: Apache-2.0
 */

// #include "mbed.h"
#include "uop_msb.h"
#include "rtos/ThisThread.h"
// #include "NTPClient.h"
#include "azure_c_shared_utility/xlogging.h"
#include <cstring>
#include <ctime>
#include <string.h>
// #include <iostream>
#include <string>
// #include "SDBlockDevice.h"
// #include "FATFileSystem.h"

// #include "SensorData.h"
#include "Buffer.h"
#include "MbedTicker.h"

using namespace uop_msb;
using namespace std;

extern void azureDemo();


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
bool outsideThreshold(float t, float l, float p);
void writeAlarmMsg();


// Mbed class objects for LED
MbedLight red(PC_2, 0);
ILED &redLED = red;


// Buffer
Buffer valuesBuffer(redLED);


// Threads
Thread t1(osPriorityAboveNormal);
Thread t2(osPriorityNormal);


// Upper and lower limits for pressure, light and temperature
const float TUpper = 22.0;
const float TLower = 19.0;

const float PUpper = 1016.0;
const float PLower = 1015.06;

const float LUpper = 0.55;
const float LLower = 0.22;


int main() {

    // START - UNCOMMENT THE FOLLOWING TWO LINES TO TEST YOUR BOARD AND SEE THE DEMO CODE WORKING
    //UOP_MSB_TEST  board;  //This class is purely for testing. Do no use it otherwise!!!!!!!!!!!
    //board.test();         //Look inside here to see how this works
    // END

    timer.attachFunc(&setFlags, 10s);
    timer2.attachFunc(&setFlags2, 60s);

    t1.start(getSensorData);
    t2.start(readBuffer);

    Azure azure;
    if (!azure.connect()) return -1;
    if (!azure.setTime()) return -1;

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
        // std::time_t dateTime;
        time_t dateTime;

        data.setSensorReadings();

        temp = data.fetchTemperature();
        pres = data.fetchPressure();
        light = data.fetchLightLevel();
        dateTime = data.fetchDateTime();


        ThisThread::flags_wait_any(1);
        printf("\n10 secs passed, running sensor data collection!\n");

        // Write to terminal for now.
        cout << "Temperature: " << temp << "\n";
        cout << "Pressure: " << pres << "\n";
        cout << "Light levels: " << light << "\n";
        // cout << "Date and time: " << ctime(&dateTime) << "\n";
        cout << "Date and time: " << ctime(&dateTime) << "\n";

        if (outsideThreshold(temp, light, pres)) {
            writeAlarmMsg();
        }

        // Write to buffer
        valuesBuffer.writeToBuffer(data);
    }
}

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

bool outsideThreshold(float t, float l, float p) {
    if (t > TUpper || t < TLower || l > LUpper || l < LLower || p > PUpper || p < PLower) {
        return true;
    }

    else {
        return false;
    }
}

void writeAlarmMsg() {
    printf("\n\n\n*** [!!!] WARNING: sensor measurement outside threshold values!\n\n\n");
}