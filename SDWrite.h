// TAREN COLLYER

#include "mbed.h"
#include "SDBlockDevice.h"
#include "FATFileSystem.h"
#include "Buffer.h"

SDBlockDevice card(PB_5, PB_4, PB_3, PF_3);

class SDWrite {
    private:

    public:
        void writeToSD(SensorData *values, int size) {
            //if (success) {               
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
                        //SensorData theValues = *values;
                        // Add readings to file
                        for (int i = 0; i <= size; i++) {
                            time_t curr = values[i].fetchDateTime();

                            printf("\nIt's writing this to the file:\nTemperature: %f\nPressure: %f\nLight levels: %f\nDate/time: %s\n\n", values[i].fetchTemperature(), values[i].fetchPressure(), values[i].fetchLightLevel(), ctime(&curr));

                            fprintf(fp, "Temperature: %f\nPressure: %f\nLight levels: %f\nDate/time: %s\n\n", values[i].fetchTemperature(), values[i].fetchPressure(), values[i].fetchLightLevel(), ctime(&curr));
                        }
                        
                        fclose(fp);
                        printf("SD write done...\n");
                        card.deinit();
                    }
                }
            //}

            // else {
            //     printf("Timeout...\n");
            // }
        }
};