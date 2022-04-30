// TAREN COLLYER

#include "ITick.h"
#include "mbed.h"
#include <chrono>

// Class for Mbed ticker using interface
class MbedTicker : public ITick<std::chrono::microseconds> {
    private:
        Ticker tm;

    public:
        virtual void attachFunc(void (*func1)(), std::chrono::microseconds time) {
            tm.attach(func1, time);
        }

        virtual void detachFunc() {
            tm.detach();
        }
};