# Zthulhu (Zephyr port of Pythulhu)

Zephyr-based port of [Pythulhu](https://github.com/LostinTimeandspaceYT/pythulhu)

## Prerequisites

1) mise
2) uv
3) knowledge of working with zephyr

## Project setup

1) Create and activate a Python venv, then install Zephyr requirements.
2) Initialize Zephyr (west workspace already present in `../zephyr`).
3) Build and flash.

Example build command:

```sh
west build -p always -b adafruit_metro_rp2350/rp2350b/m33 --shield adafruit_2_8_tft_touch_v2 applications/app
```

If your board uses UF2 flashing, copy `build/zephyr/zephyr.uf2` to the mounted
RP2350 drive.

## Hardware

- MCU board: [Adafruit Metro RP2350 (RP2350B M33)](https://docs.zephyrproject.org/latest/boards/adafruit/metro_rp2350/doc/index.html)
- Display: [Adafruit 2.8" TFT Touch Shield v2 (ILI9340, rev E restistive version)](https://docs.zephyrproject.org/latest/boards/shields/adafruit_2_8_tft_touch_v2/doc/index.html)

**Notes:**

- TFT CS = D10, TFT DC = D9, SD Card CS = D4.
- The shield blocks the SWD header and UART pins, so USB CDC is used for serial output.

## Goal

Port the Pythulhu UI and behavior to Zephyr (Zthulhu), with LVGL-based rendering,
USB CDC serial, and support for the shield's display, touch, and input devices.

## TODO

- Create TSC2007 driver (I2C touch controller used by the shield).
- Create plan for porting the rotary encoder breakout.
- Port UI elements from Pythulhu to Zthulhu.
