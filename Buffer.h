// TAREN COLLYER

#include "SDBlockDevice.h"
#include "FATFileSystem.h"
#include "MbedLight.h"
#include "SensorData.h"
#include <iostream>

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

            // else {
            //     printf("Values added to buffer!\n");
            // }

            // Buffer full, light red LED
            if (buffer.full()) {
                printf("Buffer is full!\n\n");
                redLED.lightOn();
            }

            else {
                redLED.lightOff();
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
                            time_t curr = values[i].fetchDateTime();

                            fprintf(fp, "Temperature: %f\nPressure: %f\nLight levels: %f\nDate/time: %s\n\n", values[i].fetchTemperature(), values[i].fetchPressure(), values[i].fetchLightLevel(), ctime(&curr));
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