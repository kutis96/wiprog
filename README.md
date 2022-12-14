# WiProg

Program SPI flash memories wirelessly using this very crude ESP8266-based flasher!

## What is this?

An Arduino project for ESP8266, using LittleFS for storage of static files.

It's a thing that programs SPI flash chips when asked to do so!

However, <big>**this project is a huge hack**</big>. The web server is synchronous, meaning that only one request can be
processed at a
time. It is still, however, somewhat useful.

## What this isn't

- It isn't stable, production-grade, or performance-oriented. It's meant to work often enough for development purposes.
- It isn't performant or asynchronous. It's all synchronous, in fact. It will break.
- It isn't amazingly well written. It is a huge mess, in fact.
- It isn't well-documented. It works on my machine. I've forgotten why it does. I may document it better, when I one day
  try to build this on a different machine.
- It isn't a product meant to be sold or developed further. If you wish to turn this into something like that, good
  luck. You might want to start from scratch, though.
- It isn't a remote AVR programmer. Maybe it could become one, someday. Probably not. You can try that, if you want.
- It isn't going to be maintained. Feel free to fork it and maintain it yourself, if you find it useful.

Do not use this in production of any sort.

It is meant to be used for development purposes only - that is, on some devboard on your breadboard, feeding some other
devboard's SPI flash with data.
I use it to program my FPGA devboard's SPI flash, since it doesn't actually support JTAG-based programming, just flash.

## How do I use this?

Somehow get yourself an ESP8266 devboard; in this case a WeMos D1 mini.

Connect to SPI flash as follows:

| Signal name | DevBoard pin | ESP8266 pin | Description |
| - | - | - | - |
| **SCK**   | D5 | GPIO14 | SPI serial clock |
| **MISO**  | D6 | GPIO12 | SPI target data out |
| **MOSI**  | D7 | GPIO13 | SPI target data in |
| **CS**    | D8 | GPIO15 | SPI target select. Active low. |
| **RESET** | D0 | GPIO16 | Target reset when held low. To be used to disable other devices from accessing the SPI flash
while it's being programmed. |

## "API"

One could pretend this exposes some kind of API to the outside world.

It works somewhat like this: (all requests return `text/plain` responses unless said otherwise)

### `GET /api/v1/targetreset`

Queries the status of the target `RESET` pin. Returns `true` or `false`.

### `POST /api/v1/targetreset`

Sets `RESET` low to hold target in reset until released.

### `POST /api/v1/targetrelease`

Sets `RESET` high to release target from reset.

### `GET /api/v1/jid`

Returns the connected SPI flash's JEDEC ID.

If flash is disonnected, this typically returns all F's or all 0's.

### `GET /api/v1/uid`

Returns the connected SPI flash's unique ID.

If flash is disonnected, this typically returns all F's or all 0's.

### `GET /api/v1/flash.bin`

Reads out and returns the contents of the connected flash IC as `application/octet-stream`.

### `POST /api/v1/flash.bin`

Erases and programs the connected flash IC to the contents of submitted payload.

The target is put into reset while programming happens, and released from reset, regardless of whether it was or wasn't
in reset before programming.

### `GET /api/v1/hostname`

Returns the device's mDNS hostname.

## Configuration

All configuration is now stored as multiple files within `data/config/` directory.

These files are to be uploaded to the ESP8266's LittleFS.

**You will have to configure these yourself before use.**

To do that, you'll have to:

- Create a directory under `data/` of this repository named `config`.
- In `data/config/` directory, add files with the following names (no extensions, edit them in notepad or something. Do not insert newlines at the end of the file.):

- `wifi-ssid`
  - Must contain the SSID of the WiFi network WiProg will connect to
  - defaults to `SSID` when not set
- `wifi-pass`
  - Must contain the password of the WiFi network WiProg will connect to
  - defaults to `password` when not set
- `hostname`
  - Must contain the mDNS hostname
  - defaults to `wiprog`
  - You may then simply connect to `{hostname}.local` (like http://wiprog.local) in your web browser, should your system support mDNS.
    - This functionality is still untested as of now.

## Building

This project is now using [PlatformIO](https://platformio.org/)!

Build & upload binary:
- `pio run -e ${target_board}` -t upload --upload-port ${target_port}`

Build & upload LittleFS image:
- `pio run -e ${target_board}` -t uploadfs --upload-port ${target_port}`

Replace `${target_board}` with `d1_mini_pro` (currently the only supported board).

Replace `${target_port}` with the COM port the board is connected to (like COM9 on Windows).

## Attributions

The original Web server code was shamelessly repurposed from _some_ tutorial on-line.
I have since forgotten where it was taken from.
If the code is yours, I'll happily credit you here.
Likewise, if you recognize the code as someone else's, please link it in an issue, so I can attribute it.
