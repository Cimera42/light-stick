#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <NeoPixelBus.h>
#include <NeoPixelAnimator.h>
#include <Adafruit_SSD1306.h>
#include <vector>

// #define BUTTON_A  0
// #define BUTTON_B 16
// #define BUTTON_C  2
#define WIRE Wire

// SPIClass *SPI1;
// static const int VSPI_MISO = 19;
// static const int VSPI_MOSI = 23;
static const int VSPI_CS = 5;
// static const int VSPI_CLK = 18;

// static const int HSPI_MISO = 12;
// static const int HSPI_MOSI = 13;
// static const int HSPI_CS = 15;
// static const int HSPI_CLK = 14;

int start = 0;

#ifndef LIGHT_STICK_NO_INPUT
static const int B_1 = D3;
static const int B_2 = D8;
static const int JOYSTICK = A0;
#endif

const uint16_t PixelCount = 144; // the sample images are meant for 144 pixels
const uint16_t PixelPin = 15;
const uint16_t AnimCount = 1; // we only need one

NeoPixelBus<NeoGrbFeature, NeoWs2812xMethod> strip(PixelCount, PixelPin);
NeoPixelAnimator animations(AnimCount); // NeoPixel animation management object

NeoBitmapFile<NeoGrbFeature, File> image;

std::vector<NeoBuffer<NeoBufferMethod<NeoGrbFeature>> *> buffers;
uint8_t activeBuffer = 0;

uint16_t animState;

uint8_t brightnessIndex = 2;

uint8_t lookupBrightness(uint8_t inBright)
{
    return inBright * inBright;
}
const uint8_t MAX_BRIGHT_INDEX = 10;
const uint8_t MAX_BRIGHT = lookupBrightness(MAX_BRIGHT_INDEX);

void printFiles();

uint16_t _width = 144;
uint16_t PER_BUFFER_HEIGHT = 100;

uint16_t MyLayoutMap(int16_t x, int16_t y)
{
    return y * _width + x;
}

void LoopAnimUpdate(const AnimationParam &param)
{
    // wait for this animation to complete,
    // we are using it as a timer of sorts
    if (param.state == AnimationState_Completed)
    {
        // draw the complete row at animState to the complete strip
        // image.Blt(strip, 0, 0, animState, min((uint16_t)144, image.Width()));
        buffers[activeBuffer]->Blt(
            strip,
            0,                             // xDest
            0,                             // yDest
            0,                             // xSrc
            animState % PER_BUFFER_HEIGHT, // ySrc
            _width,                        // wSrc
            1,                             // hSrc
            MyLayoutMap);

        if (animState % PER_BUFFER_HEIGHT == 0)
        {
            activeBuffer++;
        }

        animState = animState + 1; // increment and wrap

        // uint8_t brightness = lookupBrightness(brightnessIndex);
        // for (uint8_t i = 0; i < PixelCount; i++)
        // {
        //     RgbColor colour = strip.GetPixelColor(i);
        //     strip.SetPixelColor(
        //         i,
        //         RgbColor(
        //             (colour.R * brightness) / MAX_BRIGHT,
        //             (colour.G * brightness) / MAX_BRIGHT,
        //             (colour.B * brightness) / MAX_BRIGHT));
        // }

        // done, time to restart this position tracking animation/timer
        // if (animState < image.Height())
        if (animState < image.Height())
        {
            animations.RestartAnimation(param.index);
        }
        else
        {
            strip.ClearTo(RgbColor(0, 0, 0));
            strip.Show();

            int end = millis();
            int diff = end - start;
            // Serial.printf("End %d, diff %d. per frame %d\n", end, diff, diff / image.Height());
            Serial.printf("End %d, diff %d. per frame %d\n", end, diff, diff / image.Height());

            // Show file list once animation finishes
            printFiles();
            return;
        }
        strip.Show();
    }
}

#ifndef LIGHT_STICK_NO_INPUT
void button_isr(int n);

struct Button
{
    const uint8_t pin;
    bool pressed;
    unsigned long last_time;
    void (*isr)(void);
};

void b1_isr()
{
    button_isr(0);
}
void b2_isr()
{
    button_isr(1);
}

