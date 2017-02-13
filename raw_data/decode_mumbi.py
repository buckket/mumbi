#!/usr/bin/env python3

import sys
import csv

PULSE_LENGTH = 330 / 10e5
PULSE_TOLERANCE = 60 / 10e5

if __name__ == '__main__':
    if len(sys.argv) != 2:
        print("Usage: {name} FILE".format(name=sys.argv[0]))
        sys.exit(1)

    with open(sys.argv[1]) as csvfile:
        raw_data = tuple(csv.reader(csvfile))

    pulses = []
    for i in range(1, len(raw_data) - 1):
        pulses.append((int(raw_data[i][1]), float(raw_data[i + 1][0]) - float(raw_data[i][0])))

    i = 0
    while i < len(pulses) - 1:
        if abs(pulses[i][1] - (1 * PULSE_LENGTH)) <= (1 * PULSE_TOLERANCE) and abs(pulses[i+1][1] - (3 * PULSE_LENGTH)) <= (3 * PULSE_TOLERANCE):
            print(0, end="")
            i += 2
        elif abs(pulses[i][1] - (3 * PULSE_LENGTH)) <= (3 * PULSE_TOLERANCE) and abs(pulses[i+1][1] - (1 * PULSE_LENGTH)) <= (1 * PULSE_TOLERANCE):
            print(1, end="")
            i += 2
        else:
            print("")
            i += 1
