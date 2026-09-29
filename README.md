# moonlite-hw

Nighttime wind sound player and clock for an M5Stack AtomS3R with an Atomic Audio-3.5 Base. The sound plays from 23:00 to 06:00 in the Europe/Bratislava time zone after Wi-Fi time sync.

## Setup

Install PlatformIO, then create `src/secrets.h` locally:

```cpp
#pragma once
#define WIFI_SSID "your-2.4-ghz-network"
#define WIFI_PASSWORD "your-password"
```

Place a 16 kHz mono, 16-bit PCM WAV file with a standard 44-byte header at `data/wind.wav`. The firmware skips the header and loops the audio data. The original wind recording is not included because it is not licensed for redistribution.

```sh
pio run
pio run -t uploadfs
pio run -t upload
```

The display shows Wi-Fi status, local time, and whether the wind sound is playing. The screen button is currently unused.
