#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <NeoPixelBus.h>
#include <NeoPixelAnimator.h>
#include <Adafruit_SSD1306.h>

// #define BUTTON_A  0
// #define BUTTON_B 16
// #define BUTTON_C  2
#define WIRE Wire

// SPIClass *SPI1;
// static const int VSPI_MISO = 19;
// static const int VSPI_MOSI = 23;
static const int VSPI_CS = D2;
// static const int VSPI_CLK = 18;

// static const int HSPI_MISO = 12;
// static const int HSPI_MOSI = 13;
// static const int HSPI_CS = 15;
// static const int HSPI_CLK = 14;

static const int B_1 = D3;
static const int B_2 = D8;
static const int JOYSTICK = A0;

const uint16_t PixelCount = 144; // the sample images are meant for 144 pixels
const uint16_t PixelPin = D4;
const uint16_t AnimCount = 1; // we only need one

NeoPixelBus<NeoGrbFeature, NeoWs2812xMethod> strip(PixelCount, PixelPin);
NeoPixelAnimator animations(AnimCount); // NeoPixel animation management object

NeoBitmapFile<NeoGrbFeature, File> image;

uint16_t animState;

uint8_t brightnessIndex = 10;

uint8_t lookupBrightness(uint8_t inBright)
{
    return inBright * inBright;
}
const uint8_t MAX_BRIGHT_INDEX = 10;
const uint8_t MAX_BRIGHT = lookupBrightness(MAX_BRIGHT_INDEX);

void printFiles();

void LoopAnimUpdate(const AnimationParam &param)
{
    // wait for this animation to complete,
    // we are using it as a timer of sorts
    if (param.state == AnimationState_Completed)
    {
        // draw the complete row at animState to the complete strip
        image.Blt(strip, 0, 0, animState, image.Width());
        animState = animState + 1; // increment and wrap

        uint8_t brightness = lookupBrightness(brightnessIndex);
        for (uint8_t i = 0; i < PixelCount; i++)
        {
            RgbColor colour = strip.GetPixelColor(i);
            strip.SetPixelColor(
                i,
                RgbColor(
                    (colour.R * brightness) / MAX_BRIGHT,
                    (colour.G * brightness) / MAX_BRIGHT,
                    (colour.B * brightness) / MAX_BRIGHT));
        }

        // done, time to restart this position tracking animation/timer
        if (animState < image.Height())
        {
            animations.RestartAnimation(param.index);
        }
        else
        {
            strip.ClearTo(RgbColor(0, 0, 0));
            strip.Show();
            // Show file list once animation finishes
            printFiles();
            return;
        }
        strip.Show();
    }
}

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

enum Mode
{
    MENU = 1,
    PLAYING,
};

enum Direction
{
    DIR_DOWN,  // 0
    DIR_LEFT,  // 650 / 819
    DIR_UP,    // 1500 / 1637
    DIR_IN,    // ? / 2457
    DIR_RIGHT, // 3100 / 3276
    DIR_NONE,  // 4095
};

Adafruit_SSD1306 display = Adafruit_SSD1306(128, 64, &WIRE);
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
    display.clearDisplay();
    display.setCursor(0, 0);
    dir.rewindDirectory();
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
    File bitmapFile = SD.open(name.c_str());
    if (!bitmapFile)
    {
        Serial.println("File open fail, or not present");
        display.println("No such file");
        display.display();
        return false;
    }
    Serial.println("Image found");

    if (!image.Begin(bitmapFile))
    {
        Serial.println("File format fail, not a supported bitmap");
        display.println("Bad format");
        display.display();
        return false;
    }
    Serial.println("Image loaded");
    display.println("img loaded");

    return true;
}

void printBrightness()
{
    display.clearDisplay();
    display.setCursor(0, 0);
    display.printf("Brightness: %d (%d%%)", brightnessIndex, lookupBrightness(brightnessIndex));
    display.display();
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
    display.display();
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

    /*
     * INIT DISPLAY
     */
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // Address 0x3C for 128x32

    display.clearDisplay();
    display.display();

    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);

    /*
     * INIT SD CARD
     */
    Serial.println("Initializing SD card...");
    if (!SD.begin(VSPI_CS))
    {
        Serial.println("initialization failed!");
        display.println("SD Card Init failed");
        display.display();
        while (1)
            ;
    }
    else
    {
        Serial.println("initialization done.");
        display.println("SD Card Init success");
    }

    root = SD.open("/");
    rootFileCount = getDirectoryFileCount(root);
    printDirectory(root, FILES_ON_SCREEN, 0, 0);
    display.display();
}

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

void loop()
{
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

    if (animations.IsAnimating())
    {
        animations.UpdateAnimations();
    }
}
