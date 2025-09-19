#ifndef TCS_H
#define TCS_H

#include "hardware/i2c.h"
#include "pico/stdlib.h"
#include <stdio.h>

#define I2C_PORT i2c0
#define I2C_SDA 0
#define I2C_SCL 1

#define TCS34725_ADDRESS 0x29
#define COMMAND_BIT 0x80

#define ENABLE_REG 0x00
#define ATIME_REG 0x01
#define CONTROL_REG 0x0F

#define CDATAL 0x14
#define RDATAL 0x16
#define GDATAL 0x18
#define BDATAL 0x1A

void config_i2c();
void tcs_write8(uint8_t reg, uint8_t value);
uint16_t tcs_read16(uint8_t reg);
void tcs_init();
void tcs_enable();
void tcs_disable();
const char *detect_color(uint16_t r, uint16_t g, uint16_t b);
int read_color();

#endif