#include <FastLED.h>
#include <WiFi.h>
#include <WebServer.h>

// Hardware Configuration
#define RGB_PIN 3      // Data pin for LED strip
#define BUZZER 10      // Buzzer pin
#define BUTTON1 5      // Button 1 pin (change color)
#define BUTTON2 4      // Button 2 pin (play song)
#define NUM_LEDS 18    // Total number of LEDs
#define BRIGHTNESS 50  // Constant brightness level

// WiFi Configuration
const char* ssid = "HappyBirthdayBroertje!";
WebServer server(80);
// Go to http://192.168.4.1/ for the website(server)

// LED Mapping
const uint8_t digit1Mapping[6] = { 0, 1, 2, 3, 4, 5 };                             // Digit '1' mapping
const uint8_t digit2Mapping[12] = { 8, 9, 11, 10, 6, 7, 12, 13, 14, 15, 16, 17 };  // Digit '8' mapping

// LED Array
CRGB leds[NUM_LEDS];

// Button Handling
bool button1State = HIGH;
bool lastButton1State = HIGH;
bool button2State = HIGH;
bool lastButton2State = HIGH;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;

// Display Mode Enum - Adding new patterns
enum DisplayMode {
  STATIC_COLOR,
  RAINBOW_MODE,
  SNAKE_MODE,
  RANDOM_BLINK,
  CHASE_MODE,
  BREATHE_MODE,
  WAVE_MODE,
  OFF_MODE
};

// Current mode
DisplayMode currentMode = STATIC_COLOR;

// Color Options (Used for STATIC_COLOR mode)
int currentColorIndex = 0;
CRGB colorOptions[] = {
  CRGB::Red,
  CRGB::Green,
  CRGB::Blue,
  CRGB::Purple,
  CRGB::Yellow,
  CRGB::Cyan,
  CRGB::White,
  CRGB::Black
};
#define NUM_COLORS (sizeof(colorOptions) / sizeof(colorOptions[0]))

// Pattern Variables
unsigned long lastPatternUpdate = 0;
unsigned long patternUpdateInterval = 50;  // Default update interval for patterns

// Pattern-specific speed controls
const unsigned long RAINBOW_SPEED = 100;  // Slower rainbow cycle
const unsigned long SNAKE_SPEED = 150;    // Slower snake movement
const unsigned long CHASE_SPEED = 120;    // Slightly slower chase pattern
const unsigned long WAVE_SPEED = 80;      // Medium speed for wave
const unsigned long BREATHE_SPEED = 30;   // Keep breathe relatively smooth

// Snake Pattern Variables
uint8_t snakeHeadPos = 0;
const uint8_t snakeLength = 6;
uint8_t snakeHue = 0;

// Random Blink Variables
uint8_t randomLEDs[5] = {0};  // Tracks which LEDs are currently active
uint8_t randomHues[5] = {0};  // Hues for random LEDs
unsigned long randomBlinkInterval = 500;  // Slower random blink (was 200)
unsigned long lastRandomUpdate = 0;

// Chase Pattern Variables
uint8_t chasePos = 0;
uint8_t chaseHue = 0;

// Breathe Pattern Variables
uint8_t breatheBrightness = 0;
bool breatheIncreasing = true;

// Wave Pattern Variables
uint8_t waveOffset = 0;

// Music Notes
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

// Happy Birthday Melody
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

// Song State Machine
enum SongState {
  IDLE,
  PLAYING_BIRTHDAY,
  PLAYING_HIEPER,
  COLOR_FADING,
  CONFETTI_MODE,
  ENDING
};
SongState songState = IDLE;

// Timing Variables
unsigned long previousNoteTime = 0;
unsigned long noteEndTime = 0;
unsigned long noteDuration = 0;
unsigned long pauseDuration = 0;
bool noteIsPlaying = false;

