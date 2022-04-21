// TAREN COLLYER

#include "mbed.h"

class AzureIoT {
    private:
        float low_pres, low_temp, low_light, high_pres, high_temp, high_light;

    public:
        void latest() {

        }

        void buffered() {

        }

        void flush() {

        }

        void set_low(float p, float t, float l) {
            low_pres = p;
            low_temp = t;
            low_light = l;

            printf("\nLower thresholds set.\n");
        }

        void set_high(float p, float t, float l) {
            high_pres = p;
            high_temp = t;
            high_light = l;

            printf("\nUpper thresholds set.\n");
        }
};