//
// Created by SQ5RWU on 18.10.22.
//
#include "Arduino.h"
#include <SPI.h>
#include <avr/wdt.h>
#include <avr/sleep.h>

typedef uint32_t adfRegisters[6];

// 10489MHz - 9750MHz = 739MHz
// 739MHz - 430MHz = 309MHz
static const adfRegisters freq_1 = {
        0x3100B0,
        0x80080C9,
        0x4E42,
        0x4B3,
        0xBC803C,
        0x580005,
};
// 10368MHz - 9750MHz = 618MHz
// 618MHz - 430MHz = 188MHz
static const adfRegisters freq_2 = {
        0x3C0040,
        0x80080C9,
        0x4E42,
        0x4B3,
        0xCC803C,
        0x580005,
};

ISR(WDT_vect) {
    WDTCSR |= _BV(WDIE);
}

int8_t freqSelector = -1;

void setup() {
    cli();
    MCUSR &= ~_BV(WDRF);
    WDTCSR |= (_BV(WDCE) | _BV(WDE));   // Enable the WD Change Bit
    WDTCSR =   _BV(WDIE) |              // Enable WDT Interrupt
              _BV(WDP2) | _BV(WDP1);   // Set Timeout to ~1 seconds (or something)
    sei();

    pinMode(0, INPUT);
    digitalWrite(0, HIGH);
    pinMode(5, INPUT); // LD

    pinMode(7, OUTPUT); // LE
    digitalWrite(7, LOW);
    pinMode(8, OUTPUT); // CE
    digitalWrite(8, HIGH);
    SPI.begin();
}

void goToSleep(void) {
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_enable();
    ADCSRA &= ~_BV(ADEN);
    sleep_cpu();
    sleep_disable();
}

void setupADF(uint8_t cfgToUse) {
    adfRegisters *registers = (adfRegisters *) (cfgToUse == 0 ? &freq_1 : &freq_2);

    for (int x = 5; x > -1; x--) {
        digitalWrite(7, LOW);
        delayMicroseconds(20);
        for (int8_t i = 3; i > -1; i--) {
            SPI.transfer((uint8_t )((*registers)[x] >> (i * 8)));
        }
        digitalWrite(7, HIGH);
        delayMicroseconds(5);
        digitalWrite(7, LOW);
        delay(2);
    }
}


void loop() {
    uint8_t needSetting = 0;

    int freqSelectorValue = digitalRead(0);
    int lockValue = digitalRead(5);
    if (freqSelectorValue != freqSelector) {
        needSetting = 1;
        freqSelector = freqSelectorValue > 0 ? 1 : 0;
    } else if (!lockValue) {
        needSetting = 1;
    }

    if (needSetting) {
        setupADF(freqSelector);
    }

    goToSleep();
}