// Animation Variables
int ledsLit = 0;
const int hieperSequence[] = { 1, 1, 1, 1, 3, 1, 2 };  // 0=very short, 1=short, 2=long, 3=pause
const int hieperLength = 7;
int currentHieperNote = 0;

// Color Fading
uint8_t colorFadeHue = 0;
unsigned long lastColorFadeUpdate = 0;
const unsigned long colorFadeUpdateInterval = 20;

// Confetti Mode
#define CONFETTI_DURATION 86400000 // 24 hours  
#define CONFETTI_SPAWN_RATE 13
#define CONFETTI_FADE_RATE 1
unsigned long confettiStartTime = 0;
uint8_t confettiHue = 1;

// Button Combination Detection
bool bothButtonsPressed = false;
unsigned long bothButtonsStartTime = 0;
#define BOTH_BUTTONS_HOLD_TIME 2000  // 2 second

void setup() {
  // Initialize hardware
  pinMode(BUZZER, OUTPUT);
  pinMode(BUTTON1, INPUT_PULLUP);
  pinMode(BUTTON2, INPUT_PULLUP);

  // Initialize LEDs
  FastLED.addLeds<WS2812B, RGB_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);

  // Initial state
  updateDisplay();

  // Start serial
  Serial.begin(115200);
  Serial.println("18th BirthdayCard Ready!");
}

void loop() {
  checkButtons();
  checkBothButtons();
  updateSong();
  
  // Update pattern animations
  updatePatterns();

  if (songState == COLOR_FADING) {
    updateColorFade();
  } else if (songState == CONFETTI_MODE) {
    updateConfettiMode();
    server.handleClient();  // Handle web requests
  }

  FastLED.show();
}

void checkButtons() {
  bool reading1 = digitalRead(BUTTON1);
  bool reading2 = digitalRead(BUTTON2);

  // Debounce logic
  if (reading1 != lastButton1State || reading2 != lastButton2State) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    // Button 1 - Change mode
    if (reading1 != button1State) {
      button1State = reading1;
      if (button1State == LOW) {
        // If we're in a static color mode, increment the color
        if (currentMode == STATIC_COLOR) {
          currentColorIndex = (currentColorIndex + 1) % NUM_COLORS;
          if (currentColorIndex == NUM_COLORS - 1) {  // After going through all colors
            // Switch to first pattern mode
            currentMode = RAINBOW_MODE;
            Serial.println("Mode changed to RAINBOW");
          } else {
            Serial.print("Color changed to index: ");
            Serial.println(currentColorIndex);
          }
        } else {
          // If we're in a pattern mode, switch to the next pattern
          currentMode = (DisplayMode)((int)currentMode + 1);
          if (currentMode > WAVE_MODE) {  // After the last pattern
            currentMode = STATIC_COLOR;   // Reset to static color mode
            currentColorIndex = 0;        // Reset to first color
            Serial.println("Mode reset to STATIC_COLOR");
          } else {
            Serial.print("Mode changed to: ");
            Serial.println(currentMode);
          }
        }
        updateDisplay();  // Update display based on new mode
      }
    }

    // Button 2 - Play song
    if (reading2 != button2State) {
      button2State = reading2;
      if (button2State == LOW) {
        if (songState == IDLE) {
          startBirthdaySong();
        } else {
          stopSong();
        }
      }
    }
  }

  lastButton1State = reading1;
  lastButton2State = reading2;
}

void checkBothButtons() {
  bool btn1 = digitalRead(BUTTON1) == LOW;
  bool btn2 = digitalRead(BUTTON2) == LOW;

  if (btn1 && btn2) {
    if (!bothButtonsPressed) {
      bothButtonsPressed = true;
      bothButtonsStartTime = millis();
    } else if (millis() - bothButtonsStartTime >= BOTH_BUTTONS_HOLD_TIME) {
      if (songState != CONFETTI_MODE) {
        startConfettiMode();
      }
    }
  } else {
    bothButtonsPressed = false;
  }
}

