# scoreboard-firmware

Firmware to go with the [VCW scoreboard hardware](https://github.com/ianrrees/scoreboard-hardware) - see http://ianrrees.github.io/2017/04/07/scoreboard.html .

There are two separate firmwares in this repository:

`i2c_lcd_mockup` contains an Arduino "Sketch" to drive a standard LCD display over i2c. This was useful for prototyping the Arduino software to drive the whole scoreboard, before the PCB for the scoreboard digits was ready.

The second firmware, in the `start` directory, is meant to be loaded on the ATSAMD10 in each digit of the scoreboard. It's a "makefile project" based on the ASFv4 via [Atmel START](http://start.atmel.com/), and is intended to be built like:

```
cd start/gcc
make
```

If you use MacOS, I've written a somewhat scattered set of notes on getting setup for building firmware for ARM chips like this - [blog](http://ianrrees.github.io/2017/04/30/getting-started-with-atsamd21-development-on-macos.html).

Further, if you're using an FT2232-based programmer tool like the [Bus Blaster](http://dangerousprototypes.com/docs/Bus_Blaster) that's compatible with the OpenOCD "KT-link" configuration, and [appropriate adapters](http://dirtypcbs.com/store/designer/details/9294/3545/busblaster-to-swd-gerbers-zip) for the board (it's designed for a [6-pin Tag Connect](http://www.tag-connect.com/TC2030-IDC), sorry not more standard - I use these a lot for my day job and find them quite nice) the firmware can be loaded via: 

```
make install
```

The firmware essentially just provides an IIC slave interface with the address selectable via the 3 addressing solder jumpers (see main.c for details). The IIC protocol is super easy - just write a byte between 0 and 9 to display that digit, or 0xff to turn off the display, and and the firmware does the right thing. It's complete overkill to use a 32-bit micro for this job, but it was the cheapest ARM micro available on digikey when I was designing the board - $1.03USD in small quantities!

The only "gotcha" I'm aware of, is that the heartbeat LED is driven from the reset pin on the SAMD, but that pin needs to be an input for programming. The firmware includes a timer to wait a couple seconds before turning on the heartbeat LED - if you need to reprogram a board just power cycle it right before trying to load firmware.
