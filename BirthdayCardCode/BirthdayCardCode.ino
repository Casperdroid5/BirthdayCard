#define RGB_PIN 3     // Data pin for LED strip
#define BUZZER 10     // Buzzer pin
#define BUTTON1 5     // Button 1 pin (change color)
#define BUTTON2 4     // Button 2 pin (play song)
#define NUM_LEDS 18   // Total number of LEDs
#define DIGIT1_START 0 // Starting index for digit '1'
#define DIGIT1_LEDS 6  // LEDs for digit '1'
#define DIGIT2_START 6 // Starting index for digit '8'
#define DIGIT2_LEDS 12 // LEDs for digit '8'

// Speed control for song playback (0.5 = half speed, 1.0 = normal, 2.0 = double speed)
#define SONG_SPEED_FACTOR 1.0

// Include the FastLED library
#include <FastLED.h>

// Array to store LED data
CRGB leds[NUM_LEDS];

// Variables for button handling
bool button1State = HIGH;     // Current state of button 1
bool lastButton1State = HIGH; // Previous state of button 1
bool button2State = HIGH;     // Current state of button 2
bool lastButton2State = HIGH; // Previous state of button 2
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;

// Brightness control - now a software setting
#define DEFAULT_BRIGHTNESS 128  // Default brightness (medium)
uint8_t brightness = DEFAULT_BRIGHTNESS;

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

// Define the notes and their corresponding frequencies (in Hz)
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

// Define the melody for "Happy Birthday"
const int melody[] = {
  NOTE_G4, NOTE_G4, NOTE_A4, NOTE_G4, NOTE_C5, NOTE_B4,
  NOTE_G4, NOTE_G4, NOTE_A4, NOTE_G4, NOTE_D5, NOTE_C5,
  NOTE_G4, NOTE_G4, NOTE_G5, NOTE_E5, NOTE_C5, NOTE_B4, NOTE_A4,
  NOTE_F5, NOTE_F5, NOTE_E5, NOTE_C5, NOTE_D5, NOTE_C5
};

// Define the duration of each note (in fraction format)
// 1=whole note, 2=half note, 4=quarter note, 8=eighth note
const int noteDurationFractions[] = {
  8, 8, 4, 4, 4, 2,
  8, 8, 4, 4, 4, 2,
  8, 8, 4, 4, 4, 4, 4,
  8, 8, 4, 4, 4, 2
};

// Base duration in milliseconds for a quarter note
const int tempo = 250; // Adjust this to change overall song speed

// Get the number of notes in the melody
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

// Timing variables for non-blocking operation
unsigned long previousNoteTime = 0;
unsigned long noteEndTime = 0;
unsigned long noteDuration = 0;
unsigned long pauseDuration = 0;
bool noteIsPlaying = false;

// Color fading variables
uint8_t colorFadeHue = 0;
unsigned long lastColorFadeUpdate = 0;
const unsigned long colorFadeUpdateInterval = 20; // Update color fade every 20ms

void setup() {
  // Configure pins
  pinMode(BUZZER, OUTPUT);
  pinMode(BUTTON1, INPUT_PULLUP);
  pinMode(BUTTON2, INPUT_PULLUP);
  
  // Initialize LED strip
  FastLED.addLeds<WS2812B, RGB_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(brightness);
  
  // Set initial color but turn LEDs off
  updateLEDColor();
  turnOffAllLEDs();
  
  // Start serial for debugging
  Serial.begin(115200);
  Serial.println("18th BirthdayCard Setup Complete");
}

void loop() {
  // Read button states with debounce
  checkButtons();
  
  // Handle song playing
  updateSong();
  
  // Handle color fading if active
  if (songState == COLOR_FADING) {
    updateColorFade();
  }
  
  // Update LEDs if needed
  FastLED.show();
}