void updatePatterns() {
  unsigned long currentTime = millis();
  
  // Only update patterns if we're in IDLE state (not playing music)
  if (songState != IDLE) return;
  
  // Set the appropriate update interval based on current mode
  switch (currentMode) {
    case RAINBOW_MODE:
      patternUpdateInterval = RAINBOW_SPEED;
      break;
    case SNAKE_MODE:
      patternUpdateInterval = SNAKE_SPEED;
      break;
    case CHASE_MODE:
      patternUpdateInterval = CHASE_SPEED;
      break;
    case WAVE_MODE:
      patternUpdateInterval = WAVE_SPEED;
      break;
    case BREATHE_MODE:
      patternUpdateInterval = BREATHE_SPEED;
      break;
    case RANDOM_BLINK:
      // Random blink uses its own timing mechanism
      break;
    default:
      patternUpdateInterval = 50; // Default for other modes
      break;
  }
  
  // Update based on current pattern mode
  if (currentTime - lastPatternUpdate >= patternUpdateInterval) {
    lastPatternUpdate = currentTime;
    
    switch (currentMode) {
      case STATIC_COLOR:
        // Static color is set by updateDisplay(), no animation updates needed
        break;
        
      case RAINBOW_MODE:
        updateRainbowPattern();
        break;
        
      case SNAKE_MODE:
        updateSnakePattern();
        break;
        
      case RANDOM_BLINK:
        updateRandomBlinkPattern(currentTime);
        break;
        
      case CHASE_MODE:
        updateChasePattern();
        break;
        
      case BREATHE_MODE:
        updateBreathePattern();
        break;
        
      case WAVE_MODE:
        updateWavePattern();
        break;
        
      case OFF_MODE:
        turnOffAllLEDs();
        break;
    }
  }
}

void updateRainbowPattern() {
  // Simple rainbow pattern that cycles through the color wheel
  // Slowed down color transition
  colorFadeHue++;  // Increment by 1 for a slower color change
  
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CHSV(colorFadeHue + (i * 255 / NUM_LEDS), 255, 255);
  }
}

void updateSnakePattern() {
  // Snake pattern - a moving segment of LEDs
  turnOffAllLEDs();  // Clear all LEDs
  
  // Update snake head position
  snakeHeadPos = (snakeHeadPos + 1) % NUM_LEDS;
  snakeHue += 1;  // Slower color change for snake (was 2)
  
  // Draw the snake body
  for (int i = 0; i < snakeLength; i++) {
    int pos = (snakeHeadPos - i + NUM_LEDS) % NUM_LEDS;  // Ensure positive wrap-around
    // Fade the brightness based on distance from head
    int brightness = 255 - (i * 255 / snakeLength);
    leds[pos] = CHSV(snakeHue, 255, brightness);
  }
}

void updateRandomBlinkPattern(unsigned long currentTime) {
  // Random LEDs turn on and off
  if (currentTime - lastRandomUpdate >= randomBlinkInterval) {
    lastRandomUpdate = currentTime;
    
    // Turn off old random LEDs
    for (int i = 0; i < 5; i++) {
      if (randomLEDs[i] < NUM_LEDS) {
        leds[randomLEDs[i]] = CRGB::Black;
      }
    }
    
    // Generate new random LEDs
    for (int i = 0; i < 5; i++) {
      randomLEDs[i] = random8(NUM_LEDS);
      randomHues[i] = random8();
      leds[randomLEDs[i]] = CHSV(randomHues[i], 255, 255);
    }
  }
}

void updateChasePattern() {
  // A chase pattern that runs around the digits
  turnOffAllLEDs();
  
  // First handle digit 1 (the "1")
  if (chasePos < 6) {
    leds[digit1Mapping[chasePos]] = CHSV(chaseHue, 255, 255);
  } 
  // Then handle digit 2 (the "8")
  else if (chasePos < 18) {
    leds[digit2Mapping[chasePos - 6]] = CHSV(chaseHue, 255, 255);
  }
  
  // Update position and hue
  chasePos = (chasePos + 1) % 18;
  chaseHue += 5;
}

