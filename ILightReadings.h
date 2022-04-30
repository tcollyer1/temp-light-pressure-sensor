// TAREN COLLYER

// Interface for getting light readings (LDR)
template<class ReturnType>
class ILightReadings {
    public:
        // Returns LDR light reading.
        virtual ReturnType getLightReading() = 0;
};