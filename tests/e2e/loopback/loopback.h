// Copyright (c) 2022 Richard Gemmell
// Released under the MIT License. See license.txt. (https://opensource.org/licenses/MIT)

#ifndef TEENSY4_I2C_LOOPBACK_H
#define TEENSY4_I2C_LOOPBACK_H

#include <cstdint>
#include <Arduino.h>

namespace e2e {
namespace loopback {

// Contains tools and config information for the loopback
// test harness board. This board:
//   - connects all 3 I2C ports to a single bus
//   - has external pullup resistors to give the following rise times:
//      -  ~30 ns - the fastest possible rise time for this board
//      -  120 ns - max rise time for Fast Mode Plus
//      -  300 ns - max rise time for Fast Mode
//      - 1000 ns - max rise time for Standard Mode
class Loopback {
public:
    // Pins for monitors SDA and SCL
    const static uint8_t PIN_SNIFF_SDA = 23;
    const static uint8_t PIN_SNIFF_SCL = 22;

    // WARNING: Turn Trimpots clockwise to increase resistance
    // Fastest currently use a 470 Ω resistor. This gives a drain
    // current of 7 mA. This is too much for the I2C spec but is
    // Ok for the Teensy 4 which supports 10 mA.
    const static uint8_t PIN_SCL_FASTEST = 11; // < 30ns with 470 Ω resistor
    const static uint8_t PIN_SCL_120_ns = 10;
    const static uint8_t PIN_SCL_300_ns = 9;
    const static uint8_t PIN_SCL_1000_ns = 8;
    const static uint8_t PIN_SDA_FASTEST = 14; // < 30ns with 470 Ω resistor
    const static uint8_t PIN_SDA_120_ns = 15;
    const static uint8_t PIN_SDA_300_ns = 20;
    const static uint8_t PIN_SDA_1000_ns = 21;

    static void enable_pullup(uint8_t pin) {
        pinMode(pin, OUTPUT);
        delayNanoseconds(200);
        digitalWrite(pin, 1);
    }

    // Setting unused pins to INPUT_DISABLE significantly reduces
    // the line capacitance.
    static void disable_all_pullups() {
        pinMode(PIN_SCL_FASTEST, INPUT_DISABLE);
        pinMode(PIN_SCL_120_ns, INPUT_DISABLE);
        pinMode(PIN_SCL_300_ns, INPUT_DISABLE);
        pinMode(PIN_SCL_1000_ns, INPUT_DISABLE);

        pinMode(PIN_SDA_FASTEST, INPUT_DISABLE);
        pinMode(PIN_SDA_120_ns, INPUT_DISABLE);
        pinMode(PIN_SDA_300_ns, INPUT_DISABLE);
        pinMode(PIN_SDA_1000_ns, INPUT_DISABLE);
    }
};

}
}
#endif //TEENSY4_I2C_LOOPBACK_H
