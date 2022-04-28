/*
 * Copyright (c) 2020 Arm Limited
 * SPDX-License-Identifier: Apache-2.0
 */

// #include "mbed.h"
#include "uop_msb.h"
#include "rtos/ThisThread.h"
// #include "NTPClient.h"
#include "azure_c_shared_utility/xlogging.h"
#include <chrono>
#include <cstring>
#include <ctime>
#include <string.h>
// #include <iostream>
#include <string>
// #include "SDBlockDevice.h"
// #include "FATFileSystem.h"

// #include "SensorData.h"
// #include "Buffer.h"
#include "SDWrite.h"

#include "MbedTicker.h"
#include "MbedButton.h"
// #include "MbedTimer.h"

#include "AzureIoT.h"

using namespace uop_msb;
using namespace std;

extern void azureDemo();


// Timers
MbedTicker tr1;
MbedTicker tr2;
MbedTicker tr3;

ITick<std::chrono::microseconds> &timer = tr1;
ITick<std::chrono::microseconds> &timer2 = tr2;
ITick<std::chrono::microseconds> &timer3 = tr3;


// Function declarations
void getSensorData();
void setFlags();
void setFlags2();
void readBuffer();
bool outsideThreshold(float t, float l, float p);
void writeAlarmMsg();
void waitForBtnPress();
void waitOneMinute();
void setFlags3();
void sendToAzure();
void sendData();
void setFlags4();

// Azure remote functions
SensorData<float, time_t> latest();
// SensorData[] buffered();


// Mbed class objects for LED
MbedLight red(PC_2, 0);
ILED &redLED = red;
// MbedLDR the_ldr(AN_LDR_PIN);
// ILightReadings<AnalogIn> &light_readings = the_ldr;


// Buffer
Buffer valuesBuffer(redLED);

AzureIoT aaa;

// Threads
Thread producer(osPriorityAboveNormal);
Thread consumer(osPriorityNormal);
Thread t3(osPriorityNormal);
Thread t4(osPriorityNormal);


// Upper and lower limits for pressure, light and temperature
float TUpper = 25.1; // was 22
float TLower = 25.0; // was 12

float PUpper = 1016.0;
float PLower = 1015.06;

float LUpper = 0.55;
float LLower = 0.22;

// Flag for the button ticker
int ONE_MINUTE_FLAG = 3;


// Bool to determine whether to show alarm or not
bool showAlarm = true;


int main() {

    // START - UNCOMMENT THE FOLLOWING TWO LINES TO TEST YOUR BOARD AND SEE THE DEMO CODE WORKING
    //UOP_MSB_TEST  board;  //This class is purely for testing. Do no use it otherwise!!!!!!!!!!!
    //board.test();         //Look inside here to see how this works
    // END

    timer.attachFunc(&setFlags, 10s);
    timer2.attachFunc(&setFlags2, 60s);
    
    producer.start(getSensorData); // 1st thread for acquiring the sensor data (high priority) and writing to buffer (producer)
    consumer.start(readBuffer); // 2nd thread for reading buffer and writing to SD (consumer)
    t3.start(waitForBtnPress); // 3rd thread for listening for blue button press - for user to cancel alarm message
    // t4.start(sendToAzure);
    

    // Azure azure;
    // if (!azure.connect()) return -1;
    // if (!azure.setTime()) return -1;


    

    





    // The two lines below will demonstrate the features on the MSB. See uop_msb.cpp for examples of how to use different aspects of the MSB
    // UOP_MSB_TEST board;  //Only uncomment for testing - DO NOT USE OTHERWISE
    // board.test();        //Only uncomment for testing - DO NOT USE OTHERWISE

    // Write fake data to Azure IoT Center. Don't forget to edit azure_cloud_credentials.h
    // printf("You will need your own connection string in azure_cloud_credentials.h\n");
    // LogInfo("Starting the Demo");
    // // azureDemo();
    // LogInfo("The demo has ended");

    return 0;
}

