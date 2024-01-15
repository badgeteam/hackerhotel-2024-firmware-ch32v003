#include "ch32v003fun.h"
#include "i2c_slave.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// Firmware version
#define FW_VERSION 3

// I2C registers
#define I2C_REG_FW_VERSION_0 0  // LSB
#define I2C_REG_FW_VERSION_1 1  // MSB
#define I2C_REG_RESERVED_0   2
#define I2C_REG_RESERVED_1   3
#define I2C_REG_LED_0        4  // LSB
#define I2C_REG_LED_1        5
#define I2C_REG_LED_2        6
#define I2C_REG_LED_3        7  // MSB
#define I2C_REG_BTN_0        8  // Button 0
#define I2C_REG_BTN_1        9  // Button 1
#define I2C_REG_BTN_2        10 // Button 2
#define I2C_REG_BTN_3        11 // Button 3
#define I2C_REG_BTN_4        12 // Button 4

uint8_t i2c_registers[255] = {0};

uint8_t curr_i2c_registers[sizeof(i2c_registers)] = {0};
uint8_t prev_i2c_registers[sizeof(i2c_registers)] = {0};

bool i2c_changed = false;
bool i2c_buttons_read = false;

uint8_t prev_buttons[5] = {0};

void set_irq(bool active) {
    GPIOC->BSHR |= 1 << (4 + (active ? 16 : 0)); // Pull PC4 low when active, else float high
}

void i2c_read_callback(uint8_t reg) {
    if (reg >= I2C_REG_BTN_0 && reg <= I2C_REG_BTN_4) {
        i2c_buttons_read = true;
    }
}

void i2c_stop_callback(uint8_t reg, uint8_t length) {
    i2c_changed = true;
}

void control_leds(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint8_t e) {
    GPIOA->CFGLR &= ~((0xf<<(4*1)) | (0xf<<(4*2)));
    GPIOC->CFGLR &= ~((0xf<<(4*0)) | (0xf<<(4*3)));
    GPIOD->CFGLR &= ~(0xf<<(4*0));

    if (a > 1) {
        GPIOD->CFGLR |= (GPIO_Speed_In | GPIO_CNF_IN_FLOATING) << (4 * 0);
    } else {
        GPIOD->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP) << (4 * 0);
        GPIOD->BSHR  |= 1 << (0 + (a ? 16 : 0));
    }

    if (b > 1) {
        GPIOC->CFGLR |= (GPIO_Speed_In | GPIO_CNF_IN_FLOATING) << (4 * 0);
    } else {
        GPIOC->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP) << (4 * 0);
        GPIOC->BSHR  |= 1 << (0 + (b ? 16 : 0));
    }

    if (c > 1) {
        GPIOA->CFGLR |= (GPIO_Speed_In | GPIO_CNF_IN_FLOATING) << (4 * 1);
    } else {
        GPIOA->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP) << (4 * 1);
        GPIOA->BSHR  |= 1 << (1 + (c ? 16 : 0));
    }

    if (d > 1) {
        GPIOC->CFGLR |= (GPIO_Speed_In | GPIO_CNF_IN_FLOATING) << (4 * 3);
    } else {
        GPIOC->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP) << (4 * 3);
        GPIOC->BSHR  |= 1 << (3 + (d ? 16 : 0));
    }

    if (e > 1) {
        GPIOA->CFGLR |= (GPIO_Speed_In | GPIO_CNF_IN_FLOATING) << (4 * 2);
    } else {
        GPIOA->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP) << (4 * 2);
        GPIOA->BSHR  |= 1 << (2 + (e ? 16 : 0));
    }
}

void leds_off() {
    control_leds(2,2,2,2,2);
}

