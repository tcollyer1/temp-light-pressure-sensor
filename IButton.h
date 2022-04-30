// TAREN COLLYER

// Interface for button presses
class IButton {
    public:
        //virtual bool btnPressed() = 0;
        virtual void waitForBtnRise() = 0;
        virtual void waitForBtnPress() = 0;
};