void updateBreathePattern() {
  // Breathing effect - all LEDs fade in and out together
  if (breatheIncreasing) {
    breatheBrightness += 3;  // Slower increase (was 5)
    if (breatheBrightness >= 250) {
      breatheIncreasing = false;
    }
  } else {
    breatheBrightness -= 3;  // Slower decrease (was 5)
    if (breatheBrightness <= 5) {
      breatheIncreasing = true;
      colorFadeHue += 5;  // Change color slightly with each breath
    }
  }
  
  fill_solid(leds, NUM_LEDS, CHSV(colorFadeHue, 255, breatheBrightness));
}

void updateWavePattern() {
  // Wave pattern - a sine wave of color moving through the digits
  waveOffset += 5;
  
  for (int i = 0; i < 6; i++) {
    // Creating a wave effect through the first digit
    uint8_t sinBrightness = sin8(waveOffset + (i * 255 / 6));
    leds[digit1Mapping[i]] = CHSV(colorFadeHue, 255, sinBrightness);
  }
  
  for (int i = 0; i < 12; i++) {
    // Creating a wave effect through the second digit
    uint8_t sinBrightness = sin8(waveOffset + ((i + 6) * 255 / 12));
    leds[digit2Mapping[i]] = CHSV(colorFadeHue + 30, 255, sinBrightness); // Offset hue for contrast
  }
  
  colorFadeHue++;  // Slowly change the base color
}

void startWebServer() {
  WiFi.softAP(ssid);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);

  server.on("/", handleRoot);
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("Web server started");
}