Button buttons[] = {
    {B_1, false, 0, b1_isr},
    {B_2, false, 0, b2_isr},
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
#endif

enum Mode
{
    MENU = 1,
    PLAYING,
};

#ifndef LIGHT_STICK_NO_INPUT
enum Direction
{
    DIR_DOWN,  // 0
    DIR_LEFT,  // 650 / 819
    DIR_UP,    // 1500 / 1637
    DIR_IN,    // ? / 2457
    DIR_RIGHT, // 3100 / 3276
    DIR_NONE,  // 4095
};
#endif

#ifndef LIGHT_STICK_NO_DISPLAY
Adafruit_SSD1306 display = Adafruit_SSD1306(128, 64, &WIRE);
#endif
static const int FILES_ON_SCREEN = 7;
File root;
int rootFileCount = 0;
int selectedFile = 0;
bool didDirection = false;

int getDirectoryFileCount(File dir)
{
    dir.rewindDirectory();
    int n = 0;
    while (true)
    {
        File entry = dir.openNextFile();
        if (!entry)
            break;
        n++;
    }
    return n;
}
void printDirectory(File dir, int count, int skip, int selected)
{
    dir.rewindDirectory();

#ifndef LIGHT_STICK_NO_DISPLAY
    display.clearDisplay();
    display.setCursor(0, 0);
    for (int i = 0; i < count + skip; i++)
    {
        File entry = dir.openNextFile();
        if (!entry)
            break;

        if (i >= skip)
        {
            if (i == selected)
            {
                display.print("> ");
            }
            else
            {
                display.print("  ");
            }
            display.println(entry.name());
        }

        entry.close();
    }
#else
    while (true)
    {
        File entry = dir.openNextFile();
        if (!entry)
            break;

        Serial.println(entry.name());

        entry.close();
    }
#endif
}
std::string getDirectoryNthFileName(File dir, int selected)
{
    dir.rewindDirectory();
    int n = 0;
    while (true)
    {
        File entry = dir.openNextFile();
        if (!entry)
            break;
        n++;
        if (n == selected)
        {
            std::string s = entry.name();
            entry.close();
            return s;
        }
    }
    return nullptr;
}

bool load_file(std::string name)
{
    Serial.printf("Loading %s\n", name.c_str());

    File bitmapFile = SD.open(name.c_str());
    if (!bitmapFile)
    {
        Serial.println("File open fail, or not present");
#ifndef LIGHT_STICK_NO_DISPLAY
        display.println("No such file");
        display.display();
#endif
        return false;
    }
    Serial.println("Image found");

    if (!image.Begin(bitmapFile))
    {
        Serial.println("File format fail, not a supported bitmap");
#ifndef LIGHT_STICK_NO_DISPLAY
        display.println("Bad format");
        display.display();
#endif
        return false;
    }
    Serial.println("Image loaded");

    _width = min((uint16_t)144, image.Width());

    Serial.printf("Width x Height (%d x %d)\n", _width, image.Height());
#ifndef LIGHT_STICK_NO_DISPLAY
    display.println("img loaded");
#endif

    Serial.println("Starting copy");
    for (auto buffer : buffers)
    {
        delete buffer;
    }
    buffers.clear();

    Serial.println(ESP.getFreeHeap());
    uint8_t bufferCount = (image.Height() + PER_BUFFER_HEIGHT - 1) / PER_BUFFER_HEIGHT; // ceil division
    buffers.resize(bufferCount);
    for (uint8_t i = 0; i < bufferCount; i++)
    {
        buffers[i] = new NeoBuffer<NeoBufferMethod<NeoGrbFeature>>(_width, PER_BUFFER_HEIGHT, NULL);
        image.Blt(*buffers[i], 0, 0, 0, PER_BUFFER_HEIGHT * i, image.Width(), PER_BUFFER_HEIGHT, MyLayoutMap);
        Serial.printf("Buffer %d created\n", i);
        Serial.println(ESP.getFreeHeap());
    }
    Serial.println("Buffers created");

    activeBuffer = 0;

    return true;
}

void printBrightness()
{
#ifndef LIGHT_STICK_NO_DISPLAY
    display.clearDisplay();
    display.setCursor(0, 0);
    display.printf("Brightness: %d (%d%%)", brightnessIndex, lookupBrightness(brightnessIndex));
    display.display();
#else
    Serial.printf("Brightness: %d (%d%%)", brightnessIndex, lookupBrightness(brightnessIndex));
#endif
}
void printFiles()
{
    int toSkip = 0;
    if (selectedFile > 3)
    {
        toSkip = selectedFile - 3;
    }
    if (selectedFile > rootFileCount - 4)
    {
        toSkip = rootFileCount - 4;
    }
    printDirectory(root, FILES_ON_SCREEN, toSkip, selectedFile);
#ifndef LIGHT_STICK_NO_DISPLAY
    display.display();
#endif
}

void setup()
{
    /*
     * INIT LEDS
     */
    strip.Begin();
    animState = 0;
    strip.ClearTo(RgbColor(0, 0, 0));
    strip.Show();

    Serial.begin(115200);
    while (!Serial)
    {
        ; // wait for serial port to connect. Needed for native USB port only
    }

    Serial.printf("Total heap: %d\n", ESP.getHeapSize());
    Serial.printf("Free heap: %d\n", ESP.getFreeHeap());
    Serial.printf("Total PSRAM: %d\n", ESP.getPsramSize());
    Serial.printf("Free PSRAM: %d\n", ESP.getFreePsram());

#ifndef LIGHT_STICK_NO_INPUT
    /*
     * INIT BUTTONS
     */
    pinMode(B_1, INPUT);
    pinMode(B_2, INPUT);
    pinMode(JOYSTICK, INPUT_PULLUP);
    for (int i = 0; i < buttonCount; i++)
    {
        pinMode(buttons[i].pin, INPUT_PULLUP);
        attachInterrupt(
            buttons[i].pin, buttons[i].isr,
            RISING);
    }
#endif

#ifndef LIGHT_STICK_NO_DISPLAY
    /*
     * INIT DISPLAY
     */
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // Address 0x3C for 128x32

    display.clearDisplay();
    display.display();

    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
#endif

    /*
     * INIT SD CARD
     */
    Serial.println("Initializing SD card...");
    /*
     *
     */
    if (!SD.begin(VSPI_CS))
    {
        Serial.println("initialization failed!");
#ifndef LIGHT_STICK_NO_DISPLAY
        display.println("SD Card Init failed");
        display.display();
#endif
        while (1)
            ;
    }
    else
    {
        Serial.println("initialization done.");
#ifndef LIGHT_STICK_NO_DISPLAY
        display.println("SD Card Init success");
#endif
    }

    root = SD.open("/");
    rootFileCount = getDirectoryFileCount(root);
    printDirectory(root, FILES_ON_SCREEN, 0, 0);
#ifndef LIGHT_STICK_NO_DISPLAY
    display.display();
#endif
}

#ifndef LIGHT_STICK_NO_INPUT
Direction getJoystickDir()
{
    uint16_t read = analogRead(JOYSTICK);

    if (read < 409)
    {
        return DIR_DOWN;
    }
    else if (read < 1228)
    {
        return DIR_LEFT;
    }
    else if (read < 2047)
    {
        return DIR_UP;
    }
    else if (read < 2867)
    {
        return DIR_IN;
    }
    else if (read < 3686)
    {
        return DIR_RIGHT;
    }
    else
    {
        return DIR_NONE;
    }
}
#endif

void loop()
{
#ifndef LIGHT_STICK_NO_INPUT
    auto joystickDir = getJoystickDir();
    if (joystickDir == DIR_NONE)
    {
        if (didDirection)
        {
            didDirection = false;
        }
    }
    else if (!didDirection)
    {
        didDirection = true;
        bool dirChanged = false;
        if (joystickDir == DIR_UP)
        {
            if (selectedFile == 0)
                selectedFile = rootFileCount - 1;
            else
                selectedFile -= 1;
            printFiles();
        }
        else if (joystickDir == DIR_DOWN)
        {
            selectedFile = (selectedFile + 1) % rootFileCount;
            printFiles();
        }
        else if (joystickDir == DIR_LEFT)
        {
            if (brightnessIndex > 0)
            {
                brightnessIndex -= 1;
            }
            printBrightness();
        }
        else if (joystickDir == DIR_RIGHT)
        {
            if (brightnessIndex < MAX_BRIGHT_INDEX)
            {
                brightnessIndex += 1;
            }
            printBrightness();
        }
        else if (joystickDir == DIR_IN)
        {
            printFiles();
        }
    }

    // back/cancel
    if (buttons[0].pressed)
    {
        buttons[0].pressed = false;

        if (animations.IsAnimating())
        {
            animations.StopAll();
            strip.ClearTo(RgbColor(0, 0, 0));
            strip.Show();
            printFiles();
        }
    }

    // start/pause
    if (buttons[1].pressed)
    {
        buttons[1].pressed = false;
        if (animations.IsAnimating())
        {
            if (animations.IsPaused())
            {
                animations.Resume();
            }
            else
            {
                animations.Pause();
            }
        }
        else
        {
            std::string name = "/" + getDirectoryNthFileName(root, selectedFile + 1);
            bool loaded = load_file(name);

            if (loaded)
            {
                animState = 0;
                animations.StartAnimation(0, 2500 / image.Height(), LoopAnimUpdate);
                display.clearDisplay();
                display.display();
            }
        }
    }
#endif

    if (animations.IsAnimating())
    {
        animations.UpdateAnimations();
    }
#ifdef LIGHT_STICK_NO_INPUT
    else
    {
        std::string name = "/" + getDirectoryNthFileName(root, 15);
        bool loaded = load_file(name);

        if (loaded)
        {
            start = millis();
            Serial.printf("Start %d\n", start);
            animState = 0;
            animations.StartAnimation(0, 5, LoopAnimUpdate);
        }
    }
#endif
}
