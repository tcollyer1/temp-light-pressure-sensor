// TAREN COLLYER

#include "ITick.h"
#include "mbed.h"

// Class for Mbed ticker using interface
class MbedTicker : public ITick {
    private:
        Ticker tm;

    public:
        virtual void attachFunc(void (*func1)(), std::chrono::microseconds time) {
            tm.attach(func1, time);
        }
};