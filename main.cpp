/*
 * Copyright (c) 2020 Arm Limited
 * SPDX-License-Identifier: Apache-2.0
 */

#include "rtos/ThisThread.h"

#include "certs.h"
#include "iothub.h"
#include "iothub_client_options.h"
#include "iothub_device_client.h"
#include "iothub_message.h"
#include "azure_c_shared_utility/shared_util_options.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/tickcounter.h"
#include "azure_c_shared_utility/xlogging.h"
#include "SDWrite.h"

#include "iothubtransportmqtt.h"
#include "azure_cloud_credentials.h"
#include <cstring>
#include <string.h>


#include "uop_msb.h"
#include <chrono>
#include <cstring>
#include <ctime>
#include <string.h>
#include <string>
#include <stdlib.h> // For char* to float conversion

#include "MbedButton.h"


using namespace uop_msb;
using namespace std;

extern void azureDemo();

// Semaphores
// Critical error handling

// Sensor Limits
float TUpper = 25.1; // (originally 22)
float TLower = 25.0; // (originally 12)

float PUpper = 1016.0;
float PLower = 1015.06;

float LUpper = 0.55;
float LLower = 0.22;


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
void set_flag_collect_sensor_data();
void set_flag_read_and_write();
void readBuffer();
void writeAlarmMsg();
void waitForBtnPress();
void waitOneMinute();
void wait30Seconds();
void set_flag_unblock_button_thread();
void sendToAzure();
void sendData();
void setFlags4();
void set_consumer_alarm_timer_flag();
void set_bufferFlush_flag();
void set_flag_flush_complete();
void setFlags8();
void set_azure_consumer_alarm_flag();
void bufferFlush();
void writeToAzureBuffer();

// Azure remote functions
SensorData<float, time_t> latest();
int buffered();
void flush(bool &err);
void set_high_pressure(float p);
void set_low_pressure(float p);
void set_high_temperature(float t);
void set_low_temperature(float t);
void set_high_light(float l);
void set_low_light(float l);


// Mbed class objects for LED - uses interface
MbedLight red(PC_2, 0);
ILED &redLED = red;

// Buffers
Buffer valuesBuffer(redLED);
// Buffer azureBuffer(redLED);

Azure azure;

// Threads
Thread producer(osPriorityHigh);
Thread consumer(osPriorityNormal);
Thread button_handler(osPriorityNormal);
Thread azure_handler(osPriorityNormal);
Thread azure_consumer(osPriorityNormal);


// Bool to determine whether to show alarm or not
bool showAlarm = true;

// Bool that determines whether a critical error has been encountered or not
bool criticalError = false;

// Bool to determine whether data has been successfully written to the SD or not
bool notWritten = false;


/////////////////////////////////////////////////

SensorData<float, time_t> latest() {
    SensorData<float, time_t> latest;
    
    latest = valuesBuffer.peekFromBuffer();
    // latest = azureBuffer.readFromBuffer();
    
    return latest;
}

int buffered() {
    int count = valuesBuffer.bufferCount();
    // int count = azureBuffer.bufferCount();

    return count;
}

void flush() {
    // int size = azureBuffer.bufferCount();
    int size = valuesBuffer.bufferCount();
    SensorData<float, time_t> items[size];

    for (int i = 0; i < size; i++) {
        printf("\nGetting buffer item at index %d\n", i);
        items[i] = valuesBuffer.readFromBuffer();
        // items[i] = azureBuffer.readFromBuffer();
    }

    SDWrite sdWrite(redLED);
    sdWrite.writeToSD(items, size, criticalError);
    // err = criticalError;

        if (criticalError) {
            criticalError = false;
            sdWrite.print_alarm();

            timer4.attachFunc(&set_azure_consumer_alarm_flag, 30s);
            wait30Seconds();
            timer4.detachFunc();

            // Restart board here
            LogError("\nCould not write data to the SD card.\n");
            mbed_reset_reboot_error_info();
        }
}

void set_high_pressure(float p) {
    PUpper = p;
}

void set_low_pressure(float p) {
    PLower = p;
}

void set_high_temperature(float t) {
    TUpper = t;
}

void set_low_temperature(float t) {
    TLower = t;
}

void set_high_light(float l) {
    LUpper = l;
}

void set_low_light(float l) {
    LLower = l;
}


/////////////////////////////////////////////////

void bufferFlush() {
    while (true) {
        ThisThread::flags_wait_any(6);
        flush();
        // set_flag_flush_complete();
    }
}


// ********************************************************************************************************
// Global symbol referenced by the Azure SDK's port for Mbed OS, via "extern"
//NetworkInterface *_defaultSys;

static bool message_received = false;

