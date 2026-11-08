#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <TJpg_Decoder.h>

#if __has_include("config.h")
#include "config.h"
#else
#include "config.example.h"
#endif

TFT_eSPI tft = TFT_eSPI();

#define TOUCH_CS 33
#define TOUCH_IRQ 36
#define TOUCH_SCK 25
#define TOUCH_MISO 39
#define TOUCH_MOSI 32

SPIClass touchSPI(VSPI);
XPT2046_Touchscreen touch(TOUCH_CS, TOUCH_IRQ);

// Wi-Fi and the server address come from config.h (never committed).
static const char* const ssid = WIFI_SSID;
static const char* const password = WIFI_PASSWORD;

static String serverBase() {
  return String("http://") + SPOTIFY_SERVER_HOST + ":" +
         String(SPOTIFY_SERVER_PORT);
}

// Spotify information
String song = "Loading...";
String artist = "";
String coverURL = "";
String lastCoverURL = "";

bool playing = false;

// Scrolling
int scrollX = 0;
unsigned long lastScroll = 0;


// ==================================================
// BOOT ANIMATION
// ==================================================

void bootText(const char* text, bool success = true) {

  tft.setTextSize(1);

  if (success) {
    tft.setTextColor(TFT_GREEN);
  } else {
    tft.setTextColor(TFT_RED);
  }

  tft.drawString(text, 15, tft.getCursorY());

  tft.setCursor(15, tft.getCursorY() + 14);
}


void bootProgress(int percent) {

  int x = 15;
  int y = 205;
  int w = 290;
  int h = 12;

  tft.drawRect(
    x,
    y,
    w,
    h,
    TFT_DARKGREY
  );

  int fillWidth =
    map(percent, 0, 100, 0, w - 4);

  if (fillWidth > 0) {

    tft.fillRect(
      x + 2,
      y + 2,
      fillWidth,
      h - 4,
      TFT_GREEN
    );
  }

  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(1);

  String percentText =
    String(percent) + "%";

  tft.drawCentreString(
    percentText,
    160,
    220,
    1
  );
}


void bootScreen() {

  tft.fillScreen(TFT_BLACK);

  // Logo
  tft.setTextColor(TFT_GREEN);
  tft.setTextSize(3);

  tft.drawCentreString(
    "SPOTIFY",
    160,
    15,
    2
  );

  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(1);

  tft.drawCentreString(
    "CYD CONTROLLER",
    160,
    48,
    1
  );

  // Divider
  tft.drawLine(
    15,
    70,
    305,
    70,
    TFT_DARKGREY
  );

  // Initial system messages
  tft.setTextColor(TFT_LIGHTGREY);
  tft.drawString(
    "SYSTEM INITIALIZATION",
    15,
    82
  );

  delay(300);

  tft.setTextColor(TFT_GREEN);
  tft.drawString(
    "[ OK ] Display driver",
    15,
    105
  );

  delay(250);

  tft.drawString(
    "[ OK ] TFT interface",
    15,
    120
  );

  delay(250);

  tft.drawString(
    "[ OK ] Touch controller",
    15,
    135
  );

  delay(250);

  tft.drawString(
    "[ OK ] JPEG decoder",
    15,
    150
  );

  delay(250);

  tft.drawString(
    "[ OK ] Memory manager",
    15,
    165
  );

  delay(300);

  // Progress
  for (int i = 0; i <= 100; i += 5) {

    bootProgress(i);

    delay(25);
  }

  delay(300);
}


// ==================================================
// WIFI BOOT
// ==================================================

void bootWiFi() {

  tft.fillScreen(TFT_BLACK);

  tft.setTextColor(TFT_GREEN);
  tft.setTextSize(2);

  tft.drawString(
    "NETWORK INIT",
    15,
    15
  );

  tft.setTextSize(1);
  tft.setTextColor(TFT_LIGHTGREY);

  tft.drawString(
    "Connecting to Wi-Fi...",
    15,
    50
  );

  tft.setTextColor(TFT_WHITE);

  tft.drawString(
    ssid,
    15,
    70
  );

  // WiFi.begin() is already called in setup().
  // DO NOT call it repeatedly here.

  unsigned long startTime = millis();

  int dots = 0;

  while (
    WiFi.status() != WL_CONNECTED &&
    millis() - startTime < 20000
  ) {

    delay(500);

    dots++;

    if (dots > 3) {
      dots = 0;
    }

    tft.fillRect(
      15,
      100,
      290,
      25,
      TFT_BLACK
    );

    tft.setTextColor(TFT_GREEN);

    String loading = "Connecting";

    for (int i = 0; i < dots; i++) {
      loading += ".";
    }

    tft.drawString(
      loading,
      15,
      105
    );

    // Animated progress bar
    int progress = 20 + (dots * 20);

    bootProgress(progress);
  }

  // -----------------------------------------------
  // CONNECTION RESULT
  // -----------------------------------------------

  tft.fillRect(
    15,
    100,
    290,
    60,
    TFT_BLACK
  );

  if (WiFi.status() == WL_CONNECTED) {

    tft.setTextColor(TFT_GREEN);

    tft.drawString(
      "[ OK ] Wi-Fi connected",
      15,
      105
    );

    tft.drawString(
      "[ OK ] Network interface",
      15,
      120
    );

    tft.setTextColor(TFT_LIGHTGREY);

    tft.drawString(
      WiFi.localIP().toString(),
      15,
      140
    );

    bootProgress(100);

    delay(700);

  } else {

    // Don't get permanently stuck if Wi-Fi fails.
    tft.setTextColor(TFT_RED);

    tft.drawString(
      "[FAIL] Wi-Fi connection",
      15,
      105
    );

    tft.setTextColor(TFT_LIGHTGREY);

    tft.drawString(
      "Continuing offline...",
      15,
      125
    );

    bootProgress(100);

    delay(1200);
  }
}


