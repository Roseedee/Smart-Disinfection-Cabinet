#include <Arduino.h>
#include <TM1637Display.h>

#define CLK_PIN 14
#define DIO_PIN 13

TM1637Display display(CLK_PIN, DIO_PIN);

void setup()
{
    display.setBrightness(7);  // 0-7

    // ล้างจอ
    display.clear();

    delay(500);
}

void loop()
{
    display.showNumberDecEx(
        1234,
        0b01000000,   // เปิด :
        true
    );

    delay(2000);

    display.showNumberDecEx(
        0,
        0b01000000,
        true
    );

    delay(1000);

    for (int i = 0; i <= 9999; i++)
    {
        display.showNumberDec(i, true);
        delay(10);
    }
}