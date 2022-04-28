// TAREN COLLYER

#include <chrono>

// Interface for ticker
template<class TickerMeasurement>
class ITick {
    public:
        virtual void attachFunc(void (*func1)(), TickerMeasurement time) = 0;
        virtual void detachFunc() = 0;
};