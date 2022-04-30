/*
 * Copyright (c) 2020 Arm Limited
 * SPDX-License-Identifier: Apache-2.0
 */

#include "uop_msb.h"
#include "rtos/ThisThread.h"
#include "azure_c_shared_utility/xlogging.h"
#include <chrono>
#include <cstring>
#include <ctime>
#include <string.h>
#include <string>

#include "SDWrite.h"

// #include "MbedTicker.h"
#include "MbedButton.h"

#include "AzureIoT.h"

using namespace uop_msb;
using namespace std;

extern void azureDemo();

// Semaphores
// Send data to Azure
// Remote functions
// Critical error handling


// Timers - interface and virtual functions
MbedTicker tr1;
MbedTicker tr2;
MbedTicker tr3;
MbedTicker tr4;

ITick<std::chrono::microseconds> &timer = tr1;
ITick<std::chrono::microseconds> &timer2 = tr2;
ITick<std::chrono::microseconds> &timer3 = tr3;
ITick<std::chrono::microseconds> &timer4 = tr4;


// Function declarations
void getSensorData();
void setFlags();
void setFlags2();
void readBuffer();
void writeAlarmMsg();
void waitForBtnPress();
void waitOneMinute();
void wait30Seconds();
void setFlags3();
void sendToAzure();
void sendData();
void setFlags4();

// Azure remote functions
SensorData<float, time_t> latest();
// SensorData[] buffered();


// Mbed class objects for LED - uses interface
MbedLight red(PC_2, 0);
ILED &redLED = red;

// Buffer
Buffer valuesBuffer(redLED);

AzureIoT aaa;

// Threads
Thread producer(osPriorityHigh);
Thread consumer(osPriorityNormal);
Thread t3(osPriorityNormal);
Thread t4(osPriorityNormal);


// Bool to determine whether to show alarm or not
bool showAlarm = true;

// Bool that determines whether a critical error has been encountered or not
bool criticalError = false;


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
        // Temperature, light levels, pressure
        float temp, pres, light;

        // Date and time
        time_t dateTime;

        data.setSensorReadings();

        temp = data.fetchTemperature();
        pres = data.fetchPressure();
        light = data.fetchLightLevel();
        dateTime = data.fetchDateTime();


        ThisThread::flags_wait_any(1);
        printf("\n10 secs passed, running sensor data collection!\n");

        printf("Temperature: %f\nPressure: %f\nLight levels: %f\nDate and time: %s\n", temp, pres, light, ctime(&dateTime));

        if (data.outsideThreshold() && showAlarm) {
            writeAlarmMsg();
        }

        // aaa.demo(light, temp, pres);

        // Write to buffer
        valuesBuffer.writeToBuffer(data);

        // setFlags4();
    }
}

void sendToAzure() {
    while (true) {
        ThisThread::flags_wait_any(4);
        //SensorData<float, time_t> latestData = latest();
        //aaa.demo(latestData.fetchLightLevel(), latestData.fetchTemperature(),latestData.fetchPressure());
        //aaa.demo(0.23, 22.3, 1006.22);
    }
    
}



// Reads values currently in the buffer and writes these in the SD card. Triggers every minute.
void readBuffer() {
    SDWrite sdObj(redLED);
    
    while (true) {
        ThisThread::flags_wait_any(2);
        printf("1 minute passed, trying to get buffer and write to file!\n");

        //values = valuesBuffer.readFromBuffer(values, success);
        int bufferLength = valuesBuffer.bufferCount();
        SensorData<float, time_t> contents[bufferLength];
        
        for (int i = 0; i < bufferLength; i++) {
            contents[i] = valuesBuffer.readFromBuffer();
        }
        
        sdObj.writeToSD(contents, bufferLength, criticalError);

        if (criticalError) {
            sdObj.print_alarm();

            timer4.attachFunc(&setFlags4, 30s);
            wait30Seconds();
            timer4.detachFunc();

            // Restart board here
        }
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

// Makes the thread wait for its signal for one minute
void waitOneMinute() {
    ThisThread::flags_wait_any(3);
}

void wait30Seconds() {
    ThisThread::flags_wait_any(4);
}

void setFlags3() {
    t3.flags_set(3);
}

void setFlags4() {
    t4.flags_set(4);
}

void set_flags(Thread thread_name, int flag) {
    thread_name.flags_set(flag);
}

SensorData<float, time_t> latest() {
    SensorData<float, time_t> latest;
    
    latest = valuesBuffer.peekFromBuffer();
    
    return latest;
}