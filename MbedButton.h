// TAREN COLLYER

#include "IButton.h"
#include "mbed.h"
#include "PushSwitch.h"

// Class to handle Mbed button presses
class MbedButton : public IButton {
    private:
        PushSwitch button;

    public:
        MbedButton(PinName pin) : button(pin) {}
        
        virtual void waitForBtnPress() {
            button.waitForPress();
        }

        virtual void waitForBtnRise() {
            button.waitForRelease();
        }
};