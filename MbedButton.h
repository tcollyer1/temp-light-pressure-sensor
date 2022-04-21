// TAREN COLLYER

#include "IButton.h"
#include "mbed.h"
#include "PushSwitch.h"

// Class to handle Mbed button presses
class MbedButton : public IButton {
    private:
        //DigitalIn button;
        PushSwitch button;

    public:
        MbedButton(PinName pin) : button(pin) {}

        // virtual bool btnPressed() {
        //     if (button == 1) {
        //         return true;
        //     }

        //     else {
        //         return false;
        //     }
        // }

        virtual void waitForBtnPress() {
            button.waitForPress();
        }

        virtual void waitForBtnRise() {
            button.waitForRelease();
        }
};