static void on_connection_status(IOTHUB_CLIENT_CONNECTION_STATUS result, IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason, void* user_context)
{
    if (result == IOTHUB_CLIENT_CONNECTION_AUTHENTICATED) {
        LogInfo("Connected to IoT Hub");
    } else {
        LogError("Connection failed, reason: %s", MU_ENUM_TO_STRING(IOTHUB_CLIENT_CONNECTION_STATUS_REASON, reason));
    }
}

// **************************************
// * MESSAGE HANDLER (no response sent) *
// **************************************
//DigitalOut led2(LED2);
static IOTHUBMESSAGE_DISPOSITION_RESULT on_message_received(IOTHUB_MESSAGE_HANDLE message, void* user_context)
{
    LogInfo("Message received from IoT Hub");

    const unsigned char *data_ptr;
    size_t len;
    if (IoTHubMessage_GetByteArray(message, &data_ptr, &len) != IOTHUB_MESSAGE_OK) {
        LogError("Failed to extract message data, please try again on IoT Hub");
        return IOTHUBMESSAGE_ABANDONED;
    }

    message_received = true;
    LogInfo("Message body: %.*s", len, data_ptr);

    return IOTHUBMESSAGE_ACCEPTED;
}

static void on_message_sent(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void* userContextCallback)
{
    if (result == IOTHUB_CLIENT_CONFIRMATION_OK) {
        LogInfo("Message sent successfully");
    } else {
        LogInfo("Failed to send message, error: %s",
            MU_ENUM_TO_STRING(IOTHUB_CLIENT_CONFIRMATION_RESULT, result));
    }
}

// ****************************************************
// * COMMAND HANDLER (sends a response back to Azure) *
// ****************************************************
// DigitalOut led1(LED1); 
// DigitalIn blueButton(USER_BUTTON);
static int on_method_callback(const char* method_name, const unsigned char* payload, size_t size, unsigned char** response, size_t* response_size, void* userContextCallback)
{
    const char* device_id = (const char*)userContextCallback;

    printf("\r\nDevice Method called for device %s\r\n", device_id);
    printf("Device Method name:    %s\r\n", method_name);
    printf("Device Method payload: %.*s\r\n", (int)size, (const char*)payload);

    int status = 200;
    char RESPONSE_STRING[100];

    // REMOTE FUNCTION 1 - latest()
    if (strcmp("latest", method_name) == 0) {
        SensorData<float, time_t> latest_data = latest();

        // sprintf(RESPONSE_STRING, "{ \"Response\" : \"pressure\": %f, \"lightLevel\": %f, \"temperature\": %f }", latest_data.fetchPressure(), latest_data.fetchLightLevel(), latest_data.fetchTemperature());
        sprintf(RESPONSE_STRING, "{ \"Response\" : \"PRESSURE: %f, LIGHT LEVEL: %f, TEMPERATURE: %f\" }", latest_data.fetchPressure(), latest_data.fetchLightLevel(), latest_data.fetchTemperature());
    }

    // REMOTE FUNCTION 2 - buffered()
    else if (strcmp("buffered", method_name) == 0) {
        int items_buffered = buffered();

        sprintf(RESPONSE_STRING, "{ \"Response\" : %d }", items_buffered);
    }

    // REMOTE FUNCTION 3 - flush()
    else if (strcmp("flush", method_name) == 0) {
        set_bufferFlush_flag(); // Unblock azure_consumer thread to do the SD write

        sprintf(RESPONSE_STRING, "{ \"Response\" : \"Samples are being written to the SD.\" }");
    }

    // REMOTE FUNCTION 4 - set_low() (pressure)
    else if (strcmp("set_low_pressure", method_name) == 0) {
        set_low_pressure(atof((const char*)payload));

        sprintf(RESPONSE_STRING, "{ \"Response\" : \"PLower changed to %f\" }", PLower);
    }

    // REMOTE FUNCTION 4 - set_low() (temperature)
    else if (strcmp("set_low_temperature", method_name) == 0) {
        set_low_temperature(atof((const char*)payload));

        sprintf(RESPONSE_STRING, "{ \"Response\" : \"TLower changed to %f\" }", TLower);
    }

    // REMOTE FUNCTION 4 - set_low() (light levels)
    else if (strcmp("set_low_light", method_name) == 0) {
        set_low_light(atof((const char*)payload));

        sprintf(RESPONSE_STRING, "{ \"Response\" : \"LLower changed to %f\" }", LLower);
    }

    // REMOTE FUNCTION 5 - set_high() (pressure)
    else if (strcmp("set_high_pressure", method_name) == 0) {
        set_high_pressure(atof((const char*)payload));

        sprintf(RESPONSE_STRING, "{ \"Response\" : \"PUpper changed to %f\" }", PUpper);
    }

    
    // REMOTE FUNCTION 5 - set_high() (temperature)
    else if (strcmp("set_high_temperature", method_name) == 0) {
        set_high_temperature(atof((const char*)payload));

        sprintf(RESPONSE_STRING, "{ \"Response\" : \"TUpper changed to %f\" }", TUpper);
    }

    // REMOTE FUNCTION 5 - set_high() (light levels)
    else if (strcmp("set_high_light", method_name) == 0) {
        set_high_light(atof((const char*)payload));

        sprintf(RESPONSE_STRING, "{ \"Response\" : \"LUpper changed to %f\" }", LUpper);
    }


    printf("\r\nResponse status: %d\r\n", status);
    printf("Response payload: %s\r\n\r\n", RESPONSE_STRING);

    int rlen = strlen(RESPONSE_STRING);
    *response_size = rlen;
    if ((*response = (unsigned char*)malloc(rlen)) == NULL) {
        status = -1;
    }
    else {
        memcpy(*response, RESPONSE_STRING, *response_size);
    }
    return status;
}

