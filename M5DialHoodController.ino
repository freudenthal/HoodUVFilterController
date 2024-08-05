#define VSPI 3

#include <PCF8574.h>
#include <TaskManagerIO.h>
#include <CommandParser.h>
#include <PrintCharArray.h>
//#include <M5Stack.h>
#include <M5Dial.h>
//#include <M5ez.h>
//#include <ezTime.h>

long oldPosition = -999;

int prev_x = -1;
int prev_y = -1;

static const uint8_t DigitalIOCount = 2;
static const uint8_t DigitalIOAddress0 = 0x22;
static const uint8_t DigitalIOAddress1 = 0x20;
static const uint8_t LineBufferCount = 255;

static const uint8_t LightControlPin = 4;
static const uint8_t FilterUVControlPin = 7;
static const uint8_t HoodUVControlPin = 3;
static const uint8_t FilterFanControlPin = 5;
static const uint8_t ExhaustFanControlPin = 6;

static m5::touch_state_t prev_state;

bool LightIsActive = false;
bool FilterUVIsActive = false;
bool HoodUVIsActive = false;
bool FilterFanIsActive = false;
bool ExhaustFanIsActive = false;

typedef CommandParser<> TextCommandParser;
TextCommandParser Parser;
PCF8574 DigitalIOExpander0 = PCF8574(DigitalIOAddress0,&Wire);
PCF8574 DigitalIOExpander1 = PCF8574(DigitalIOAddress1,&Wire);

PCF8574* DigitalIOExpanders[DigitalIOCount]
{
  &DigitalIOExpander0,
  &DigitalIOExpander1,
};

typedef void (*ButtonCallback)();
uint8_t ButtonActiveHigh = 0;
uint8_t ButtonState = 0;
ButtonCallback ButtonPressedCallbacks[8] = {ToggleLights,ToggleFilterUV,ToggleFilterFan,ToggleExhaustFan,ToggleHoodUV,ToggleTest,PrintTest6,PrintTest7};
ButtonCallback ButtonReleasedCallbacks[8] = {NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};

char LineBuffer[LineBufferCount];
uint8_t LineBufferReadLength = 0;
char LastChar = '\0';
PrintCharArray CommandResponse(TextCommandParser::MAX_RESPONSE_SIZE);
PrintCharArray DisplayString(TextCommandParser::MAX_RESPONSE_SIZE);

void draw_function(LovyanGFX* gfx)
{
    int x      = rand() % gfx->width();
    int y      = rand() % gfx->height();
    int r      = (gfx->width() >> 4) + 2;
    uint16_t c = rand();
    gfx->fillRect(x - r, y - r, r * 2, r * 2, c);
}

void ScanI2C()
{
    uint8_t error = 0;
    uint8_t nDevices = 0;
    for(uint8_t address = 1; address < 127; address++ )
    {
        // The i2c_scanner uses the return value of
        // the Write.endTransmisstion to see if
        // a device did acknowledge to the address.
        Wire.beginTransmission(address);
        error = Wire.endTransmission();
        if (error == 0)
        {
            Serial.print("I2C device found at address 0x");
            if (address<16)
            {
            Serial.print("0");
            }
            //Serial.print("0x");
            Serial.print(address,HEX);
            Serial.println(".");
            nDevices++;
        }
        else if (error==4)
        {
            Serial.print("Unknown error at address 0x");
            if (address<16)
            {
                Serial.print("0");
            }
            Serial.println(address,HEX);
        }
        else
        {
          Serial.print("Nothing at ");
          Serial.print(address,HEX);
          Serial.print(" with ");
          Serial.print(error);
          Serial.println(".");
        }
    }
    if (nDevices == 0)
    {
    Serial.println("No I2C devices found.");
    }
    else
    {
    Serial.print("I2C scan done found ");
    Serial.print(nDevices);
    Serial.println(" devices found.");
    }
}

void CommandIdentify(TextCommandParser::Argument *args, char *response)
{
    CommandResponse.clear();
    CommandResponse.println("HoodControllerV01");
}

void CommandI2CBusScan(TextCommandParser::Argument *args, char *response)
{
    taskManager.execute([] { ScanI2C(); });
    CommandResponse.clear();
    CommandResponse.println("I2C bus scan starting.");
}

void CommandSetPin(TextCommandParser::Argument *args, char *response)
{
    uint8_t ParsedControllerIndex = (uint8_t)(constrain((uint8_t)args[0].asUInt64,(uint8_t)0,(uint8_t)1));
    uint8_t ParsedPinIndex = (uint8_t)(constrain((uint8_t)args[1].asUInt64,(uint8_t)0,7));
    bool ParsedPinSetting = (bool)(constrain((uint8_t)args[2].asUInt64,(uint8_t)0,(uint8_t)1));
    DigitalIOExpanders[ParsedControllerIndex]->write(ParsedPinIndex,ParsedPinSetting);
    CommandResponse.clear();
    CommandResponse.print("Set controller ");
    CommandResponse.print(ParsedControllerIndex);
    CommandResponse.print(" pin ");
    CommandResponse.print(ParsedPinIndex);
    CommandResponse.print(" to ");
    if (ParsedPinSetting)
    {
        CommandResponse.println("on.");
    }
    else
    {
        CommandResponse.println("off.");
    }
}