// ==================================================
// FINAL BOOT SCREEN
// ==================================================

void bootReady() {

  tft.fillScreen(TFT_BLACK);

  tft.setTextColor(TFT_GREEN);
  tft.setTextSize(3);

  tft.drawCentreString(
    "READY",
    160,
    35,
    2
  );

  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(1);

  tft.drawCentreString(
    "SPOTIFY CONTROLLER",
    160,
    80,
    1
  );

  tft.setTextColor(TFT_DARKGREY);

  tft.drawCentreString(
    "SYSTEM ONLINE",
    160,
    105,
    1
  );

  // Green status box
  tft.drawRoundRect(
    80,
    135,
    160,
    40,
    8,
    TFT_GREEN
  );

  tft.setTextColor(TFT_GREEN);

  tft.drawCentreString(
    "CONTROLS ENABLED",
    160,
    150,
    1
  );

  delay(1200);
}


// ==================================================
// JPEG CALLBACK
// ==================================================

bool tftOutput(
  int16_t x,
  int16_t y,
  uint16_t w,
  uint16_t h,
  uint16_t* bitmap
) {

  if (y >= tft.height()) {
    return false;
  }

  tft.pushImage(
    x,
    y,
    w,
    h,
    bitmap
  );

  return true;
}


// ==================================================
// SEND COMMAND
// ==================================================

void command(const char* path) {

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Command skipped: Wi-Fi offline");
    return;
  }

  HTTPClient http;

  http.setConnectTimeout(3000);
  http.setTimeout(5000);

  http.begin(
    serverBase() + path
  );

  int code = http.GET();

  Serial.print(path);
  Serial.print(" -> ");
  Serial.println(code);

  http.end();
}


// ==================================================
// GET SPOTIFY INFORMATION
// ==================================================

void getSpotifyInfo() {

  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  HTTPClient http;

  http.setConnectTimeout(3000);
  http.setTimeout(5000);

  if (!http.begin(serverBase() + "/now-playing")) {
    http.end();
    return;
  }

  int code = http.GET();

  if (code == 200) {

    String data =
      http.getString();

    int first =
      data.indexOf("|");

    int second =
      data.indexOf(
        "|",
        first + 1
      );

    if (
      first != -1 &&
      second != -1
    ) {

      String newSong =
        data.substring(
          0,
          first
        );

      String newArtist =
        data.substring(
          first + 1,
          second
        );

      // Legacy servers return "song|artist|cover"; the current server appends
      // "|1" or "|0" for the playback state. Accept both formats.
      int third = data.indexOf("|", second + 1);

      String newCover = (third == -1)
        ? data.substring(second + 1)
        : data.substring(second + 1, third);

      if (third != -1) {
        String state = data.substring(third + 1);
        state.trim();
        playing = (state == "1" || state == "true" ||
                   state == "playing" || state == "Playing");
      }

      if (newSong != song) {
        scrollX = 0;
      }

      song = newSong;
      artist = newArtist;

      // Keep the existing album cover when the server has no artwork, e.g.
      // while playback is paused or between tracks.
      if (newCover.length() > 0) {
        coverURL = newCover;
      }
    }
  }

  http.end();

  Serial.println(
    "SONG: " + song
  );

  Serial.println(
    "ARTIST: " + artist
  );
}


// ==================================================
// DOWNLOAD ALBUM COVER
// ==================================================

