// TAREN COLLYER

#include "mbed.h"
#include "Buffer.h"
#include <chrono>
#include "MbedTicker.h"

SDBlockDevice card(PB_5, PB_4, PB_3, PF_3);

class SDWrite {
    private:
        ILED &redLED2;

        MbedTicker tick;
        ITick<std::chrono::microseconds> &t = tick;

    public:
        SDWrite(ILED &led) : redLED2(led) {}

        // Displays critical error alarm message in the event of a critical error.
        void print_alarm() {
            printf("\n[!] Critical error! [!]\n");
        }

        // Writes blocks of buffer contents to the mounted SD card.
        void writeToSD(SensorData<float, time_t> values[], int size, bool &criticalError) {
          int err;

          err = card.init();

          if (0 != err) {
            // Critical error - light red LED
            redLED2.lightOn();
            criticalError = true;

            printf("Card init failed: error %d\n", err);
          }

          else {
            FATFileSystem fs("sd", &card);
            FILE *fp = fopen("/sd/test.txt", "w");

            if (fp == NULL) {
              // Critical error - light red LED
              redLED2.lightOn();
              criticalError = true;

              printf("Could not open file for write\n");
              card.deinit();

            }
            
            else {
              // Add readings to file
              for (int i = 0; i < size; i++) {
                time_t curr = values[i].fetchDateTime();

                fprintf(fp,
                        "Temperature: %f\nPressure: %f\nLight levels: "
                        "%f\nDate/time: %s\n\n",
                        values[i].fetchTemperature(), values[i].fetchPressure(),
                        values[i].fetchLightLevel(), ctime(&curr));
              }

              fclose(fp);
              printf("SD write done...\n");
              card.deinit();
            }
          }
        }
};