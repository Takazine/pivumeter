# Pi VU Meter

## ALSA Plugin to display a VU meter on various Raspberry Pi add-ons

This library is free software; you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation; either version 2.1 of the License, or (at your option) any later version.

In accordance with this license, we have chosen to publish Pi VU Meter under GPL version 3.0.

Pi VU Meter is Derived from the SDL display version of the ameter ALSA level meter plugin.

### Setup

Currently, you will need to compile the pivumeter lib from source.

#### pre-requisite

```
sudo apt-get install build-essential autoconf automake libtool libasound2-dev libfftw3-dev wiringpi git i2c-tools
```

#### clone repository

if you haven't done so already, git clone this repository and cd into it:

```
git clone https://github.com/Takazine/pivumeter
cd pivumeter
```

#### Install

You may use our automated setup to do so, specifying the output device the wish to use (see `options` below):

```
setup.sh i2c-led-bar
```

Setup is complete.

if you'd rather compile and set things up yourself, read on!



#### compiling from source


from the top of the repository, run the following to generate the MakeFile:

```
aclocal && libtoolize
autoconf && automake --add-missing
```

then, to compile and install:

```
./configure && make
sudo make install
```

### Options

Pi VU Meter supports various options in your `/etc/asound.conf`, these include:

#### output_device

Specify which device to display the VU meter on.

Supported devices:

* i2c-led-bar (16-segment LED driven by PCA9685)
* i2c-analog-vu (analog VU Meter driven by PCA9685)
* default (18-segment VU driven by SN3218)
* blinkt - Simple amplitude meter through Green->Yellow->Red
* phat-beat - Simple stereo amplitude meter
* speaker-phat - Simple mono amplitude meter
* scroll-phat - displays 11-band FFT-based EQ
* scroll-phat-hd - displays 17-band FFT-based EQ

#### brightness

Specify the pixel brightness from 0 to 255

#### Example

```
pcm_scope.pivumeter {
        type pivumeter
        decay_ms 500
        peak_ms 400
        brightness 128
        output_device blinkt
}
```

a full example of a suitable `asound.conf` file can be found in the `dependencies` directory.
