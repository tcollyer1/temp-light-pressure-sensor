// TAREN COLLYER

// Interface for blue button
class IButton {
    public:
        //virtual bool btnPressed() = 0;
        virtual void waitForBtnRise() = 0;
        virtual void waitForBtnPress() = 0;
};