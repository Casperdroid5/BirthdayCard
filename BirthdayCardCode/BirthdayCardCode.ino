#include <FastLED.h>

#define RGB_PIN 3     // Data pin for LED strip
#define BUZZER 10     // Buzzer pin
#define BUTTON1 5     // Button 1 pin (change color)
#define BUTTON2 4     // Button 2 pin (play song)
#define NUM_LEDS 18   // Total number of LEDs

// LED MAPPING CONFIGURATION
const uint8_t digit1Mapping[6] = {0, 1, 2, 3, 4, 5}; // Digit '1' mapping (6 LEDs)
const uint8_t digit2Mapping[12] = {8,9,11,10,6,7,12,13,14,15,16,17}; // Digit '8' mapping

// Speed control for song playback
#define SONG_SPEED_FACTOR 1.0

// Array to store LED data
CRGB leds[NUM_LEDS];

// Variables for button handling
bool button1State = HIGH;
bool lastButton1State = HIGH;
bool button2State = HIGH;
bool lastButton2State = HIGH;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;

// Brightness control
#define BRIGHTNESS 90
uint8_t brightness = BRIGHTNESS;

// Color switching variables
int currentColorIndex = 0;
CRGB colorOptions[] = {
  CRGB::Red,
  CRGB::Green,
  CRGB::Blue,
  CRGB::Purple,
  CRGB::Yellow,
  CRGB::Cyan,
  CRGB::White
};
#define NUM_COLORS (sizeof(colorOptions) / sizeof(colorOptions[0]))

// Musical notes definitions
const int NOTE_C4 = 262;
const int NOTE_D4 = 294;
const int NOTE_E4 = 330;
const int NOTE_F4 = 349;
const int NOTE_G4 = 392;
const int NOTE_A4 = 440;
const int NOTE_B4 = 494;
const int NOTE_C5 = 523;
const int NOTE_D5 = 587;
const int NOTE_E5 = 659;
const int NOTE_F5 = 698;
const int NOTE_G5 = 784;

// Happy Birthday melody
const int melody[] = {
  NOTE_G4, NOTE_G4, NOTE_A4, NOTE_G4, NOTE_C5, NOTE_B4,
  NOTE_G4, NOTE_G4, NOTE_A4, NOTE_G4, NOTE_D5, NOTE_C5,
  NOTE_G4, NOTE_G4, NOTE_G5, NOTE_E5, NOTE_C5, NOTE_B4, NOTE_A4,
  NOTE_F5, NOTE_F5, NOTE_E5, NOTE_C5, NOTE_D5, NOTE_C5
};

const int noteDurationFractions[] = {
  8, 8, 4, 4, 4, 2,
  8, 8, 4, 4, 4, 2,
  8, 8, 4, 4, 4, 4, 4,
  8, 8, 4, 4, 4, 2
};

const int tempo = 250;
const int melodyLength = sizeof(melody) / sizeof(melody[0]);
int currentNote = 0;

// LED animation variables
int ledsLit = 0;

// Hieper de piep pattern
const int hieperSequence[] = {1, 1, 1, 2}; // 1=short, 2=long
const int hieperLength = 4;
int currentHieperNote = 0;

// Song state machine
enum SongState {
  IDLE,
  PLAYING_BIRTHDAY,
  PLAYING_HIEPER,
  COLOR_FADING,
  ENDING
};
SongState songState = IDLE;

// Timing variables
unsigned long previousNoteTime = 0;
unsigned long noteEndTime = 0;
unsigned long noteDuration = 0;
unsigned long pauseDuration = 0;
bool noteIsPlaying = false;

// Color fading variables
uint8_t colorFadeHue = 0;
unsigned long lastColorFadeUpdate = 0;
const unsigned long colorFadeUpdateInterval = 20;

void setup() {
  pinMode(BUZZER, OUTPUT);
  pinMode(BUTTON1, INPUT_PULLUP);
  pinMode(BUTTON2, INPUT_PULLUP);
  
  FastLED.addLeds<WS2812B, RGB_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(brightness);
  
  updateLEDColor();
  turnOffAllLEDs();
  
  Serial.begin(115200);
  Serial.println("18th BirthdayCard Setup Complete");
}

void loop() {
  checkButtons();
  updateSong();
  
  if (songState == COLOR_FADING) {
    updateColorFade();
  }
  
  FastLED.show();
}

void checkButtons() {
  bool reading1 = digitalRead(BUTTON1);
  bool reading2 = digitalRead(BUTTON2);
  
  if (reading1 != lastButton1State || reading2 != lastButton2State) {
    lastDebounceTime = millis();
  }
  
  if ((millis() - lastDebounceTime) > debounceDelay) {
    // Button 1 - Change color
    if (reading1 != button1State) {
      button1State = reading1;
      if (button1State == LOW) {
        currentColorIndex = (currentColorIndex + 1) % NUM_COLORS;
        updateLEDColor();
        Serial.print("Color changed to index: ");
        Serial.println(currentColorIndex);
      }
    }
    
    // Button 2 - Play birthday song
    if (reading2 != button2State) {
      button2State = reading2;
      if (button2State == LOW) {
        if (songState == IDLE) {
          Serial.println("Starting birthday song");
          startBirthdaySong();
        } else {
          songState = IDLE;
          noTone(BUZZER);
          updateLEDColor();
        }
      }
    }
  }
  
  lastButton1State = reading1;
  lastButton2State = reading2;
}

