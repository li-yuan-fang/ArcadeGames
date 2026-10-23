# ArcadeGames

[中文](./README.md) | English

## Overview

ArcadeGames is a USB HID gamepad demo built around the CH585 microcontroller. It supports USB 2.0 High-Speed operation for low-latency input and can be extended to support additional buttons.

~~Yes, you can use it to play rhythm games.~~

## Features

- USB 2.0 support
- High-Speed and Full-Speed operation
- Timer-based software debouncing
- Two device modes: standard HID Gamepad and vendor-defined HID
- Theoretical latency as low as 125 μs

## Test Setup

For convenience, testing was done with a CH585 EVB evaluation board and a third-party button board.

> Designing and assembling your own PCB is recommended for a more affordable setup.
>
> The button board used for testing has external pull-up resistors, so the firmware does not enable the chip's internal pull-ups. Adjust this as needed for your hardware.

The demo was tested on Windows 11 in osu!lazer's mania 4K mode, with no additional drivers required. Despite the button board's poor tactile feel, the overall gameplay experience was much better than with a typical USB keyboard.

## Building and Flashing

The project was created with [MounRiver Studio](https://www.mounriver.com/), WCH's official IDE. Open the project in MounRiver Studio to build and flash the firmware.

> **Build and flashing tips**
>
> The project files use UTF-8 encoding. Set MounRiver Studio to use UTF-8 instead of its default GBK encoding.
>
> CH58X chips support USB and UART flashing out of the box. Hold down the Boot button while powering on the board to enter bootloader mode.
>
> Before you can use WCH-Link for two-wire programming, you need to unlock the chip with the WCH ISP tool. Enter bootloader mode using the Boot button, connect through WCH ISP, and unlock the chip. You can then program it with WCH-Link.
