#include <MHZ19.h>
MHZ19 mhz(&Serial1); // library can be found at: https://github.com/strange-v/MHZ19
#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "RunningAverage.h"
#include "Adafruit_FreeTouch.h"

#define pixelpin 8
#define serial_rx 7
#define serial_tx 6
#define silent_piezopin 3
#define piezopin 2
#define touchpin A0
MHZ19_RESULT response;
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32

#define nr_of_pixels 15
Adafruit_NeoPixel pixel(nr_of_pixels, pixelpin, NEO_GRB + NEO_KHZ800);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Adafruit_FreeTouch touch = Adafruit_FreeTouch(touchpin, OVERSAMPLE_16, RESISTOR_0, FREQ_MODE_NONE);
#define touchtreshold 800
boolean silenced_s = false;
boolean silenced_l = false;
int co2;

int max_value = 400;

#define alarm_treshold 900
#define big_alarm_treshold 1200

void setup() {
  pixel.begin();
  pixel.clear();
  pixel.setBrightness(100);
  if (! touch.begin()) {
    Serial.println("Failed to begin qt on pin A0");
  }
  Serial.begin(115200);
  Serial1.begin(9600);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setRotation(2);
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(3);
  display.setCursor(22, 7);
  display.print(F("Tjilp"));
  display.display();
  pixel.fill(pixel.Color(80, 80, 80)); pixel.show();
  chirp(HIGH);
  delay(3333);

  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(20, 10);
  display.print(F("Startup"));
  display.display();
  co2 = get_co2_measurement();
  Serial.print("startup:");
  Serial.println(co2);
  while (co2 == 410 || co2 == 0) {
    co2 = get_co2_measurement();
    pixel.fill(pixel.Color(80, 80, 80));
    pixel.show();
    Serial.print("startup:");
    Serial.println(co2);
    delay(1250);
  }
  chirp(LOW);
}

void check_max_value(int value) {
  if (value > max_value) {
    max_value = value;
  }
}

int get_co2_measurement() {
  response = mhz.retrieveData();
  while (response != MHZ19_RESULT_OK) {
    pixel.fill(pixel.Color(0, 0, 255));
    pixel.show();
    delay(100);
    response = mhz.retrieveData();
  }
  return mhz.getCO2();
}

void loud_mode() {
  pinMode(piezopin, OUTPUT);
  pinMode(silent_piezopin, INPUT);
}
void silent_mode() {
  pinMode(piezopin, INPUT);
  pinMode(silent_piezopin, OUTPUT);
}

void loop() {
  for (int t = 0; t < random(600, 1500); t++) {
    // We take a reading every 2 sec
    co2 = get_co2_measurement();
    check_max_value(co2);
    Serial.print(F("CO2: "));
    Serial.print(co2);

    display.clearDisplay();

    display.setTextSize(2);
    display.setCursor(27, 5);
    display.print(co2);
    display.setTextSize(1);
    display.print(" now");

    display.setTextSize(1);
    display.setCursor(40, 23);
    display.print(max_value);
    display.setTextSize(1);
    display.print(" max");

    display.display();

    int R, G;

    if (co2 > big_alarm_treshold) {
      if (touch.measure() > touchtreshold) {
        silenced_l = true;
        silenced_s = true;
      }
      //big alarm
      int R = map(co2, big_alarm_treshold, 1500, 255, 255);
      int G = map(co2, big_alarm_treshold, 1500, 130, 0);
      led(R, G);
      Serial.println(F(" That's horrible!"));
      if (!silenced_l) {
        loud_mode();
        for (int l = 0; l < 15; l++) {
          lowChirp(random(5, 10), 1);
          if (random(0, 5) == 0) {
            tweet(random(2, 12), 2);
          }
        }
      } else {
        delay(1000);
      }
    } else if (co2 > alarm_treshold) {
      silenced_l = false;
      if (touch.measure() > touchtreshold) {
        silenced_s = true;
      }
      //small alarm
      int R = map(co2, alarm_treshold, big_alarm_treshold, 255, 255);
      int G = map(co2, alarm_treshold, big_alarm_treshold, 255, 130);
      led(R, G);
      Serial.println(F(" That's not so good!"));
      if (!silenced_s) {
        silent_mode();
        for (int l = 0; l < 10; l++) {
          highChirp(random(5, 10), 1);
          if (random(0, 5) == 0) {
            tweet(random(2, 12), 2);
          }
        }
      } else {
        delay(1000);
      }
    } else {
      silenced_s = false;
      silenced_l = false;
      int R = map(co2, 450, alarm_treshold, 0, 255);
      int G = map(co2, 450, alarm_treshold, 255, 255);
      led(R, G);
      Serial.println(F(" That is ok"));
      delay(1000);
    }
    delay(1000);
    if (touch.measure() > touchtreshold) {
      int i = 0;
      for (i = 0; i <= 10; i++) {
        if (touch.measure() < touchtreshold) {
          break;
        }
        delay(1000);
      }
      if (i > 9) {
        max_value = 400;
        chirp(LOW);
      }
    }
  }
}

void led (int R, int G) {
  R = constrain(R, 0, 255);
  G = constrain(G, 0, 255);
  pixel.fill(pixel.gamma32(pixel.Color(R, G, 0)));
  pixel.show();
}

void chirp(boolean volume) {
  if (volume) {
    loud_mode();
  } else {
    silent_mode();
  }
  Serial.println("chirp");
  for (int i = 0; i < random(1, 3); i++ ) {
    highChirp(5, random(1, 5)); //intensity, amount of chirps
    delay(100);
    lowChirp(random(40, 200), 2); //intensity, amount of chirps
    delay(100);
    tweet(random(2, 12), 2); //intensity, amount of tweets
  }
}

void highChirp(int intensity, int chirpsNumber) {
  for (int veces = 0; veces <= chirpsNumber; veces++) {
    for (int i = 100; i > 0; i--) {
      for  (int x = 0; x < intensity;  x++) {
        digitalWrite (piezopin, HIGH);
        digitalWrite (silent_piezopin, HIGH);
        delayMicroseconds (i);
        digitalWrite (piezopin, LOW);
        digitalWrite (silent_piezopin, LOW);
        delayMicroseconds (i);
      }
    }
  }
}

void lowChirp(int intensity, int chirpsNumber) {
  for (int veces = 0; veces <= chirpsNumber; veces++) {
    for (int i = 0; i < 200; i++) {
      digitalWrite (piezopin, HIGH);
      digitalWrite (silent_piezopin, HIGH);
      delayMicroseconds (i);
      digitalWrite (piezopin, LOW);
      digitalWrite (silent_piezopin, LOW);
      delayMicroseconds (i);
    }
    for (int i = 90; i > 80; i--) {
      for  (int x = 0; x < 5;  x++) {
        digitalWrite (piezopin, HIGH);
        digitalWrite (silent_piezopin, HIGH);
        delayMicroseconds (i);
        digitalWrite (piezopin, LOW);
        digitalWrite (silent_piezopin, LOW);
        delayMicroseconds (i);
      }
    }
  }
}

void tweet(int intensity, int chirpsNumber) {
  //normal chirpsNumber 3, normal intensity 5
  for (int veces = 0; veces < chirpsNumber; veces++) {
    for (int i = 80; i > 0; i--) {
      for  (int x = 0; x < intensity;  x++) {
        digitalWrite (piezopin, HIGH);
        digitalWrite (silent_piezopin, HIGH);
        delayMicroseconds (i);
        digitalWrite (piezopin, LOW);
        digitalWrite (silent_piezopin, LOW);
        delayMicroseconds (i);
      }
    }
  }
  delay(1000);
}