void turnOffAllLEDs() {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB::Black;
  }
  FastLED.show();
}

void updateLEDColor() {
  if (songState == COLOR_FADING) {
    songState = IDLE;
    turnOffAllLEDs();
  }
  
  turnOnDigit1(colorOptions[currentColorIndex]);
  turnOnDigit2(colorOptions[currentColorIndex]);
  FastLED.show();
}

void turnOnDigit1(CRGB color) {
  for (int i = 0; i < 6; i++) {
    leds[digit1Mapping[i]] = color;
  }
}

void turnOnDigit2(CRGB color) {
  for (int i = 0; i < 12; i++) {
    leds[digit2Mapping[i]] = color;
  }
}

void startBirthdaySong() {
  currentNote = 0;
  ledsLit = 0;
  songState = PLAYING_BIRTHDAY;
  previousNoteTime = millis();
  noteIsPlaying = false;
  turnOffAllLEDs();
}

void updateSong() {
  unsigned long currentTime = millis();
  
  switch (songState) {
    case IDLE:
      break;
      
    case PLAYING_BIRTHDAY:
      if (!noteIsPlaying && currentTime >= previousNoteTime) {
        if (currentNote < melodyLength) {
          noteDuration = (tempo * 4 / noteDurationFractions[currentNote]) / SONG_SPEED_FACTOR;
          pauseDuration = noteDuration * 0.3;
          
          tone(BUZZER, melody[currentNote], noteDuration);
          
          float ledsPerNote = (float)(NUM_LEDS) / melodyLength;
          int targetLEDs = round((currentNote + 1) * ledsPerNote);
          
          while (ledsLit < targetLEDs && ledsLit < NUM_LEDS) {
            if (ledsLit < 6) {
              leds[digit1Mapping[ledsLit]] = colorOptions[currentColorIndex];
            } else if (ledsLit < 18) {
              leds[digit2Mapping[ledsLit - 6]] = colorOptions[currentColorIndex];
            }
            ledsLit++;
          }
          FastLED.show();
          
          noteEndTime = currentTime + noteDuration;
          noteIsPlaying = true;
        } else {
          for (int i = 0; i < NUM_LEDS; i++) {
            leds[i] = colorOptions[currentColorIndex];
          }
          FastLED.show();
          
          songState = PLAYING_HIEPER;
          currentHieperNote = 0;
          noteIsPlaying = false;
          previousNoteTime = currentTime + 500 / SONG_SPEED_FACTOR;
        }
      } else if (noteIsPlaying && currentTime >= noteEndTime) {
        noteIsPlaying = false;
        currentNote++;
        previousNoteTime = currentTime + pauseDuration;
      }
      break;
      
    case PLAYING_HIEPER:
      if (!noteIsPlaying && currentTime >= previousNoteTime) {
        if (currentHieperNote < hieperLength) {
          int hieperType = hieperSequence[currentHieperNote];
          
          if (hieperType == 1) {
            tone(BUZZER, NOTE_C5, 150 / SONG_SPEED_FACTOR);
            noteDuration = 150 / SONG_SPEED_FACTOR;
            
            for (int j = 0; j < NUM_LEDS; j++) {
              leds[j] = CRGB::White;
            }
            FastLED.setBrightness(BRIGHTNESS);
            FastLED.show();
          } else {
            tone(BUZZER, NOTE_C5, 500 / SONG_SPEED_FACTOR);
            noteDuration = 500 / SONG_SPEED_FACTOR;
            
            for (int j = 0; j < NUM_LEDS; j++) {
              leds[j] = CHSV(j * 255 / NUM_LEDS, 255, 255);
            }
            FastLED.setBrightness(BRIGHTNESS);
            FastLED.show();
          }
          
          noteEndTime = currentTime + noteDuration;
          noteIsPlaying = true;
        } else {
          songState = COLOR_FADING;
          colorFadeHue = 0;
          lastColorFadeUpdate = currentTime;
          Serial.println("Song finished, starting color fade");
        }
      } else if (noteIsPlaying && currentTime >= noteEndTime) {
        noteIsPlaying = false;
        currentHieperNote++;
        
        if (currentHieperNote < hieperLength && hieperSequence[currentHieperNote-1] == 1) {
          previousNoteTime = currentTime + (100 / SONG_SPEED_FACTOR);
          FastLED.setBrightness(50);
          FastLED.show();
        } else {
          previousNoteTime = currentTime + (200 / SONG_SPEED_FACTOR);
        }
      }
      break;
      
    case COLOR_FADING:
      break;
      
    case ENDING:
      songState = IDLE;
      FastLED.setBrightness(brightness);
      updateLEDColor();
      break;
  }
}

void updateColorFade() {
  unsigned long currentTime = millis();
  
  if (currentTime - lastColorFadeUpdate >= colorFadeUpdateInterval) {
    lastColorFadeUpdate = currentTime;
    colorFadeHue++;
    FastLED.setBrightness(BRIGHTNESS);
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i] = CHSV(colorFadeHue + (i * 255 / NUM_LEDS), 255, 255);
    }
    
  }
}
