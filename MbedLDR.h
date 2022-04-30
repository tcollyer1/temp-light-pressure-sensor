// TAREN COLLYER

#include "ILightReadings.h"
#include "mbed.h"

// Class for AnalogIn for the LDR
class MbedLDR : public ILightReadings<AnalogIn> {
    private:
        AnalogIn ldr;

    public:
        MbedLDR(PinName pin_name) : ldr(pin_name) {}

        virtual AnalogIn getLightReading() {
            return ldr;
        }
};