void downloadCover() {

  if (coverURL.length() == 0) {
    return;
  }

  Serial.println(
    "Downloading cover..."
  );

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;

  if (
    !http.begin(
      client,
      coverURL
    )
  ) {

    Serial.println(
      "Could not connect to cover"
    );

    return;
  }

  int code =
    http.GET();

  Serial.print(
    "Cover HTTP: "
  );

  Serial.println(code);

  if (code != 200) {

    http.end();

    return;
  }

  int len =
    http.getSize();

  Serial.print(
    "Cover size: "
  );

  Serial.println(len);

  if (
    len <= 0 ||
    len > 200000
  ) {

    Serial.println(
      "Invalid cover size"
    );

    http.end();

    return;
  }

  uint8_t* jpg =
    (uint8_t*)malloc(len);

  if (!jpg) {

    Serial.println(
      "Not enough RAM"
    );

    http.end();

    return;
  }

  WiFiClient* stream =
    http.getStreamPtr();

  int total = 0;

  while (total < len) {

    int available =
      stream->available();

    if (available > 0) {

      int toRead =
        min(
          available,
          len - total
        );

      int bytesRead =
        stream->read(
          jpg + total,
          toRead
        );

      if (bytesRead > 0) {
        total += bytesRead;
      }
    }

    delay(1);
  }

  http.end();

  Serial.print(
    "Downloaded: "
  );

  Serial.println(total);

  // Clear album-art area
  tft.fillRect(
    5,
    45,
    80,
    80,
    TFT_BLACK
  );

  // 640x640 -> 80x80
  TJpgDec.setJpgScale(8);

  TJpgDec.drawJpg(
    5,
    45,
    jpg,
    total
  );

  free(jpg);

  // Album border
  tft.drawRect(
    5,
    45,
    80,
    80,
    TFT_DARKGREY
  );

  Serial.println(
    "Cover drawn!"
  );
}


// ==================================================
// DRAW BUTTON
// ==================================================

void button(
  int x,
  int y,
  int w,
  int h,
  const char* text
) {

  tft.fillRoundRect(
    x,
    y,
    w,
    h,
    8,
    TFT_DARKGREY
  );

  tft.drawRoundRect(
    x,
    y,
    w,
    h,
    8,
    TFT_WHITE
  );

  tft.setTextColor(
    TFT_WHITE
  );

  tft.setTextSize(1);

  int textWidth =
    tft.textWidth(text);

  int textHeight =
    tft.fontHeight();

  tft.drawString(
    text,
    x + (w - textWidth) / 2,
    y + (h - textHeight) / 2
  );
}


// ==================================================
// DRAW PLAYBACK BUTTONS ONLY
// ==================================================

void drawPlaybackButtons() {
  button(10, 140, 90, 35, "|<");
  button(115, 140, 90, 35, playing ? "PAUSE" : "PLAY");
  button(220, 140, 90, 35, ">|");
}


// ==================================================
// DRAW MAIN SCREEN
// ==================================================

void drawScreen() {

  tft.fillScreen(
    TFT_BLACK
  );

  // Header
  tft.setTextColor(
    TFT_GREEN
  );

  tft.setTextSize(3);

  tft.drawString(
    "SPOTIFY",
    105,
    5
  );

  // Album art border
  tft.drawRect(
    5,
    45,
    80,
    80,
    TFT_DARKGREY
  );

  // Artist
  tft.setTextColor(
    TFT_LIGHTGREY
  );

  tft.setTextSize(1);

  tft.drawString(
    artist,
    100,
    95
  );

  // Playback buttons
  drawPlaybackButtons();

  // Volume buttons
  button(
    55,
    190,
    90,
    35,
    "VOL -"
  );

  button(
    175,
    190,
    90,
    35,
    "VOL +"
  );

  // Connection state, so a failed network setup is visible instead of a
  // screen that looks alive but cannot reach the server.
  tft.setTextSize(1);
  tft.setTextColor(
    WiFi.status() == WL_CONNECTED ? TFT_GREEN : TFT_RED
  );
  tft.drawString(
    WiFi.status() == WL_CONNECTED ? "WIFI ONLINE" : "WIFI OFFLINE",
    10,
    230
  );
}


// ==================================================
// DRAW SCROLLING SONG TITLE
// ==================================================

void drawScrollingText() {

  if (
    tft.textWidth(song) <= 190
  ) {

    tft.fillRect(
      95,
      45,
      220,
      35,
      TFT_BLACK
    );

    tft.setTextColor(
      TFT_WHITE
    );

    tft.setTextSize(2);

    tft.drawString(
      song,
      100,
      55
    );

    return;
  }

  if (
    millis() - lastScroll < 100
  ) {
    return;
  }

  lastScroll =
    millis();

  // Clip drawing ONLY to title area
  tft.setViewport(
    95,
    45,
    220,
    40
  );

  tft.fillRect(
    0,
    0,
    220,
    40,
    TFT_BLACK
  );

  tft.setTextColor(
    TFT_WHITE
  );

  tft.setTextSize(2);

  tft.drawString(
    song,
    5 - scrollX,
    10
  );

  tft.resetViewport();

  scrollX++;

  int textWidth =
    tft.textWidth(song);

  if (
    scrollX >
    textWidth + 40
  ) {

    scrollX = 0;
  }
}


