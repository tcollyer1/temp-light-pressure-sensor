// TAREN COLLYER

#include <chrono>

// Interface for ticker
class ITick {
    public:
        virtual void attachFunc(void (*func1)(), std::chrono::microseconds time) = 0;
};