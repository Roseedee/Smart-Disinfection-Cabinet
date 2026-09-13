#include <Arduino.h>
#include <TM1637Display.h>

// =====================================================
// PIN
// =====================================================

// TM1637
#define CLK_PIN       14
#define DIO_PIN       13

// BUTTON
#define START_STOP    27
#define UP_BUTTON     26
#define DOWN_BUTTON   25
#define SET_BUTTON    33


TM1637Display display(CLK_PIN, DIO_PIN);


// =====================================================
// TIMER
// =====================================================

// เวลาที่เหลือทั้งหมดเป็นวินาที
uint32_t remainingSeconds = 0;

bool running = false;


// =====================================================
// SET MODE
// =====================================================

// 0 = MINUTE
// 1 = SECOND
uint8_t setMode = 0;


// =====================================================
// BLINK
// =====================================================

unsigned long lastBlink = 0;
bool blinkState = true;


// =====================================================
// TIMER CLOCK
// =====================================================

unsigned long lastSecond = 0;


// =====================================================
// BUTTON STATE
// =====================================================

bool lastStartState = HIGH;
bool lastUpState    = HIGH;
bool lastDownState  = HIGH;
bool lastSetState   = HIGH;


// =====================================================
// BUTTON DEBOUNCE
// =====================================================

unsigned long lastStartPress = 0;
unsigned long lastUpPress    = 0;
unsigned long lastDownPress  = 0;
unsigned long lastSetPress   = 0;

const unsigned long debounceTime = 150;


// =====================================================
// SETUP
// =====================================================

void setup()
{
    Serial.begin(115200);

    // -------------------------------------------------
    // TM1637
    // -------------------------------------------------

    display.setBrightness(7);
    display.clear();


    // -------------------------------------------------
    // Buttons
    // -------------------------------------------------

    pinMode(START_STOP, INPUT_PULLUP);
    pinMode(UP_BUTTON, INPUT_PULLUP);
    pinMode(DOWN_BUTTON, INPUT_PULLUP);
    pinMode(SET_BUTTON, INPUT_PULLUP);


    // -------------------------------------------------
    // Display
    // -------------------------------------------------

    updateDisplay();


    Serial.println();
    Serial.println("======================");
    Serial.println(" MM:SS TIMER READY");
    Serial.println("======================");
}


// =====================================================
// LOOP
// =====================================================

void loop()
{
    unsigned long now = millis();


    // -------------------------------------------------
    // Buttons
    // -------------------------------------------------

    handleButtons(now);


    // -------------------------------------------------
    // Timer
    // -------------------------------------------------

    if (running)
    {
        // ครบ 1 วินาที
        if (now - lastSecond >= 1000)
        {
            lastSecond = now;

            countdown();
        }


        // -------------------------------------------------
        // Colon blink
        // -------------------------------------------------

        if (now - lastBlink >= 500)
        {
            lastBlink = now;

            blinkState = !blinkState;

            updateDisplay();
        }
    }


    // -------------------------------------------------
    // SET MODE blink
    // -------------------------------------------------

    else
    {
        if (now - lastBlink >= 500)
        {
            lastBlink = now;

            blinkState = !blinkState;

            updateDisplay();
        }
    }
}


// =====================================================
// HANDLE BUTTONS
// =====================================================