void CommandGetPin(TextCommandParser::Argument *args, char *response)
{
    uint8_t ParsedControllerIndex = (uint8_t)(constrain((uint8_t)args[0].asUInt64,(uint8_t)0,(uint8_t)1));
    uint8_t ParsedPinIndex = (uint8_t)((uint64_t)constrain((uint8_t)args[1].asUInt64,(uint8_t)0,(uint8_t)7));
    bool PinSetting = DigitalIOExpanders[ParsedControllerIndex]->read(ParsedPinIndex);
    CommandResponse.clear();
    CommandResponse.print("Controller ");
    CommandResponse.print(ParsedControllerIndex);
    CommandResponse.print(" pin ");
    CommandResponse.print(ParsedPinIndex);
    CommandResponse.print(" is ");
    if (PinSetting)
    {
        CommandResponse.println("on.");
    }
    else
    {
        CommandResponse.println("off.");
    }
}

void CommandGetSubsystemActive(TextCommandParser::Argument *args, char *response)
{
    char* ParsedString = args[0].asString;
    uint8_t ParsedOnOff = (bool)((uint64_t)constrain((uint8_t)args[1].asUInt64,(uint8_t)0,(uint8_t)1));
    char ParsedStart = ParsedString[0];
    bool PinSetting = DigitalIOExpanders[ParsedControllerIndex]->read(ParsedPinIndex);
    CommandResponse.clear();
    CommandResponse.print("Controller ");
    CommandResponse.print(ParsedControllerIndex);
    CommandResponse.print(" pin ");
    CommandResponse.print(ParsedPinIndex);
    CommandResponse.print(" is ");
    if (PinSetting)
    {
        CommandResponse.println("on.");
    }
    else
    {
        CommandResponse.println("off.");
    }
}

void BuildParser()
{
    Parser.registerCommand("ID", "", &CommandIdentify);
    Parser.registerCommand("I2CScan", "", &CommandI2CBusScan);
    Parser.registerCommand("SetPin", "uuu", &CommandSetPin);
    Parser.registerCommand("GetPin", "uu", &CommandGetPin);
    Parser.registerCommand("SetSubsystemActive", "su", &CommandGetSubsystemActive);
    //Parser.registerCommand("DisplaySet", "s", &CommandDisplaySet);
}

void ClearLineBuffer()
{
    memset(LineBuffer, 0, sizeof(LineBuffer));
    LineBufferReadLength = 0;
    LastChar = '\0';
}

void ProcessLineBuffer()
{
    LineBuffer[LineBufferReadLength] = '\0';
    Parser.processCommand(LineBuffer, CommandResponse.getBuffer());
    Serial.println(CommandResponse.getBuffer());
}

void ParseSerial()
{
    while (Serial.available() > 0)
    {
        char NewChar = Serial.read();
        bool NewCharIsLineEnd = (NewChar == '\n') || (NewChar == ';') || (NewChar == '\r');
        bool NewCharIsBlankOrEnd = ( isspace(NewChar) || NewCharIsLineEnd );
        bool NewCharIsIgnored = (NewChar == '\t') || (NewChar == '\v') || (NewChar == '\f');
        bool NewCharIsDoubleSpace = isspace(NewChar) && isspace(LastChar);
        bool IsLeadingWhiteSpace = (LineBufferReadLength == 0) && NewCharIsBlankOrEnd;
        if ( IsLeadingWhiteSpace || NewCharIsIgnored || NewCharIsDoubleSpace )
        {
            //Do nothing
        }
        else if ( NewCharIsLineEnd )
        {
            if (LineBufferReadLength > 1)
            {
                ProcessLineBuffer();
            }
            ClearLineBuffer();
            break;
        }
        else
        {
            LastChar = NewChar;
            LineBuffer[LineBufferReadLength] = NewChar;
            LineBufferReadLength++;
            if (LineBufferReadLength >= LineBufferCount)
            {
                ClearLineBuffer();
                Serial.println("Input buffer overflow. Input cleared.");
                break;
            }
        }
    }
}

void SetupDigitalIOExpander()
{
  for (uint8_t Index = 0; Index < DigitalIOCount; Index++)
  {
    DigitalIOExpanders[Index]->begin(255);
    for(uint8_t BitIndex = 0; BitIndex < 8; BitIndex++)
    {
        DigitalIOExpanders[Index]->write(BitIndex,true);
    }
  }
}