void led_on(uint8_t led) { // Select one specific LED
    switch(led) {
        case 1:  return control_leds(2,2,0,2,1); // A
        case 2:  return control_leds(0,2,1,2,2); // B
        case 3:  return control_leds(2,2,2,0,1); // D
        case 4:  return control_leds(2,2,0,1,2); // E
        case 5:  return control_leds(0,1,2,2,2); // F
        case 6:  return control_leds(2,0,1,2,2); // G
        case 7:  return control_leds(2,0,2,2,1); // H
        case 8:  return control_leds(0,2,2,2,1); // I
        case 9:  return control_leds(2,0,2,1,2); // K
        case 10: return control_leds(0,2,2,1,2); // L
        case 11: return control_leds(1,2,0,2,2); // M
        case 12: return control_leds(1,0,2,2,2); // N
        case 13: return control_leds(2,1,2,2,0); // O
        case 14: return control_leds(2,2,1,2,0); // P
        case 15: return control_leds(1,2,2,2,0); // R
        case 16: return control_leds(2,1,0,2,2); // S
        case 17: return control_leds(2,2,1,0,2); // T
        case 18: return control_leds(1,2,2,0,2); // V
        case 19: return control_leds(2,1,2,0,2); // W
        case 20: return control_leds(2,2,2,1,0); // Y
        default: return control_leds(2,2,2,2,2); // Off
    }
}

void read_buttons(uint8_t* values) {
    for (uint8_t button = 0; button < 5; button++) {
        GPIOD->BSHR |= 1 << (6 - button + 16);
        values[button] = (~(GPIOC->INDR >> 5)) & 0x07; // Read MSWR (bit 0), MSWB (bit 1) and MSWL (bit 2) all at once
        GPIOD->BSHR |= 1 << (6 - button);
    }
}

int main() __attribute__((optimize("O0")));
int main() {
    SystemInit();

    // Enable GPIO ports A, C and D
    RCC->APB2PCENR |= RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD;

    // Initialize I2C in peripheral mode on pins PC1 (SDA) and PC2 (SCL)
    SetupI2CSlave(0x42, 0, i2c_registers, sizeof(i2c_registers), i2c_stop_callback, i2c_read_callback);

    // Configure MSWS1 (PD6), MSWS2 (PD5), MSWS3 (PD4), MSWS4 (PD3), MSWS5 (PD2) for output open-drain mode
    // and set them all to high
    for (uint8_t i = 2; i <= 6; i++) {
        GPIOD->CFGLR &= ~(0xf<<(4*i));
        GPIOD->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_OD) << (4 * i);
        GPIOD->BSHR  |= 1 << i;
    }

    // Configure MSWL (PC7), MSWB (PC6) and MSWR (PC5) as inputs
    for (uint8_t i = 5; i <= 7; i++) {
        GPIOC->CFGLR &= ~(0xf<<(4*i));
        GPIOC->CFGLR |= (GPIO_Speed_In | GPIO_CNF_IN_FLOATING) << (4 * i);
    }

    // Configure IRQ (PC4) as open drain output and set it to high (floating)
    GPIOC->CFGLR &= ~(0xf<<(4*4));
    GPIOC->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_OD) << (4 * 4);
    GPIOC->BSHR  |= 1 << 4;

    while (1) {
        if (i2c_changed) {
            memcpy(curr_i2c_registers, i2c_registers, sizeof(i2c_registers));
            i2c_changed = false;
        }

        if (i2c_buttons_read) {
            set_irq(false);
            i2c_buttons_read = false;
        }

        uint32_t led_values = (curr_i2c_registers[I2C_REG_LED_0]) |
                              (curr_i2c_registers[I2C_REG_LED_1] << 8) |
                              (curr_i2c_registers[I2C_REG_LED_2] << 16) |
                              (curr_i2c_registers[I2C_REG_LED_3] << 24);

        for (uint8_t i = 0; i <= 20; i++) {
            if ((led_values >> i) & 1) {
                led_on(i + 1);
            }
        }
        leds_off();

        // Write to registers
        i2c_registers[I2C_REG_FW_VERSION_0] = (FW_VERSION     ) & 0xFF;
        i2c_registers[I2C_REG_FW_VERSION_1] = (FW_VERSION >> 8) & 0xFF;

        uint8_t buttons[5] = {0};
        read_buttons(buttons);
        if (memcmp(prev_buttons, buttons, 5) != 0) {
            memcpy(&i2c_registers[I2C_REG_BTN_0], buttons, 5);
            memcpy(prev_buttons, buttons, 5);
            set_irq(true);
        }

        memcpy(prev_i2c_registers, curr_i2c_registers, sizeof(i2c_registers));
    }
}