void checkButtons() {
  // Read current button states
  bool reading1 = digitalRead(BUTTON1);
  bool reading2 = digitalRead(BUTTON2);
  
  // Check if any button state has changed
  if (reading1 != lastButton1State || reading2 != lastButton2State) {
    lastDebounceTime = millis();
  }
  
  // Check if enough time has passed since last button state change
  if ((millis() - lastDebounceTime) > debounceDelay) {
    // Button 1 - Change color
    if (reading1 != button1State) {
      button1State = reading1;
      if (button1State == LOW) {
        // Button pressed - change color
        currentColorIndex = (currentColorIndex + 1) % NUM_COLORS;
        
        // If we're in color fading mode, exit it
        if (songState == COLOR_FADING) {
          songState = IDLE;
        }
        
        updateLEDColor();
        Serial.print("Color changed to index: ");
        Serial.println(currentColorIndex);
      }
    }
    
    // Button 2 - Play birthday song
    if (reading2 != button2State) {
      button2State = reading2;
      if (button2State == LOW) {
        if (songState == COLOR_FADING) {
          // If in fading mode, exit to IDLE state
          songState = IDLE;
          updateLEDColor();
        } else if (songState == IDLE) {
          // Only start song if we're in IDLE state
          Serial.println("Starting birthday song");
          startBirthdaySong();
        }
      }
    }
  }
  
  // Save button states for next loop
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
  // Set visible LEDs to the current color
  for (int i = 0; i < NUM_LEDS; i++) {
    if (leds[i] != CRGB::Black) { // Only update LEDs that are on
      leds[i] = colorOptions[currentColorIndex];
    }
  }
  FastLED.show();
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
      // Nothing to do when idle
      break;
      
    case PLAYING_BIRTHDAY:
      if (!noteIsPlaying && currentTime >= previousNoteTime) {
        // Time to start a new note
        if (currentNote < melodyLength) {
          // Calculate note timing with fraction format and speed factor
          // tempo = ms for a quarter note (4)
          // Duration = tempo * 4 / fraction value
          noteDuration = (tempo * 4 / noteDurationFractions[currentNote]) / SONG_SPEED_FACTOR;
          pauseDuration = noteDuration * 0.3; // 30% gap between notes
          
          // Play note
          tone(BUZZER, melody[currentNote], noteDuration);
          
          // Light up next LED in sequence - calculate how many LEDs should turn on per note
          float ledsPerNote = (float)(NUM_LEDS) / melodyLength;
          int targetLEDs = round((currentNote + 1) * ledsPerNote);
          
          // Turn on new LEDs
          while (ledsLit < targetLEDs && ledsLit < NUM_LEDS) {
            // First light up the '1' digit (first 6 LEDs)
            if (ledsLit < DIGIT1_LEDS) {
              leds[DIGIT1_START + ledsLit] = colorOptions[currentColorIndex];
            } 
            // Then light up the '8' digit (next 12 LEDs)
            else if (ledsLit < (DIGIT1_LEDS + DIGIT2_LEDS)) {
              leds[DIGIT2_START + (ledsLit - DIGIT1_LEDS)] = colorOptions[currentColorIndex];
            }
            ledsLit++;
          }
          FastLED.show();
          
          noteEndTime = currentTime + noteDuration;
          noteIsPlaying = true;
        } else {
          // Make sure all LEDs are on before hieper de piep
          for (int i = 0; i < NUM_LEDS; i++) {
            leds[i] = colorOptions[currentColorIndex];
          }
          FastLED.show();
          
          // Birthday song ended, start hieper de piep
          songState = PLAYING_HIEPER;
          currentHieperNote = 0;
          noteIsPlaying = false;
          // Add a short pause before starting hieper de piep
          previousNoteTime = currentTime + 500 / SONG_SPEED_FACTOR;
        }
      } else if (noteIsPlaying && currentTime >= noteEndTime) {
        // Current note ended, prepare for next note
        noteIsPlaying = false;
        currentNote++;
        previousNoteTime = currentTime + pauseDuration;
      }
      break;
      
    case PLAYING_HIEPER:
      if (!noteIsPlaying && currentTime >= previousNoteTime) {
        if (currentHieperNote < hieperLength) {
          // Play hieper de piep sequence
          int hieperType = hieperSequence[currentHieperNote];
          
          if (hieperType == 1) {
            // Short beep for "hiep"
            tone(BUZZER, NOTE_C5, 150 / SONG_SPEED_FACTOR); // Using C5 for hiep
            noteDuration = 150 / SONG_SPEED_FACTOR;
            
            // Flash all LEDs white
            for (int j = 0; j < NUM_LEDS; j++) {
              leds[j] = CRGB::White;
            }
            FastLED.setBrightness(255); // Full brightness for effect
            FastLED.show();
          } else {
            // Long beep for "hoera"
            tone(BUZZER, NOTE_C5, 500 / SONG_SPEED_FACTOR); // Using C5 for hoera
            noteDuration = 500 / SONG_SPEED_FACTOR;
            
            // Make rainbow effect
            for (int j = 0; j < NUM_LEDS; j++) {
              leds[j] = CHSV(j * 255 / NUM_LEDS, 255, 255);
            }
            FastLED.setBrightness(255); // Full brightness for effect
            FastLED.show();
          }
          
          noteEndTime = currentTime + noteDuration;
          noteIsPlaying = true;
        } else {
          // End of hieper de piep sequence, move to color fading state
          songState = COLOR_FADING;
          colorFadeHue = 0;
          lastColorFadeUpdate = currentTime;
          Serial.println("Song finished, starting color fade");
        }
      } else if (noteIsPlaying && currentTime >= noteEndTime) {
        // Current hieper note ended
        noteIsPlaying = false;
        currentHieperNote++;
        
        // Set pause between hieper notes
        if (currentHieperNote < hieperLength && hieperSequence[currentHieperNote-1] == 1) {
          // After short beep, shorter pause
          previousNoteTime = currentTime + (100 / SONG_SPEED_FACTOR);
          // Dim LEDs between short beeps
          FastLED.setBrightness(50);
          FastLED.show();
        } else {
          // After long beep, longer pause
          previousNoteTime = currentTime + (200 / SONG_SPEED_FACTOR);
        }
      }
      break;
      
    case COLOR_FADING:
      // Colors are handled in updateColorFade()
      // This state continues until a button is pressed
      break;
      
    case ENDING:
      // Reset song state and LED color
      songState = IDLE;
      FastLED.setBrightness(brightness); // Restore user-set brightness
      updateLEDColor();
      break;
  }
}

void updateColorFade() {
  unsigned long currentTime = millis();
  
  // Update color fade at the specified interval
  if (currentTime - lastColorFadeUpdate >= colorFadeUpdateInterval) {
    lastColorFadeUpdate = currentTime;
    
    // Increment hue for color cycling
    colorFadeHue++;
    
    // Apply a smooth color fade across all LEDs
    for (int i = 0; i < NUM_LEDS; i++) {
      // Create a gradient effect by offsetting hue based on LED position
      leds[i] = CHSV(colorFadeHue + (i * 255 / NUM_LEDS), 255, 255);
    }
    
    // Apply a pulsing brightness effect
    int pulsingBrightness = 128 + (127 * sin(millis() / 500.0));
    FastLED.setBrightness(pulsingBrightness);
  }
}