void UpdateDisplayWithOnOffStatus()
{
    M5Dial.Display.clear();
    DisplayString.clear();
    DisplayString.print("L:");
    DisplayString.print(LightIsActive);
    M5Dial.Display.drawString(String(DisplayString.getBuffer()), M5Dial.Display.width() / 2, M5Dial.Display.height() / 4);
    DisplayString.clear();
    DisplayString.print("UV:H:");
    DisplayString.print(HoodUVIsActive);
    DisplayString.print(":F:");
    DisplayString.print(FilterUVIsActive);
    M5Dial.Display.drawString(String(DisplayString.getBuffer()), M5Dial.Display.width() / 2, M5Dial.Display.height() / 2);
    DisplayString.clear();
    DisplayString.print("F:F:");
    DisplayString.print(FilterFanIsActive);
    DisplayString.print(":E:");
    DisplayString.print(ExhaustFanIsActive);
    M5Dial.Display.drawString(String(DisplayString.getBuffer()), M5Dial.Display.width() / 2, M5Dial.Display.height() * 3 / 4);
}

void OnOffBeep(bool OnOff)
{
    UpdateDisplayWithOnOffStatus();
    if (OnOff)
    {
        M5Dial.Speaker.tone(11000, 200);
    }
    else
    {
        M5Dial.Speaker.tone(9000, 200);
    }
}

void ToggleLights()
{
    LightIsActive = !LightIsActive;
    DigitalIOExpander0.write(LightControlPin,!LightIsActive);
    OnOffBeep(LightIsActive);
    Serial.print("Light ");
    Serial.println(LightIsActive);
}

void ToggleFilterUV()
{
    FilterUVIsActive = !FilterUVIsActive;
    DigitalIOExpander0.write(FilterUVControlPin,!FilterUVIsActive);
    OnOffBeep(FilterUVIsActive);
    Serial.print("Filter UV ");
    Serial.println(FilterUVIsActive);
}

void ToggleHoodUV()
{
    HoodUVIsActive = !HoodUVIsActive;
    DigitalIOExpander0.write(HoodUVControlPin,!HoodUVIsActive);
    OnOffBeep(HoodUVIsActive);
    Serial.print("Hood UV ");
    Serial.println(HoodUVIsActive);
}

void ToggleFilterFan()
{
    FilterFanIsActive = !FilterFanIsActive;
    DigitalIOExpander0.write(FilterFanControlPin,!FilterFanIsActive);
    OnOffBeep(FilterFanIsActive);
    Serial.print("Filter fan ");
    Serial.println(FilterFanIsActive);
}

void ToggleExhaustFan()
{
    ExhaustFanIsActive = !ExhaustFanIsActive;
    DigitalIOExpander0.write(ExhaustFanControlPin,!ExhaustFanIsActive);
    OnOffBeep(ExhaustFanIsActive);
    Serial.print("Exhaust fan ");
    Serial.println(ExhaustFanIsActive);
}

bool TestIsActive = false;
void ToggleTest()
{
    TestIsActive = !TestIsActive;
    Serial.print("Test toggle ");
    Serial.print(TestIsActive);
    Serial.println();
}

void PrintTest6()
{
  Serial.println("Button 6");
}

void PrintTest7()
{
  Serial.println("Button 7");
}

void CheckEncoder()
{
    long newPosition = M5Dial.Encoder.read();
    if (newPosition != oldPosition)
    {
        M5Dial.Speaker.tone(8000, 20);
        M5Dial.Display.clear();
        oldPosition = newPosition;
        Serial.println(newPosition);
        M5Dial.Display.drawString(String(newPosition), M5Dial.Display.width() / 2, M5Dial.Display.height() / 2);
    }
}

void CheckButtons()
{
    uint8_t ButtonStatus = DigitalIOExpander1.read8();
    bool AnyPressed = ButtonStatus != 255;
    bool NeedToClear = ButtonState != 0;
    bool CurrentIsActive = false;
    bool CurrentButtonState = false;
    bool StateChangeNeeded = false;
    if (AnyPressed || NeedToClear)
    {
        ButtonStatus = DigitalIOExpander1.read8();
        //Serial.print("Pressed ");
        for (uint8_t Index = 0; Index < 8; Index++)
        {
            CurrentIsActive = bitRead(ButtonStatus,Index) == bitRead(ButtonActiveHigh,Index);
            CurrentButtonState = bitRead(ButtonState,Index);
            StateChangeNeeded = CurrentIsActive != CurrentButtonState;
            if (StateChangeNeeded)
            {
                bitWrite(ButtonState, Index, CurrentIsActive);
                if (CurrentIsActive)
                {
                    if (ButtonPressedCallbacks[Index] != NULL)
                    {
                        ButtonPressedCallbacks[Index]();
                    }
                }
                else
                {
                    if (ButtonReleasedCallbacks[Index] != NULL)
                    {
                        ButtonReleasedCallbacks[Index]();
                    }
                }
            }
            //Serial.print(Index);
            //Serial.print(",");
            //Serial.print(CurrentIsActive);
            //Serial.print(",");
            //Serial.print(CurrentButtonState);
            //Serial.print(",");
            //Serial.print(StateChangeNeeded);
            //Serial.print(",");
            //Serial.print(ButtonPressedCallbacks[Index] != NULL);
            //Serial.print(",");
            //Serial.print(ButtonReleasedCallbacks[Index] != NULL);
            //Serial.print(",");
        }
        //Serial.println();
    }
}