void send_data() {
    bool trace_on = MBED_CONF_APP_IOTHUB_CLIENT_TRACE;
    tickcounter_ms_t interval = 100;
    IOTHUB_CLIENT_RESULT res;

    LogInfo("Initializing IoT Hub client");
    IoTHub_Init();



    IOTHUB_DEVICE_CLIENT_HANDLE client_handle = IoTHubDeviceClient_CreateFromConnectionString(
        azure_cloud::credentials::iothub_connection_string,
        MQTT_Protocol
    );
    if (client_handle == nullptr) {
        LogError("Failed to create IoT Hub client handle");
        goto cleanup;
    }

    // Enable SDK tracing
    res = IoTHubDeviceClient_SetOption(client_handle, OPTION_LOG_TRACE, &trace_on);
    if (res != IOTHUB_CLIENT_OK) {
        LogError("Failed to enable IoT Hub client tracing, error: %d", res);
        goto cleanup;
    }

    // Enable static CA Certificates defined in the SDK
    res = IoTHubDeviceClient_SetOption(client_handle, OPTION_TRUSTED_CERT, certificates);
    if (res != IOTHUB_CLIENT_OK) {
        LogError("Failed to set trusted certificates, error: %d", res);
        goto cleanup;
    }

    // Process communication every 100ms
    res = IoTHubDeviceClient_SetOption(client_handle, OPTION_DO_WORK_FREQUENCY_IN_MS, &interval);
    if (res != IOTHUB_CLIENT_OK) {
        LogError("Failed to set communication process frequency, error: %d", res);
        goto cleanup;
    }

    // set incoming message callback
    res = IoTHubDeviceClient_SetMessageCallback(client_handle, on_message_received, nullptr);
    if (res != IOTHUB_CLIENT_OK) {
        LogError("Failed to set message callback, error: %d", res);
        goto cleanup;
    }

    // Set incoming command callback
    res = IoTHubDeviceClient_SetDeviceMethodCallback(client_handle, on_method_callback, nullptr);
    if (res != IOTHUB_CLIENT_OK) {
        LogError("Failed to set method callback, error: %d", res);
        goto cleanup;
    }

    // Set connection/disconnection callback
    res = IoTHubDeviceClient_SetConnectionStatusCallback(client_handle, on_connection_status, nullptr);
    if (res != IOTHUB_CLIENT_OK) {
        LogError("Failed to set connection status callback, error: %d", res);
        goto cleanup;
    }

    // Send messages to the cloud
    // or until we receive a message from the cloud
    IOTHUB_MESSAGE_HANDLE message_handle;
    char message[80];
    while (true) {
        if (message_received) {
            // If we have received a message from the cloud, don't send more messages
            break;
        }

        // Signal wait - waits for a new item to be added to the buffer and then sends it by using the latest() function, which peeks the newest sensor data item.
        ThisThread::flags_wait_any(4);

        // Only send a message if the buffer is not empty
        // if (!azureBuffer.bufferIsEmpty()) {
            //SensorData<float, time_t> current = latest();

            
            // SensorData<float, time_t> current = azureBuffer.readFromBuffer();
            SensorData<float, time_t> current = valuesBuffer.peekFromBuffer();

            sprintf(message, "{ \"LightLevel\" : %f, \"Temperature\" : %f, \"Pressure\" : %f }", current.fetchLightLevel(), current.fetchTemperature(), current.fetchPressure());
            LogInfo("Sending: \"%s\"", message);

            message_handle = IoTHubMessage_CreateFromString(message);
            if (message_handle == nullptr) {
                LogError("Failed to create message");
                goto cleanup;
            }

            res = IoTHubDeviceClient_SendEventAsync(client_handle, message_handle, on_message_sent, nullptr);
            IoTHubMessage_Destroy(message_handle); // message already copied into the SDK

            if (res != IOTHUB_CLIENT_OK) {
                LogError("Failed to send message event, error: %d", res);
                goto cleanup;
            }
        // }

        
    }

cleanup:
    IoTHubDeviceClient_Destroy(client_handle);
    IoTHub_Deinit();
}
// ********************************************************************************************************


