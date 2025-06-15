#ifndef LED_CONTROLLER_H
#define LED_CONTROLLER_H

#include <Adafruit_NeoPixel.h>

class LEDController
{
private:
    Adafruit_NeoPixel pixel;
    int currentRed;
    int currentGreen;
    int currentBlue;

public:
    LEDController();
    void init();
    void setColor(int red, int green, int blue);
    void setConnectingIndicator();
    void setConnectedIndicator();
    void setDisconnectedIndicator();
    void setSensorIndicator(int sensorValue);
    void getCurrentColor(int &red, int &green, int &blue);
};

#endif // LED_CONTROLLER_H