void SetupTasks()
{
    taskManager.schedule(repeatMillis(50), ParseSerial);
    taskManager.schedule(repeatMillis(50), CheckEncoder);
    taskManager.schedule(repeatMillis(50), CheckButtons);
}

void setup()
{
    Serial.begin(115200);
    Serial.println("Start begin");
    auto cfg = M5.config();
    Serial.println("M5 Dial begin");
    M5Dial.begin(cfg, true, true);
    Serial.println("Display begin");
    int textsize = M5Dial.Display.height() / 60;
    if (textsize == 0) {
        textsize = 1;
    }
    textsize = 1;
    M5Dial.Display.setTextColor(GREEN);
    M5Dial.Display.setTextDatum(middle_center);
    M5Dial.Display.setTextFont(&fonts::Orbitron_Light_32);
    M5Dial.Display.setTextSize(textsize);
    Serial.println("I2C begin");
    Wire.begin(13,15);
    Serial.println("DIO begin");
    SetupDigitalIOExpander();
    Serial.println("endstart");
    BuildParser();
    SetupTasks();
}

void loop()
{
    //int x      = rand() % M5Dial.Display.width();
    //int y      = rand() % M5Dial.Display.height();
    //int r      = (M5Dial.Display.width() >> 4) + 2;
    //uint16_t c = rand();
    //M5Dial.Display.fillCircle(x, y, r, c);
    //draw_function(&M5Dial.Display);
    //Serial.println("test");
    M5Dial.update();

    taskManager.runLoop();
    
    //CheckKeys();

    
    //if (M5Dial.BtnA.wasPressed())
    //{
    //    M5Dial.Encoder.readAndReset();
    //}
    //if (M5Dial.BtnA.pressedFor(5000))
    //{
    //    M5Dial.Encoder.write(100);
    //}

    //M5Dial.Speaker.tone(10000, 100);
    //delay(1000);
    //M5Dial.Speaker.tone(4000, 20);
    //delay(1000);

    //auto t = M5Dial.Touch.getDetail();
    //if (prev_state != t.state)
    //{
    //    prev_state = t.state;
    //    static constexpr const char* state_name[16] = {
    //        "none", "touch", "touch_end", "touch_begin",
    //        "___",  "hold",  "hold_end",  "hold_begin",
    //        "___",  "flick", "flick_end", "flick_begin",
    //        "___",  "drag",  "drag_end",  "drag_begin"};
    //    M5_LOGI("%s", state_name[t.state]);
    //    M5Dial.Display.fillRect(0, 0, M5Dial.Display.width(),
    //                            M5Dial.Display.height() / 2, BLACK);

    //    M5Dial.Display.drawString(state_name[t.state],
    //                              M5Dial.Display.width() / 2,
    //                              M5Dial.Display.height() / 2 - 30);
    //}
    //if (prev_x != t.x || prev_y != t.y)
    //{
    //    M5Dial.Display.fillRect(0, M5Dial.Display.height() / 2,
    //                            M5Dial.Display.width(),
    //                            M5Dial.Display.height() / 2, BLACK);
    //    M5Dial.Display.drawString(
    //        "X:" + String(t.x) + " / " + "Y:" + String(t.y),
    //        M5Dial.Display.width() / 2, M5Dial.Display.height() / 2 + 30);
    //    prev_x = t.x;
    //    prev_y = t.y;
    //    M5Dial.Display.drawPixel(prev_x, prev_y);
    //}

    //if (M5Dial.BtnA.wasPressed())
    //{
    //    M5Dial.Speaker.tone(8000, 20);
    //    M5Dial.Display.clear();
    //    M5Dial.Display.drawString("Pressed", M5Dial.Display.width() / 2,
    //                              M5Dial.Display.height() / 2);
    //}
    //if (M5Dial.BtnA.wasReleased())
    //{
    //    M5Dial.Speaker.tone(8000, 20);
    //    M5Dial.Display.clear();
    //    M5Dial.Display.drawString("Released", M5Dial.Display.width() / 2,
    //                              M5Dial.Display.height() / 2);
    //}
}