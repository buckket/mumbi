mumbi
~~~~~

Recently I bought a bunch of `wireless remote control power sockets <https://www.amazon.de/mumbi-FS300-Funksteckdosen-Funksteckdose-Fernbedienung/dp/B002UJKW7K>`_.
I wanted to control them not only via the supplied remote, but also via my computer. There’re numerous solutions out there,
but somehow none supports this exact model I’ve got, which is a "self-learning" one, meaning there’re no DIP-switches to select the
device and/or house code. I hooked up my logic analyzer and grabbed the raw signal straight from the remote. I was not
successful decoding the protocol so far, but at least I can mimic it. All you need is a 433/434 MhZ RF Transmitter and an
embedded Linux platform with GPIOs like a BeagleBone Black or Raspberry Pi.

This repo includes a simple command line app as well as Python extension to control such devices.

Check out ``src/mumbi.c`` for some information about the protocol and how this works.

Requirements
------------

- `mraa <https://github.com/intel-iot-devkit/mraa>`_

License
-------

mumbi is released under the GPLv3+ License. See the bundled LICENSE file for details.