int main() {

    // START - UNCOMMENT THE FOLLOWING TWO LINES TO TEST YOUR BOARD AND SEE THE DEMO CODE WORKING
    // UOP_MSB_TEST  board;  //This class is purely for testing. Do no use it otherwise!!!!!!!!!!!
    // board.test();         //Look inside here to see how this works
    // END

    timer.attachFunc(&set_flag_collect_sensor_data, 10s);
    timer2.attachFunc(&set_flag_read_and_write, 60s);
    
    azure_handler.start(sendToAzure); // 4th thread, responsible for connecting to and sending data to Azure
    producer.start(getSensorData); // 1st thread for acquiring the sensor data (high priority) and writing to buffer (producer)
    consumer.start(readBuffer); // 2nd thread for reading buffer and writing to SD (consumer)
    button_handler.start(waitForBtnPress); // 3rd thread for listening for blue button press - for user to cancel alarm message
    azure_consumer.start(bufferFlush); // 5th thread for emptying buffer/writing to SD from Azure
    


    // Write fake data to Azure IoT Center. Don't forget to edit azure_cloud_credentials.h
    // printf("You will need your own connection string in azure_cloud_credentials.h\n");
    // LogInfo("Starting the Demo");
    // azureDemo();
    // LogInfo("The demo has ended");

    return 0;
}

// Acquires sensor data using SensorData class, running every 10 seconds.
// Displays alarm if readings fall outside thresholds and writes readings to 2 buffers - one for the SD and one for sending to Azure.
void getSensorData() {
    SensorData<float, time_t> data;

    while (true) {

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

        if (data.outsideThreshold(TUpper, TLower, LUpper, LLower, PUpper, PLower) && showAlarm) {
            writeAlarmMsg();
        }

        // Write to buffer
        valuesBuffer.writeToBuffer(data);
        // azureBuffer.writeToBuffer(data);

        setFlags4(); // Unblock Azure thread so newest buffer item can now be sent
    }
}

void sendToAzure() {
    if (azure.connect() && azure.setTime()) {
        send_data();
    }

    else {
        // Critical error
        redLED.lightOn();
        error("\n[!] Connection to Azure failed...\n");
    }
}



// Reads values currently in the buffer and writes these in the SD card. Triggers every minute.
void readBuffer() {
    SDWrite sdObj(redLED);
    
    while (true) {
        ThisThread::flags_wait_any(2);
        printf("1 minute passed, trying to get buffer and write to file!\n");

        int bufferLength = valuesBuffer.bufferCount();
        SensorData<float, time_t> contents[bufferLength];
        
        for (int i = 0; i < bufferLength; i++) {
            contents[i] = valuesBuffer.readFromBuffer();
        }
        
        sdObj.writeToSD(contents, bufferLength, criticalError);

        if (criticalError) {
            criticalError = false;
            sdObj.print_alarm();

            timer4.attachFunc(&set_consumer_alarm_timer_flag, 30s);
            wait30Seconds();
            timer4.detachFunc();

            LogError("\nCould not write data to the SD card.\n");
            mbed_reset_reboot_error_info();
        }
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
        ThisThread::sleep_for(50ms); // Switch debounce
        blueBtn.waitForBtnRise();

        printf("[!] Alarm cancelled for 1 minute.\n");
        showAlarm = false;

        ThisThread::sleep_for(50ms); // switch debounce

        timer3.attachFunc(&set_flag_unblock_button_thread, 60s);
        waitOneMinute();

        printf("[!] Alarm reenabled\n");

        timer3.detachFunc();

        showAlarm = true;
    }
}

// Sets flags for the first timer - collecting sensor data every 10 seconds.
void set_flag_collect_sensor_data() {
    producer.flags_set(1);
}

// Sets flags for the second timer - reading from the buffer and writing to the SD card every minute.
void set_flag_read_and_write() {
    consumer.flags_set(2);
}

// Makes the thread wait for its signal for one minute
void waitOneMinute() {
    ThisThread::flags_wait_any(3);
}

void wait30Seconds() {
    ThisThread::flags_wait_any(5);
}

void set_flag_unblock_button_thread() {
    button_handler.flags_set(3);
}

void setFlags4() {
    azure_handler.flags_set(4);
}

void set_consumer_alarm_timer_flag() {
    consumer.flags_set(5);
}

void set_bufferFlush_flag() {
    azure_consumer.flags_set(6);
}

void set_flag_flush_complete() {
    azure_handler.flags_set(7);
}

void set_azure_consumer_alarm_flag() {
    azure_consumer.flags_set(5);
}