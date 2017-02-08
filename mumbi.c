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

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#include <mraa.h>


#define DEFAULT_IOPIN 8


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


/*
 * Commands:
 *
 * 0: Channel A: On
 * 1: Channel A: Off
 *
 * 2: Channel B: On
 * 3: Channel B: Off
 *
 * 4: Channel C: On
 * 5: Channel C: Off
 *
 * 6: Channel D: On
 * 7: Channel D: Off
 *
 * 8: Every Channel: On
 * 9: Every Channel: Off
 */
const uint8_t data_packets[10][34] = {
        {0, 1, 1, 1, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 0, 1, 0, 0},
        {0, 1, 1, 1, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0},
        {0, 1, 1, 1, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0},
        {0, 1, 1, 1, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0},
        {0, 1, 1, 1, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 1, 0, 0, 1, 0, 0},
        {0, 1, 1, 1, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0},
        {0, 1, 1, 1, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 1, 1, 0, 1, 0, 1, 0, 0},
        {0, 1, 1, 1, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 0, 1, 0, 0, 0, 0},
        {0, 1, 1, 1, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 0, 1, 1, 1, 0, 0},
        {0, 1, 1, 1, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 1, 0, 0},
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

int main(int argc, char **argv) {
    mraa_result_t r = MRAA_SUCCESS;
    uint8_t iopin = DEFAULT_IOPIN;
    uint8_t command = 0;

    if (argc < 2) {
        printf("Usage: %s COMMAND [IOPIN]\n", argv[0]);
        exit(1);
    }
    if (argc >= 3) {
        iopin = strtol(argv[2], NULL, 10);
    }
    if (argc >= 2) {
        command = strtol(argv[1], NULL, 10);
    }

    mraa_init();
    mraa_gpio_context gpio;

    gpio = mraa_gpio_init(iopin);
    if (gpio == NULL) {
        fprintf(stderr, "Are you sure that Pin%d you requested is valid on your platform?", iopin);
        exit(1);
    }

    r = mraa_gpio_dir(gpio, MRAA_GPIO_OUT);
    if (r != MRAA_SUCCESS) {
        mraa_result_print(r);
        exit(1);
    }

    for (int i = 0; i < 8; i++) {
        for (int pos = 0; pos < sizeof(data_packets[command]) / sizeof(uint8_t); pos++) {
            if (data_packets[command][pos]) {
                send_1(gpio);
            } else {
                send_0(gpio);
            }
        }
        send_sync(gpio);
    }

    r = mraa_gpio_close(gpio);
    if (r != MRAA_SUCCESS) {
        mraa_result_print(r);
    }

    return r;
}
