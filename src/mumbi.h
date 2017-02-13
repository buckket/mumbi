#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#include <mraa.h>

#define DEFAULT_DATA_PIN 7
#define DEFAULT_POWER_PIN 8

int transmit(uint8_t data_pin, uint8_t power_pin, uint8_t channel, uint8_t status);
