# AOA audio

The purpose of this PoC is to find a solution to [forward audio][issue14] in
[scrcpy].

[issue14]: https://github.com/Genymobile/scrcpy/issues/14
[scrcpy]: https://github.com/Genymobile/scrcpy

## Build

Install the following packages (on _Debian_):

    sudo apt install libusb-1.0-0-dev ffmpeg

Build the tool:

    make


## Run

Find your Android device _vendor id_ and _product id_ in the output of `lsusb`:

    $ lsusb
    ...
    Bus 003 Device 011: ID 18d1:4ee2 Google Inc. Nexus 4 (debug)
    ...

Here, the _vendor id_ is `18d1` and the _product id_ is `4ee1`.

Then enable AOA audio:

    ./audio 18d1 4ee1 1

The last argument (`1`) is the [audio mode] (provided in the `value` field).

[audio mode]: https://source.android.com/devices/accessories/aoa2#audio-support

The output should look like:

    $ ./audio 18d1 4ee1 1
    Device 18d1:2d05 found. Opening...
    Setting audio mode: 1
    SUCCESS

_Note that changing the audio mode changes the [device product id][pid]._

[pid]: https://source.android.com/devices/accessories/aoa2#detecting-android-open-accessory-20-support

Once enabled, your computer should detect a new input audio device. List the
sources (provided your system uses _PulseAudio_):

    pactl list short sources

You should see your device:

    $ pactl list short sources
    ...
    5   alsa_input.usb-LGE_Nexus_5_05f5e60a0ae518e5-01.analog-stereo     module-alsa-card.c  s16le 2ch 44100Hz   RUNNING

or:

    $ ffmpeg -sources pulse
    ...
      alsa_input.usb-LGE_Nexus_5_05f5e60a0ae518e5-00.analog-stereo [Nexus 5 Stéréo analogique]

Play the sound with `ffplay`:

    ffplay -vn -nodisp -f pulse -i 5

or with the full name:

    ffplay -vn -nodisp -f pulse -i alsa_input.usb-LGE_Nexus_5_05f5e60a0ae518e5-01.analog-stereo

The sound of your Android device should be played on your computer.


## Issues

This PoC works, but there are problems to solve before this can be implemented
in _scrcpy_.


### Find the USB device

How to find the VID and PID of the device from adb? Is there a better way than
matching the serial from `adb devices` with the USB `iSerial` field?

```bash
lsusb -d 18d1:4ee2 -v | grep iSerial | grep -o '[0-9a-f]\+$'
```

### Stop accessory

The accessory may not be stopped easily. Setting mode to `0` is not sufficient,
so the device sound is redirected until the device is rebooted.


### Find the audio source

Enabling audio accessory creates a new audio source on the computer, but how to
find it reliably and automatically? What if the system does not use
_PulseAudio_?

It would be great if it were possible to receive audio packets manually, so that
we could play them with SDL directly.


### Non-Linux system

The PoC uses _libusb_. What about systems like _Windows_?