void handleRoot() {
  String html = R"=====(
<!DOCTYPE html>
<html>
<head>
  <title>Happy Birthday Secret Message</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body {
      font-family: 'Courier New', monospace;
      max-width: 800px;
      margin: 0 auto;
      padding: 20px;
      background-color: #0a1a2a;
      background-image: 
        linear-gradient(rgba(0, 150, 255, 0.1) 1px, transparent 1px),
        linear-gradient(90deg, rgba(0, 150, 255, 0.1) 1px, transparent 1px),
        linear-gradient(rgba(0, 150, 255, 0.05) 1px, transparent 1px),
        linear-gradient(90deg, rgba(0, 150, 255, 0.05) 1px, transparent 1px);
      background-size: 
        50px 50px,
        50px 50px,
        10px 10px,
        10px 10px;
      color: #00ffaa;
      text-align: center;
      position: relative;
      overflow-x: hidden;
      min-height: 100vh;
      box-sizing: border-box;
    }
    h1 {
      color: #00ffaa;
      font-size: clamp(1.5em, 5vw, 2em);
      text-shadow: 0 0 5px #00ffaa;
      border-bottom: 1px solid #00ffaa;
      padding-bottom: 10px;
      margin-top: 20px;
    }
    .message-container {
      max-height: 70vh;
      overflow-y: auto;
      padding: 10px;
    }
    .message {
      background-color: rgba(10, 26, 42, 0.8);
      padding: 15px;
      border-radius: 5px;
      border: 1px solid #00ffaa;
      box-shadow: 0 0 15px rgba(0, 255, 170, 0.3);
      text-align: left;
      margin: 20px auto;
      position: relative;
      z-index: 10;
      max-width: 95%;
      font-size: clamp(14px, 3vw, 16px);
      line-height: 1.5;
    }
    .signature {
      font-style: italic;
      text-align: right;
      margin-top: 20px;
      color: #00aaff;
    }
    .confetti {
      position: fixed;
      width: 8px;
      height: 8px;
      opacity: 0;
      z-index: 1;
      background-color: #00ffaa;
      border-radius: 50%;
    }
    .chip {
      position: absolute;
      width: 40px;
      height: 40px;
      background-color: rgba(0, 50, 80, 0.3);
      border: 1px solid #00ffaa;
      border-radius: 5px;
      z-index: 0;
    }
    .pin {
      position: absolute;
      width: 4px;
      height: 4px;
      background-color: #ff5555;
      border-radius: 50%;
      z-index: 1;
    }
    @media (max-width: 600px) {
      body {
        padding: 10px;
      }
      .message {
        padding: 10px;
      }
    }
  </style>
</head>
<body>
  <div class="message-container">
    <h1>Happy Birthday Broertje!</h1>
    <div class="message">
      <p>Hey lief broertje, je hebt de geheime confetti modus gevonden!</p>
      <p>En heb je al eens de PCB aangesloten op je computer? Misschien om te zien wat er nog meer kan met deze PCB?</p>
      <p>Of om te kijken wat je nog meer kon programmeren met de krachtige ESP32(C3) microcontroller? Speciaal gekozen zodat ik deze gekke stunt kon uitvoeren voor je haha ;)</p>
      <p>Natuurlijk mag je doen wat je wil met de printplaat, wat je maar wil, meer informatie over deze kaart/pcb vind je hier: https://github.com/Casperdroid5/BirthdayCard<p>
      <p>Ik heb hem met alle liefde voor jou gemaakt en ik hoop dat je er net zo blij mee ben als dat ik dat ben met jou.</p>
      <p>Van harte gefeliciteerd met je 18e verjaardag en ik kijk uit naar alle jaren die we verder samen gaan ervaren.</p>
      <p>Ik hou van jou, lieve broertje.</p>
      <p class="signature">- Casper</p>
    </div>
  </div>

  <script>
    // Create circuit board elements
    function createCircuitElements() {
      // Add microchips
      for (let i = 0; i < 5; i++) {
        const chip = document.createElement('div');
        chip.className = 'chip';
        chip.style.left = Math.random() * 90 + 'vw';
        chip.style.top = Math.random() * 90 + 'vh';
        chip.style.transform = 'rotate(' + (Math.random() * 90 - 45) + 'deg)';
        document.body.appendChild(chip);
        
        // Add pins to chips
        const pinCount = Math.floor(Math.random() * 8) + 4;
        for (let p = 0; p < pinCount; p++) {
          const pin = document.createElement('div');
          pin.className = 'pin';
          pin.style.left = (Math.random() * 30 + 5) + 'px';
          pin.style.top = (Math.random() * 30 + 5) + 'px';
          chip.appendChild(pin);
        }
      }
    }
    
    // Create confetti from sides
    function createConfetti() {
      const colors = ['#00ffaa', '#00aaff', '#ff55aa', '#ffff00', '#ff5555'];
      const confettiCount = 50;
      const sides = ['left', 'right']; // Confetti comes from left and right
      
      for (let i = 0; i < confettiCount; i++) {
        const confetti = document.createElement('div');
        confetti.className = 'confetti';
        confetti.style.backgroundColor = colors[Math.floor(Math.random() * colors.length)];
        
        // Randomly choose left or right side
        const side = sides[Math.floor(Math.random() * sides.length)];
        if (side === 'left') {
          confetti.style.left = '-10px';
          confetti.style.top = Math.random() * 100 + 'vh';
        } else {
          confetti.style.left = '100vw';
          confetti.style.top = Math.random() * 100 + 'vh';
        }
        
        const size = Math.random() * 8 + 4;
        confetti.style.width = size + 'px';
        confetti.style.height = size + 'px';
        
        document.body.appendChild(confetti);
        
        const animationDuration = Math.random() * 3 + 2;
        const angle = (Math.random() * 60 - 30); // Random angle between -30 and 30 degrees
        
        // Animate diagonally across screen
        confetti.animate([
          { 
            left: confetti.style.left,
            top: confetti.style.top,
            opacity: 1 
          },
          { 
            left: (side === 'left' ? '100vw' : '-10px'),
            top: (Math.random() * 100) + 'vh',
            opacity: 0 
          }
        ], {
          duration: animationDuration * 1000,
          easing: 'cubic-bezier(0.1, 0.8, 0.9, 1)'
        });
        
        setTimeout(() => {
          confetti.remove();
        }, animationDuration * 1000);
      }
    }
    
    // Initialize page
    createCircuitElements();
    createConfetti();
    setInterval(createConfetti, 10000);
  </script>