void handleButtons(unsigned long now)
{

    // ===================================================
    // START / STOP
    // ===================================================

    bool startState = digitalRead(START_STOP);

    if (startState == LOW && lastStartState == HIGH)
    {
        if (now - lastStartPress >= debounceTime)
        {

            // ห้าม START ถ้า 00:00
            if (remainingSeconds > 0)
            {
                running = !running;

                if (running)
                {
                    // เริ่มจับเวลาใหม่จากเวลาปัจจุบัน
                    lastSecond = now;

                    // เริ่ม colon จากติด
                    lastBlink = now;
                    blinkState = true;

                    Serial.println("START");
                }
                else
                {
                    Serial.println("STOP");
                }

                updateDisplay();
            }

            lastStartPress = now;
        }
    }

    lastStartState = startState;


    // ===================================================
    // SET
    // ===================================================

    bool setState = digitalRead(SET_BUTTON);

    if (setState == LOW && lastSetState == HIGH)
    {
        if (!running && now - lastSetPress >= debounceTime)
        {

            // MINUTE -> SECOND -> MINUTE
            setMode++;

            if (setMode > 1)
            {
                setMode = 0;
            }


            // เริ่ม blink ใหม่
            blinkState = true;
            lastBlink = now;

            updateDisplay();

            lastSetPress = now;


            if (setMode == 0)
            {
                Serial.println("SET MODE: MINUTE");
            }
            else
            {
                Serial.println("SET MODE: SECOND");
            }
        }
    }

    lastSetState = setState;


    // ===================================================
    // UP
    // ===================================================

    bool upState = digitalRead(UP_BUTTON);

    if (upState == LOW && lastUpState == HIGH)
    {
        if (!running && now - lastUpPress >= debounceTime)
        {

            uint32_t minutes = remainingSeconds / 60;
            uint32_t seconds = remainingSeconds % 60;


            if (setMode == 0)
            {
                // -------------------------------
                // เพิ่มนาที
                // -------------------------------

                minutes++;

                if (minutes > 99)
                {
                    minutes = 0;
                }
            }
            else
            {
                // -------------------------------
                // เพิ่มวินาที
                // -------------------------------

                seconds++;

                if (seconds > 59)
                {
                    seconds = 0;
                }
            }


            // สร้างเวลาใหม่
            remainingSeconds = (minutes * 60) + seconds;


            // reset blink
            blinkState = true;
            lastBlink = now;

            updateDisplay();

            lastUpPress = now;
        }
    }

    lastUpState = upState;


    // ===================================================
    // DOWN
    // ===================================================

    bool downState = digitalRead(DOWN_BUTTON);

    if (downState == LOW && lastDownState == HIGH)
    {
        if (!running && now - lastDownPress >= debounceTime)
        {

            uint32_t minutes = remainingSeconds / 60;
            uint32_t seconds = remainingSeconds % 60;


            if (setMode == 0)
            {
                // -------------------------------
                // ลดนาที
                // -------------------------------

                if (minutes == 0)
                {
                    minutes = 99;
                }
                else
                {
                    minutes--;
                }
            }
            else
            {
                // -------------------------------
                // ลดวินาที
                // -------------------------------

                if (seconds == 0)
                {
                    seconds = 59;
                }
                else
                {
                    seconds--;
                }
            }


            // สร้างเวลาใหม่
            remainingSeconds = (minutes * 60) + seconds;


            // reset blink
            blinkState = true;
            lastBlink = now;

            updateDisplay();

            lastDownPress = now;
        }
    }

    lastDownState = downState;
}


// =====================================================
// COUNTDOWN
// =====================================================

void countdown()
{
    if (remainingSeconds > 0)
    {
        remainingSeconds--;

        updateDisplay();


        // Debug
        uint32_t minutes = remainingSeconds / 60;
        uint32_t seconds = remainingSeconds % 60;

        Serial.print("TIME = ");

        if (minutes < 10)
            Serial.print("0");

        Serial.print(minutes);

        Serial.print(":");

        if (seconds < 10)
            Serial.print("0");

        Serial.println(seconds);
    }


    // ---------------------------------------------------
    // Time Up
    // ---------------------------------------------------

    if (remainingSeconds == 0)
    {
        running = false;

        blinkState = true;

        updateDisplay();

        Serial.println("TIME UP");
    }
}


// =====================================================
// DISPLAY
// =====================================================

// =====================================================
// DISPLAY
// =====================================================

// =====================================================
// DISPLAY
// =====================================================

// =====================================================
// DISPLAY
// =====================================================

void updateDisplay()
{
    uint32_t minutes = remainingSeconds / 60;
    uint32_t seconds = remainingSeconds % 60;

    uint16_t value = (minutes * 100) + seconds;


    // ===================================================
    // RUNNING
    // ===================================================

    if (running)
    {
        // ตอนทำงาน : กระพริบเฉพาะ colon
        display.showNumberDecEx(
            value,
            blinkState ? 0b01000000 : 0,
            true
        );

        return;
    }


    // ===================================================
    // SET MODE
    // ===================================================

    uint8_t seg[4];


    // ---------------------------------------------------
    // MINUTE MODE
    // ---------------------------------------------------

    if (setMode == 0)
    {
        // MM กระพริบ
        if (blinkState)
        {
            seg[0] = display.encodeDigit(minutes / 10);
            seg[1] = display.encodeDigit(minutes % 10);
        }
        else
        {
            seg[0] = 0x00;
            seg[1] = 0x00;
        }

        // SS แสดงตลอด
        seg[2] = display.encodeDigit(seconds / 10);
        seg[3] = display.encodeDigit(seconds % 10);
    }


    // ---------------------------------------------------
    // SECOND MODE
    // ---------------------------------------------------

    else
    {
        // MM แสดงตลอด
        seg[0] = display.encodeDigit(minutes / 10);
        seg[1] = display.encodeDigit(minutes % 10);

        // SS กระพริบ
        if (blinkState)
        {
            seg[2] = display.encodeDigit(seconds / 10);
            seg[3] = display.encodeDigit(seconds % 10);
        }
        else
        {
            seg[2] = 0x00;
            seg[3] = 0x00;
        }
    }


    // ---------------------------------------------------
    // แสดงตัวเลข
    // ---------------------------------------------------

    display.setSegments(seg);


    // ---------------------------------------------------
    // เปิด COLON
    // ---------------------------------------------------

    // ใช้ showNumberDecEx เพื่อเปิด colon
    // เฉพาะกรณีที่ตัวเลขทั้งหมดแสดง
    if (blinkState)
    {
        display.showNumberDecEx(
            value,
            0b01000000,
            true
        );
    }
}