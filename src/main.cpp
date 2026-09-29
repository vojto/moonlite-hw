#include <M5Unified.h>
#include <M5EchoBase.h>
#include <LittleFS.h>
#include <WiFi.h>
#include "secrets.h"

// Europe/Bratislava, including daylight saving time changes
#define TIME_ZONE "CET-1CEST,M3.5.0,M10.5.0/3"

// data/wind.wav is 16 kHz mono 16-bit; samples start after the 44-byte header
#define SAMPLE_RATE 16000
#define WAV_HEADER_SIZE 44
#define CHUNK_FRAMES 512

M5EchoBase echobase(I2S_NUM_0);
volatile bool isWindOn = false;

// Loops the wind file forever; plays silence while the wind is off
void windTask(void *) {
  static int16_t mono[CHUNK_FRAMES];
  static int16_t stereo[CHUNK_FRAMES * 2];
  File file = LittleFS.open("/wind.wav");
  file.seek(WAV_HEADER_SIZE);

  while (true) {
    int frames = CHUNK_FRAMES;
    if (isWindOn) {
      frames = file.read((uint8_t *)mono, sizeof(mono)) / 2;
      if (frames < CHUNK_FRAMES) file.seek(WAV_HEADER_SIZE);
      if (frames == 0) continue;
    } else {
      memset(mono, 0, sizeof(mono));
    }

    for (int i = 0; i < frames; i++) {
      stereo[i * 2] = mono[i];
      stereo[i * 2 + 1] = mono[i];
    }
    echobase.play((uint8_t *)stereo, frames * 4, false);
  }
}

void drawWifiStatus() {
  M5.Display.setTextSize(1);
  M5.Display.setCursor(4, 4);
  if (WiFi.status() == WL_CONNECTED) {
    M5.Display.setTextColor(TFT_GREEN, TFT_BLACK);
    M5.Display.printf("%-21s", WiFi.localIP().toString().c_str());
  } else {
    M5.Display.setTextColor(TFT_ORANGE, TFT_BLACK);
    M5.Display.printf("%-21s", "Connecting...");
  }
}

void drawTime() {
  struct tm now;
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  if (!getLocalTime(&now, 0)) {
    M5.Display.setTextSize(1);
    M5.Display.setCursor(4, 60);
    M5.Display.print("Syncing time...");
    return;
  }

  char text[16];
  M5.Display.setTextSize(2);
  M5.Display.setCursor(16, 48);
  strftime(text, sizeof(text), "%H:%M:%S", &now);
  M5.Display.print(text);

  M5.Display.setTextSize(1);
  M5.Display.setCursor(34, 80);
  strftime(text, sizeof(text), "%a %d %b", &now);
  M5.Display.print(text);
}

void drawWindStatus() {
  M5.Display.setTextSize(1);
  M5.Display.setCursor(4, 116);
  M5.Display.setTextColor(TFT_SKYBLUE, TFT_BLACK);
  M5.Display.printf("%-21s", isWindOn ? "Wind: on" : "Wind: off");
}

void showError(const char *text) {
  M5.Display.fillScreen(TFT_RED);
  M5.Display.setTextColor(TFT_WHITE);
  M5.Display.setCursor(4, 60);
  M5.Display.print(text);
  while (true) delay(1000);
}

void setup() {
  M5.begin();
  M5.Display.setBrightness(80);  // 0-255
  M5.Display.fillScreen(TFT_BLACK);

  if (!LittleFS.begin()) showError("No files :(");
  // Pins for AtomS3R: I2C SDA, SCL, then I2S DIN, WS, DOUT, BCK
  if (!echobase.init(SAMPLE_RATE, 38, 39, 7, 6, 5, 8, Wire)) showError("No base :(");
  echobase.setSpeakerVolume(70);
  echobase.setMute(false);
  xTaskCreate(windTask, "wind", 4096, nullptr, 2, nullptr);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  // Syncs time from the internet once connected, then again every hour
  configTzTime(TIME_ZONE, "pool.ntp.org");
}

void loop() {
  M5.update();

  static bool wasTimeSynced = false;
  struct tm now;
  bool isTimeSynced = getLocalTime(&now, 0);
  // Plays at night, from 23:00 until 6:00
  isWindOn = isTimeSynced && (now.tm_hour >= 23 || now.tm_hour < 6);
  if (isTimeSynced != wasTimeSynced) {
    M5.Display.fillScreen(TFT_BLACK);
    wasTimeSynced = isTimeSynced;
  }

  drawWifiStatus();
  drawTime();
  drawWindStatus();
  delay(20);
}