</body>
</html>
)=====";

  server.send(200, "text/html", html);
}

void handleNotFound() {
  server.send(404, "text/plain", "404: Not found - Try broertje/");
}

void turnOffAllLEDs() {
  fill_solid(leds, NUM_LEDS, CRGB::Black);
}

void updateDisplay() {
  // Reset any ongoing animations when changing modes
  if (songState == COLOR_FADING || songState == CONFETTI_MODE) {
    songState = IDLE;
  }

  // Clear all LEDs first
  turnOffAllLEDs();
  
  // Then set the display according to current mode
  switch (currentMode) {
    case STATIC_COLOR:
      turnOnDigit1(colorOptions[currentColorIndex]);
      turnOnDigit2(colorOptions[currentColorIndex]);
      break;
      
    case RAINBOW_MODE:
      // Will be handled by updatePatterns
      break;
      
    case SNAKE_MODE:
      // Initialize snake pattern
      snakeHeadPos = 0;
      snakeHue = random8();
      break;
      
    case RANDOM_BLINK:
      // Initialize random blink
      for (int i = 0; i < 5; i++) {
        randomLEDs[i] = random8(NUM_LEDS);
        randomHues[i] = random8();
      }
      break;
      
    case CHASE_MODE:
      // Initialize chase pattern
      chasePos = 0;
      chaseHue = random8();
      break;
      
    case BREATHE_MODE:
      // Initialize breathe pattern
      breatheBrightness = 0;
      breatheIncreasing = true;
      colorFadeHue = random8();
      break;
      
    case WAVE_MODE:
      // Initialize wave pattern
      waveOffset = 0;
      colorFadeHue = random8();
      break;
      
    case OFF_MODE:
      // All LEDs off, already handled by turnOffAllLEDs()
      break;
  }
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
  
  // If not in static color mode, select a random color for the song
  if (currentMode != STATIC_COLOR) {
    currentColorIndex = random8(NUM_COLORS - 1); // Avoid the last color (black)
    Serial.print("Selected random color for song: ");
    Serial.println(currentColorIndex);
  }
  
  Serial.println("Starting birthday song");
}

void stopSong() {
  songState = IDLE;
  noTone(BUZZER);
  updateDisplay();
  Serial.println("Song stopped");
}

void startConfettiMode() {
  Serial.println("CONFETTI MODE ACTIVATED!");
  songState = CONFETTI_MODE;
  confettiStartTime = millis();
  confettiHue = 0;

  // Celebration sound
  tone(BUZZER, NOTE_E5, 200);
  delay(200);
  tone(BUZZER, NOTE_C5, 100);
  delay(100);
  tone(BUZZER, NOTE_G5, 500);

  // Start web server
  startWebServer();

  // Print secret message
  Serial.println("Secret message activated!");
  turnOffAllLEDs();
}

void updateColorFade() {
  unsigned long currentTime = millis();

  if (currentTime - lastColorFadeUpdate >= colorFadeUpdateInterval) {
    lastColorFadeUpdate = currentTime;
    colorFadeHue++;

    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i] = CHSV(colorFadeHue + (i * 255 / NUM_LEDS), 255, 255);
    }
  }
}