// ==================================================
// SETUP
// ==================================================

void setup() {

  Serial.begin(
    115200
  );

  // -----------------------------------------------
  // DISPLAY
  // -----------------------------------------------

  tft.init();

  tft.setRotation(1);

  tft.fillScreen(
    TFT_BLACK
  );

  // -----------------------------------------------
  // JPEG DECODER
  // -----------------------------------------------

  TJpgDec.setCallback(
    tftOutput
  );

  TJpgDec.setSwapBytes(
    true
  );

  TJpgDec.setJpgScale(
    8
  );

  // -----------------------------------------------
  // TOUCHSCREEN
  // -----------------------------------------------

  touchSPI.begin(
    TOUCH_SCK,
    TOUCH_MISO,
    TOUCH_MOSI,
    TOUCH_CS
  );

  touch.begin(
    touchSPI
  );

  touch.setRotation(1);

  // -----------------------------------------------
  // BOOT ANIMATION
  // -----------------------------------------------

  bootScreen();

  // -----------------------------------------------
  // WIFI
  // -----------------------------------------------

  WiFi.setAutoReconnect(true);

  WiFi.begin(
    ssid,
    password
  );

  bootWiFi();

  // -----------------------------------------------
  // SPOTIFY
  // -----------------------------------------------

  getSpotifyInfo();

  // -----------------------------------------------
  // READY SCREEN
  // -----------------------------------------------

  bootReady();

  // -----------------------------------------------
  // MAIN UI
  // -----------------------------------------------

  drawScreen();

  // -----------------------------------------------
  // ALBUM ART
  // -----------------------------------------------

  downloadCover();

  lastCoverURL =
    coverURL;
}


// ==================================================
// LOOP
// ==================================================

void loop() {

  // Keep trying the saved network instead of requiring a reboot.
  static unsigned long lastWifiRetry = 0;
  if (WiFi.status() != WL_CONNECTED &&
      millis() - lastWifiRetry > 15000) {
    lastWifiRetry = millis();
    Serial.println("Wi-Fi reconnect attempt");
    WiFi.disconnect();
    WiFi.begin(ssid, password);
  }

  // -----------------------------------------------
  // TOUCH
  // -----------------------------------------------

  if (touch.touched()) {

    TS_Point p =
      touch.getPoint();

    int x =
      map(
        p.x,
        300,
        3800,
        0,
        320
      );

    int y =
      map(
        p.y,
        300,
        3800,
        0,
        240
      );

    x = constrain(
      x,
      0,
      319
    );

    y = constrain(
      y,
      0,
      239
    );

    // ---------------------------------------------
    // PLAYBACK BUTTONS
    // ---------------------------------------------

    if (
      y >= 140 &&
      y <= 175
    ) {

      if (x < 105) {

        command(
          "/previous"
        );

      } else if (x < 215) {

        command(
          "/play"
        );

        playing =
          !playing;

      } else {

        command(
          "/next"
        );
      }

      delay(700);

      String oldSong = song;
      String oldCover = coverURL;

      getSpotifyInfo();

      // Full redraw only when the track changed. A pause/resume must not
      // clear the album-art area (the historical disappearing-cover bug).
      if (song != oldSong || coverURL != oldCover) {
        drawScreen();
        downloadCover();
      } else {
        drawPlaybackButtons();
      }

      lastCoverURL = coverURL;

      while (
        touch.touched()
      ) {

        delay(10);
      }
    }

    // ---------------------------------------------
    // VOLUME BUTTONS
    // ---------------------------------------------

    else if (
      y >= 190 &&
      y <= 225
    ) {

      if (x < 160) {

        command(
          "/volume/down"
        );

      } else {

        command(
          "/volume/up"
        );
      }

      delay(300);

      while (
        touch.touched()
      ) {

        delay(10);
      }
    }
  }


  // -----------------------------------------------
  // SPOTIFY UPDATE
  // -----------------------------------------------

  static unsigned long lastUpdate = 0;

  if (
    millis() - lastUpdate > 3000
  ) {

    lastUpdate =
      millis();

    String oldSong =
      song;

    String oldCover =
      coverURL;

    getSpotifyInfo();

    if (song != oldSong) {
      drawScreen();
      downloadCover();
      lastCoverURL = coverURL;
    }
    else if (coverURL != oldCover) {
      downloadCover();
      lastCoverURL = coverURL;
    }
    else {
      // Refresh PLAY/PAUSE without touching the artwork.
      static bool lastPlaying = false;
      if (playing != lastPlaying) {
        lastPlaying = playing;
        drawPlaybackButtons();
      }
    }
  }


  // -----------------------------------------------
  // SCROLL TITLE
  // -----------------------------------------------

  drawScrollingText();
}