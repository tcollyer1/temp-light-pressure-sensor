// TAREN COLLYER

#include "ILED.h"
#include "mbed.h"

// Class for Mbed LEDs using interface
class MbedLight : public ILED {
    private:
        DigitalOut led;

    public:
        MbedLight(PinName pin, int state) : led(pin, state) {}

        virtual void lightOn() {
            led = 1;
        }

        virtual void lightOff() {
            led = 0;
        }
};