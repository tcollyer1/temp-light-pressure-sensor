// TAREN COLLYER

#include <chrono>

// Interface for Mbed timer
class ITimer {
    public:
        virtual void tmrStart() = 0;
        virtual void tmrStop() = 0;
        virtual void tmrReset() = 0;
        virtual std::chrono::microseconds tmrElapsed() = 0;
};