void updateConfettiMode() {
  // Add new confetti
  if (random8() < CONFETTI_SPAWN_RATE) {
    int pos = random16(NUM_LEDS);
    leds[pos] = CHSV(confettiHue + random8(32), 200 + random8(55), 200 + random8(55));
    confettiHue += random8(8, 16);
  }

  // Fade out
  fadeToBlackBy(leds, NUM_LEDS, CONFETTI_FADE_RATE);

  // Check duration
  if (millis() - confettiStartTime > CONFETTI_DURATION) {
    songState = IDLE;
    updateDisplay();
    WiFi.softAPdisconnect(true);
    Serial.println("Confetti mode ended");
  }
}

void updateSong() {
  unsigned long currentTime = millis();

  switch (songState) {
    case IDLE:
      break;

    case PLAYING_BIRTHDAY:
      if (!noteIsPlaying && currentTime >= previousNoteTime) {
        if (currentNote < melodyLength) {
          // Play note
          noteDuration = (tempo * 4 / noteDurationFractions[currentNote]);
          pauseDuration = noteDuration * 0.3;
          tone(BUZZER, melody[currentNote], noteDuration);

          // Light LEDs progressively
          float ledsPerNote = (float)(NUM_LEDS) / melodyLength;
          int targetLEDs = round((currentNote + 1) * ledsPerNote);

          while (ledsLit < targetLEDs && ledsLit < NUM_LEDS) {
            if (ledsLit < 6) {
              leds[digit1Mapping[ledsLit]] = colorOptions[currentColorIndex];
            } else {
              leds[digit2Mapping[ledsLit - 6]] = colorOptions[currentColorIndex];
            }
            ledsLit++;
          }

          noteEndTime = currentTime + noteDuration;
          noteIsPlaying = true;
        } else {
          // Song complete, transition to hieper
          fill_solid(leds, NUM_LEDS, colorOptions[currentColorIndex]);
          songState = PLAYING_HIEPER;
          currentHieperNote = 0;
          noteIsPlaying = false;
          previousNoteTime = currentTime + 500;
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
          if (hieperType == 0) {  // Very short beep
            tone(BUZZER, NOTE_C5, 80);
            noteDuration = 80;
            fill_solid(leds, NUM_LEDS, CRGB::White);
          } else if (hieperType == 1) {  // Short beep
            tone(BUZZER, NOTE_C5, 150);
            noteDuration = 150;
            fill_solid(leds, NUM_LEDS, CRGB::White);
          } else if (hieperType == 3) {  // Pause
            noTone(BUZZER);
            noteDuration = 300;  // 0.3-second pause
            turnOffAllLEDs();
          } else {                       // Long beep (type 2)
            tone(BUZZER, NOTE_C5, 800);  // Extended length for the final "piieeeeep"
            noteDuration = 800;
            for (int j = 0; j < NUM_LEDS; j++) {
              leds[j] = CHSV(j * 255 / NUM_LEDS, 255, 255);
            }
          }
          noteEndTime = currentTime + noteDuration;
          noteIsPlaying = true;
        } else {
          songState = COLOR_FADING;
          colorFadeHue = 0;
          lastColorFadeUpdate = currentTime;
          Serial.println("Starting color fade");
        }
      } else if (noteIsPlaying && currentTime >= noteEndTime) {
        noteIsPlaying = false;
        currentHieperNote++;
        // Adjust timing based on the previous note type
        if (currentHieperNote < hieperLength) {
          if (hieperSequence[currentHieperNote - 1] == 0) {         // After very short beep
            previousNoteTime = currentTime + 50;                    // Shorter pause
          } else if (hieperSequence[currentHieperNote - 1] == 1) {  // After short beep
            previousNoteTime = currentTime + 100;                   // Short pause
          } else {                                                  // After long beep
            previousNoteTime = currentTime + 200;                   // Longer pause
          }
        }
      }
      break;

    case COLOR_FADING:
      // Handled in main loop
      break;

    case CONFETTI_MODE:
      // Handled in main loop
      break;

    case ENDING:
      songState = IDLE;
      updateDisplay();
      break;
  }
}
