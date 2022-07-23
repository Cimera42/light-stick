#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <NeoPixelBus.h>
#include <NeoPixelAnimator.h>
#include <FastLED.h>
// #include <FunctionalInterrupt.h>

// SPIClass *SPI1;
// static const int VSPI_MISO = 19;
// static const int VSPI_MOSI = 23;
// static const int VSPI_CS = 5;
// static const int VSPI_CLK = 18;

// static const int HSPI_MISO = 12;
// static const int HSPI_MOSI = 13;
// static const int HSPI_CS = 15;
// static const int HSPI_CLK = 14;

// static const int B_1 = 33;
// static const int B_2 = 32;
// static const int B_3 = 27;
// static const int B_4 = 26;
// static const int B_5 = 25;

static const int B_1 = 7;
// static const int B_2 = 32;

// #define NUM_LEDS 144
// #define LED_TYPE WS2811
// #define COLOR_ORDER GRB
// #define DATA_PIN 3
// //#define CLK_PIN       4
// #define VOLTS 5
// #define MAX_MA 1000

static const int UNO_SS = 10;

const uint16_t PixelCount = 144; // the sample images are meant for 144 pixels
const uint16_t PixelPin = 3;
const uint16_t AnimCount = 1; // we only need one

NeoPixelBus<NeoGrbFeature, NeoWs2812xMethod> strip(PixelCount, PixelPin);
NeoPixelAnimator animations(AnimCount); // NeoPixel animation management object

NeoBitmapFile<NeoGrbFeature, File> image;

uint16_t animState;

uint16_t MyLayoutMap(int16_t x, int16_t y)
{
    return y;
}

void LoopAnimUpdate(const AnimationParam &param)
{
    // wait for this animation to complete,
    // we are using it as a timer of sorts
    if (param.state == AnimationState_Completed)
    {

        // draw the complete row at animState to the complete strip
        // uint16_t h = image.Height();
        // for (uint16_t i = 0; i < h; i++)
        // {
        //     // image.Blt(strip, 0, 0, animState, image.Width());
        //     strip.SetPixelColor(i, image.GetPixelColor(animState, i));
        // }
        // image.Blt(strip, 0, 0, animState, 0, 1, image.Height(), MyLayoutMap);
        image.Blt(strip, 0, 0, animState, image.Width());
        // animState = (animState + 1) % image.Height(); // increment and wrap
        animState = animState + 1; // increment and wrap

        // done, time to restart this position tracking animation/timer
        if (animState < image.Height())
        {
            animations.RestartAnimation(param.index);
        }
        else
        {
            strip.ClearTo(RgbColor(0, 0, 0));
        }
    }
}

void button_isr(int n);

struct Button
{
    const uint8_t pin;
    bool pressed;
    unsigned long last_time;
    // std::function<void(void)> isr;
};

void b1_isr()
{
    button_isr(0);
}

Button buttons[] = {
    {B_1, false, 0},
    // {B_2, false, 0},
    // {B_3, false, 0},
    // {B_4, false, 0},
    // {B_5, false, 0},
};
const int buttonCount = sizeof(buttons) / sizeof(Button);

void button_isr(int n)
{
    auto button_time = millis();
    if (button_time - buttons[n].last_time > 250)
    {
        buttons[n].pressed = true;
        buttons[n].last_time = button_time;
    }
}

// CRGB leds[NUM_LEDS];

// #define AUTO_SELECT_BACKGROUND_COLOR 0

void setup()
{
    Serial.begin(115200);
    while (!Serial)
    {
        ; // wait for serial port to connect. Needed for native USB port only
    }

    // FastLED.addLeds<WS2811, DATA_PIN, GRB>(leds, NUM_LEDS);

    // strip.Begin();
    // strip.Show();

    for (int i = 0; i < buttonCount; i++)
    {
        pinMode(buttons[i].pin, INPUT_PULLUP);
        // attachInterrupt(
        //     buttons[i].pin, [i]()
        //     { button_isr(i); },
        //     FALLING);
        // attachInterrupt(
        //     buttons[i].pin, b1_isr,
        //     CHANGE);
    }

    // Serial.println("Initializing SPI...");
    // Serial.flush();
    // SPI1 = new SPIClass(SPI);

    Serial.println("Initializing SD card...");
    if (!SD.begin(UNO_SS))
    {
        Serial.println("initialization failed!");
        while (1)
            ;
    }
    else
    {
        Serial.println("initialization done.");
    }

    File bitmapFile = SD.open("p2p.bmp");
    if (!bitmapFile)
    {
        Serial.println("File open fail, or not present");
        // don't do anything more:
        return;
    }
    Serial.println("Image found");

    if (!image.Begin(bitmapFile))
    {
        Serial.println("File format fail, not a supported bitmap");
        // don't do anything more:
        return;
    }
    Serial.println("Image loaded");

    animState = 0;
    // // we use the index 0 animation to time how often we rotate all the pixels
    // animations.StartAnimation(0, 35, LoopAnimUpdate);

    strip.ClearTo(RgbColor(0, 0, 0));
    // strip.SetPixelColor(0, RgbColor(0, 0, 255));
    // strip.SetPixelColor(1, RgbColor(0, 255, 0));
    // strip.SetPixelColor(143, RgbColor(255, 0, 0));
}

void loop()
{
    if (digitalRead(buttons[0].pin) == LOW)
    {
        // Serial.println("Press");
        // buttons[0].pressed = false;

        // if (animations.IsAnimating())
        // {
        //     Serial.println("Stop");
        //     animations.StopAll();
        // }
        // else
        // {
        // Serial.println("Start");
        animState = 0;
        // animations.StartAnimation(0, 35, LoopAnimUpdate);
        animations.StartAnimation(0, 2500 / image.Height(), LoopAnimUpdate);
        // }
    }

    // Serial.println(SPI1.pinSS());

    animations.UpdateAnimations();
    strip.Show();
    // for (int whiteLed = 0; whiteLed < NUM_LEDS; whiteLed = whiteLed + 1)
    // {
    //     // Turn our current led on to white, then show the leds
    //     leds[whiteLed] = CRGB::Blue;

    //     // Show the leds (only one of which is set to white, from above)
    //     FastLED.show();

    //     // Wait a little bit
    //     delay(100);

    //     // Turn our current led back to black for the next loop around
    //     leds[whiteLed] = CRGB::Black;
    // }
}
