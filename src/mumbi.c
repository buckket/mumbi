/*
 * mumbi.c – control m-FS300 remote sockets
 * Copyright © 2017 buckket
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "mumbi.h"

/* Do you speak power socket?
 *
 * A data packet consists of 34 bits, a complete transmission repeats the data 8 times.
 * A short pulse/pause (smallest time unit) takes 330ms, a long pulse/pause takes three times as long (990ms).
 *
 * To transmit one bit of information it takes exactly 4 units of time:
 * A 0 consists of a short pulse followed by a long pause: |‾|___
 * A 1 consists of a long pulse followed by short pause:   |‾‾‾|_
 *
 * Example:
 * |‾‾‾|_ |‾‾‾|_ |‾|___ <and so on>
 *  1      1      0
 *
 * A complete data packet looks like this:
 *
 * Channel A On:
 *  01110010 01111110 00001111 00111101 (00)
 *    0x72     0x7E     0x0F     0x3D
 *
 * Channel A Off:
 *  01110010 01111110 00001110 00111100 (00)
 *    0x72     0x7E     0x0E     0x3C
 *
 * The first two bytes seem to be the "unique" house code, whilst the third and forth byte
 * seem to somehow encode the device identifier as well as the desired switch position.
 * How? Don’t know. Yet!? ;_;
 *
 * Other interesting findings:
 * The programmable power sockets seem to be capable of remembering more than one code.
 */

/*
 * Because we’re running a non-ROS with many delays we have to tweak the pulse length.
 * Feel free to modify the base value, depending on your platform, use logic analyzer if needed. :-)
 */
#define TIME_UNIT 235000 // works fine on my BeagleBone Black

const struct timespec short_pulse = {0, 1 * TIME_UNIT};
const struct timespec long_pulse = {0, 3 * TIME_UNIT};
const struct timespec long_pause = {0, 29 * TIME_UNIT};

const uint8_t data_packets[10][34] = {
        // Channel A On/Off:
        {0, 1, 1, 1, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0},
        {0, 1, 1, 1, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 0, 1, 0, 0},

        // Channel B On/Off:
        {0, 1, 1, 1, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0},
        {0, 1, 1, 1, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0},

        // Channel C On/Off:
        {0, 1, 1, 1, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0},
        {0, 1, 1, 1, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 1, 0, 0, 1, 0, 0},

        // Channel D On/Off:
        {0, 1, 1, 1, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 0, 1, 0, 0, 0, 0},
        {0, 1, 1, 1, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 1, 1, 0, 1, 0, 1, 0, 0},

        // All Channels On/Off:
        {0, 1, 1, 1, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 1, 0, 0},
        {0, 1, 1, 1, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 0, 1, 1, 1, 0, 0},
};

void send_0(mraa_gpio_context gpio) {
    mraa_gpio_write(gpio, 1);
    nanosleep(&short_pulse, NULL);
    mraa_gpio_write(gpio, 0);
    nanosleep(&long_pulse, NULL);
}

void send_1(mraa_gpio_context gpio) {
    mraa_gpio_write(gpio, 1);
    nanosleep(&long_pulse, NULL);
    mraa_gpio_write(gpio, 0);
    nanosleep(&short_pulse, NULL);
}

void send_sync(mraa_gpio_context gpio) {
    mraa_gpio_write(gpio, 1);
    mraa_gpio_write(gpio, 0);
    nanosleep(&long_pause, NULL);
}

int transmit(uint8_t data_pin, uint8_t power_pin, uint8_t channel, uint8_t status) {
    mraa_result_t r = MRAA_SUCCESS;

    mraa_init();
    mraa_gpio_context gpio_d, gpio_p;

    gpio_d = mraa_gpio_init(data_pin);
    if (gpio_d == NULL) {
        fprintf(stderr, "Are you sure that Pin%d you requested is valid on your platform?", data_pin);
        return -1;
    }

    gpio_p = mraa_gpio_init(power_pin);
    if (gpio_p == NULL) {
        fprintf(stderr, "Are you sure that Pin%d you requested is valid on your platform?", power_pin);
        return -1;
    }

    r = mraa_gpio_dir(gpio_d, MRAA_GPIO_OUT);
    if (r != MRAA_SUCCESS) {
        mraa_result_print(r);
        return r;
    }

    r = mraa_gpio_dir(gpio_p, MRAA_GPIO_OUT);
    if (r != MRAA_SUCCESS) {
        mraa_result_print(r);
        return r;
    }

    mraa_gpio_write(gpio_p, 1);

    uint8_t i, pos;
    for (i = 0; i < 8; i++) {
        for (pos = 0; pos < sizeof(data_packets[(2 * channel) + status]) / sizeof(uint8_t); pos++) {
            if (data_packets[(2 * channel) + status][pos]) {
                send_1(gpio_d);
            } else {
                send_0(gpio_d);
            }
        }
        send_sync(gpio_d);
    }

    mraa_gpio_write(gpio_p, 0);

    r = mraa_gpio_close(gpio_d);
    if (r != MRAA_SUCCESS) {
        mraa_result_print(r);
        return r;
    }

    r = mraa_gpio_close(gpio_p);
    if (r != MRAA_SUCCESS) {
        mraa_result_print(r);
        return r;
    }

    return 0;
}

int main(int argc, char **argv) {
    uint8_t data_pin = DEFAULT_DATA_PIN;
    uint8_t power_pin = DEFAULT_POWER_PIN;
    uint8_t channel, status;

    if (argc < 3) {
        printf("Usage: %s CHANNEL STATUS [DATAPIN] [POWERPIN]\n", argv[0]);
        exit(1);
    }
    if (argc >= 5) {
        power_pin = strtol(argv[4], NULL, 10);
    }
    if (argc >= 4) {
        data_pin = strtol(argv[3], NULL, 10);
    }
    if (argc >= 3) {
        channel = strtol(argv[1], NULL, 10);
        status = strtol(argv[2], NULL, 10);
    }

    if (transmit(data_pin, power_pin, channel, status) != 0) {
        return -1;
    } else {
        return 0;
    };
}
