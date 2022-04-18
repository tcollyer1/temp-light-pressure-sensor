// TAREN COLLYER

#include "ITimer.h"
#include "mbed.h"

// Class for Mbed timer
class MbedTimer : public ITimer {
    private:
        Timer t;

    public:
        virtual void tmrStart() {
            t.start();
        }

        virtual void tmrStop() {
            t.start();
        }

        virtual void tmrReset() {
            t.reset();
        }

        virtual std::chrono::microseconds tmrElapsed() {
            return t.elapsed_time();
        }
};