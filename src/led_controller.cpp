#include "led_controller.h"
#include "config.h"

LEDController::LEDController() : pixel(NUM_PIXELS, RGB_LED_PIN, NEO_GRB + NEO_KHZ800)
{
    currentRed = 0;
    currentGreen = 0;
    currentBlue = 0;
}

void LEDController::init()
{
    pixel.begin();
    pixel.setBrightness(128);
    setDisconnectedIndicator();
}

void LEDController::setColor(int red, int green, int blue)
{
    currentRed = red;
    currentGreen = green;
    currentBlue = blue;
    pixel.setPixelColor(0, pixel.Color(red, green, blue));
    pixel.show();
}

void LEDController::setConnectingIndicator()
{
    static bool ledState = false;
    ledState = !ledState;
    pixel.setPixelColor(0, ledState ? pixel.Color(0, 0, 255) : pixel.Color(0, 0, 0));
    pixel.show();
}

void LEDController::setConnectedIndicator()
{
    pixel.setPixelColor(0, pixel.Color(0, 255, 0));
    pixel.show();
}

void LEDController::setDisconnectedIndicator()
{
    pixel.setPixelColor(0, pixel.Color(255, 0, 0));
    pixel.show();
}

void LEDController::setSensorIndicator(int sensorValue)
{
    if (sensorValue == 1)
    {
        pixel.setPixelColor(0, pixel.Color(0, 0, 255)); // Blue
    }
    else
    {
        pixel.setPixelColor(0, pixel.Color(255, 0, 0)); // Red
    }
    pixel.show();
}

void LEDController::getCurrentColor(int &red, int &green, int &blue)
{
    red = currentRed;
    green = currentGreen;
    blue = currentBlue;
}