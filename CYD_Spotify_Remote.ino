#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <TJpg_Decoder.h>

TFT_eSPI tft = TFT_eSPI();

#define TOUCH_CS 33
#define TOUCH_IRQ 36
#define TOUCH_SCK 25
#define TOUCH_MISO 39
#define TOUCH_MOSI 32

SPIClass touchSPI(VSPI);
XPT2046_Touchscreen touch(TOUCH_CS, TOUCH_IRQ);

// Wi-Fi
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// PC Flask server
const char* server = "http://YOUR_COMPUTER_LAN_IP:5000";

// Spotify information
String song = "Loading...";
String artist = "";
String coverURL = "";
String lastCoverURL = "";

bool playing = false;

// Scrolling
int scrollX = 0;
unsigned long lastScroll = 0;

// --------------------------------------------------
// JPEG CALLBACK
// --------------------------------------------------

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

// --------------------------------------------------
// SEND COMMAND
// --------------------------------------------------

void command(const char* path) {

  HTTPClient http;

  http.begin(
    String(server) + path
  );

  int code = http.GET();

  Serial.print(path);
  Serial.print(" -> ");
  Serial.println(code);

  http.end();
}

// --------------------------------------------------
// GET SPOTIFY INFORMATION
// --------------------------------------------------

void getSpotifyInfo() {

  HTTPClient http;

  http.begin(
    String(server) + "/now-playing"
  );

  int code = http.GET();

  if (code == 200) {

    String data = http.getString();

    int first = data.indexOf("|");
    int second = data.indexOf("|", first + 1);

    if (first != -1 && second != -1) {

      String newSong =
        data.substring(0, first);

      String newArtist =
        data.substring(
          first + 1,
          second
        );

      String newCover =
        data.substring(
          second + 1
        );

      if (newSong != song) {
        scrollX = 0;
      }

      song = newSong;
      artist = newArtist;
      coverURL = newCover;
    }
  }

  http.end();

  Serial.println("SONG: " + song);
  Serial.println("ARTIST: " + artist);
}

// --------------------------------------------------
// DOWNLOAD ALBUM COVER
// --------------------------------------------------

void downloadCover() {

  if (coverURL.length() == 0) {
    return;
  }

  Serial.println("Downloading cover...");

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;

  if (!http.begin(client, coverURL)) {

    Serial.println(
      "Could not connect to cover"
    );

    return;
  }

  int code = http.GET();

  Serial.print("Cover HTTP: ");
  Serial.println(code);

  if (code != 200) {

    http.end();
    return;
  }

  int len = http.getSize();

  Serial.print("Cover size: ");
  Serial.println(len);

  if (len <= 0 || len > 200000) {

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

  Serial.print("Downloaded: ");
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

// --------------------------------------------------
// DRAW BUTTON
// --------------------------------------------------

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

// --------------------------------------------------
// DRAW MAIN SCREEN
// --------------------------------------------------

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
  button(
    10,
    140,
    90,
    35,
    "|<"
  );

  button(
    115,
    140,
    90,
    35,
    playing ? "PAUSE" : "PLAY"
  );

  button(
    220,
    140,
    90,
    35,
    ">|"
  );

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
}

// --------------------------------------------------
// DRAW SCROLLING SONG TITLE
// --------------------------------------------------

void drawScrollingText() {

  if (tft.textWidth(song) <= 190) {

    // Normal title
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

  // Stop drawing outside viewport
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

// --------------------------------------------------
// SETUP
// --------------------------------------------------

void setup() {

  Serial.begin(
    115200
  );

  // Display
  tft.init();

  tft.setRotation(1);

  tft.fillScreen(
    TFT_BLACK
  );

  // JPEG decoder
  TJpgDec.setCallback(
    tftOutput
  );

  TJpgDec.setSwapBytes(
    true
  );

  TJpgDec.setJpgScale(
    8
  );

  // Touchscreen
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

  // Wi-Fi
  WiFi.begin(
    ssid,
    password
  );

  Serial.print(
    "Connecting to WiFi"
  );

  while (
    WiFi.status() != WL_CONNECTED
  ) {

    delay(300);

    Serial.print(".");
  }

  Serial.println();

  Serial.println(
    "WIFI CONNECTED"
  );

  // Spotify
  getSpotifyInfo();

  // UI
  drawScreen();

  // Album art
  downloadCover();

  lastCoverURL =
    coverURL;
}

// --------------------------------------------------
// LOOP
// --------------------------------------------------

void loop() {

  // -----------------------------------------------
  // TOUCH
  // -----------------------------------------------

  if (touch.touched()) {

    TS_Point p =
      touch.getPoint();

    int x = map(
      p.x,
      300,
      3800,
      0,
      320
    );

    int y = map(
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

    // Playback buttons
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

      String oldCover =
        coverURL;

      getSpotifyInfo();

      drawScreen();

      if (
        coverURL != oldCover
      ) {

        downloadCover();

        lastCoverURL =
          coverURL;
      }

      while (
        touch.touched()
      ) {

        delay(10);
      }
    }

    // Volume buttons
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

    if (
      song != oldSong
    ) {

      drawScreen();
    }

    if (
      coverURL != oldCover
    ) {

      downloadCover();

      lastCoverURL =
        coverURL;
    }
  }

  // -----------------------------------------------
  // SCROLL TITLE
  // -----------------------------------------------

  drawScrollingText();
}