// Acquires sensor data using SensorData class, running every 10 seconds.
// Displays alarm if readings fall outside thresholds and writes readings to the buffer.
void getSensorData() {
    SensorData<float, time_t> data;

    while (true) {
        printf("\nSensor data loop starting again\n");
        // Temperature, Light Levels & Pressure
        float temp, pres, light;
        time_t dateTime;
        //MbedLDR my_ldr(AN_LDR_PIN);
        //ILightReadings<AnalogIn> &ldr_pin = my_ldr;

        // data.setSensorReadings(ldr_pin);
        data.setSensorReadings();

        temp = data.fetchTemperature();
        pres = data.fetchPressure();
        light = data.fetchLightLevel();
        dateTime = data.fetchDateTime();


        ThisThread::flags_wait_any(1);
        printf("\n10 secs passed, running sensor data collection!\n");

        printf("Temperature: %f\nPressure: %f\nLight levels: %f\nDate and time: %s\n", temp, pres, light, ctime(&dateTime));

        if (outsideThreshold(temp, light, pres) && showAlarm) {
            writeAlarmMsg();
        }

        // aaa.demo(light, temp, pres);

        // Write to buffer
        valuesBuffer.writeToBuffer(data);

        // setFlags4();

        Timer tee;
        int elapsed = tee.elapsed_time().count();
        cout << elapsed;
    }
}

void sendToAzure() {
    while (true) {
        ThisThread::flags_wait_any(4);
        //SensorData<float, time_t, AnalogIn> latestData = latest();
        //aaa.demo(latestData.fetchLightLevel(), latestData.fetchTemperature(),latestData.fetchPressure());
        //aaa.demo(0.23, 22.3, 1006.22);
    }
    
}



// Reads values currently in the buffer and writes these in the SD card. Triggers every minute.
void readBuffer() {
    SDWrite sdObj;
    
    while (true) {
        ThisThread::flags_wait_any(2);
        printf("1 minute passed, trying to get buffer and write to file!\n");

        //values = valuesBuffer.readFromBuffer(values, success);
        int bufferLength = valuesBuffer.bufferCount();
        SensorData<float, time_t> contents[bufferLength];
        
        for (int i = 0; i < bufferLength; i++) {
            contents[i] = valuesBuffer.readFromBuffer();
        }
        
        sdObj.writeToSD(contents, bufferLength);
    }
}

// Sets flags for the first timer - collecting sensor data every 10 seconds.
void setFlags() {
    producer.flags_set(1);
}

// Sets flags for the second timer - reading from the buffer and writing to the SD card every minute.
void setFlags2() {
    consumer.flags_set(2);
}

// Takes temperature, light and pressure readings as parameters.
// Checks if they are outside upper or lower thresholds.
// Returns either true or false.
bool outsideThreshold(float t, float l, float p) {
    if (t > TUpper || t < TLower || l > LUpper || l < LLower || p > PUpper || p < PLower) {
        return true;
    }

    else {
        return false;
    }
}

// Writes alarm message to the terminal.
void writeAlarmMsg() {
    printf("\n*** [!!!] WARNING: sensor measurement outside threshold values! ***\n");
}

// Function that waits for user to press the blue button. Runs on thread 3 (t3).
// The code uses signal-wait to wait on user button press.
// It then uses a timer interrupt after the minute has passed to re-enable the alarm.
void waitForBtnPress() {
    MbedButton btn(USER_BUTTON);
    IButton &blueBtn = btn;

    MbedTicker time;
    ITick<std::chrono::microseconds> &timer3 = time;

    while (true) {
        blueBtn.waitForBtnPress();
        ThisThread::sleep_for(50ms);
        blueBtn.waitForBtnRise();

        printf("[!] Alarm cancelled for 1 minute.\n");
        showAlarm = false;

        ThisThread::sleep_for(50ms); // switch debounce

        timer3.attachFunc(&setFlags3, 60s);
        waitOneMinute();

        printf("[!] Alarm reenabled\n");

        timer3.detachFunc();

        showAlarm = true;
    }
}

void waitOneMinute() {
    ThisThread::flags_wait_any(ONE_MINUTE_FLAG);
}

void setFlags3() {
    t3.flags_set(ONE_MINUTE_FLAG);
}

void setFlags4() {
    t4.flags_set(4);
}

SensorData<float, time_t> latest() {
    SensorData<float, time_t> latest;
    
    latest = valuesBuffer.readFromBuffer();
    
    return latest;
}