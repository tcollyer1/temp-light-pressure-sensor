// TAREN COLLYER
#include "mbed.h"

class PushSwitch {
    private:
        int RISING_EVENT = 1, FALLING_EVENT = 2;
        InterruptIn switchInterrupt;
        osThreadId_t threadID;

        void switchPressed() {
            switchInterrupt.rise(NULL);
            osSignalSet(threadID, RISING_EVENT);
        }

        void switchReleased() {
            switchInterrupt.fall(NULL);
            osSignalSet(threadID, FALLING_EVENT);
        }
    
    public:
        PushSwitch(PinName pin) : switchInterrupt(pin) {
            threadID = ThisThread::get_id();
        }

        ~PushSwitch() {
            switchInterrupt.rise(NULL);
            switchInterrupt.fall(NULL);
        }

        void waitForPress() {       
            switchInterrupt.rise(callback(this, &PushSwitch::switchPressed));
            ThisThread::flags_wait_any(RISING_EVENT);      
        }

        void waitForRelease() {       
            switchInterrupt.fall(callback(this, &PushSwitch::switchReleased));
            ThisThread::flags_wait_any(FALLING_EVENT);      
        }
};