// TAREN COLLYER

#include "IButton.h"
#include "mbed.h"

// Class to handle Mbed button presses
class MbedButton : public IButton {
    private:
        DigitalIn button;

    public:
        MbedButton(PinName pin) : button(pin) {}

        virtual bool btnPressed() {
            if (button == 1) {
                return true;
            }

            else {
                return false;
            }
        }
};