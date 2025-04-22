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

// Color Options
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
#define NUM_COLORS (sizeof(colorOptions) / sizeof(colorOptions[0]))  // Added missing parenthesis

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
  updateLEDColor();
  turnOffAllLEDs();

  // Start serial
  Serial.begin(115200);
  Serial.println("18th BirthdayCard Ready!");
}

void loop() {
  checkButtons();
  checkBothButtons();
  updateSong();

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
    // Button 1 - Change color
    if (reading1 != button1State) {
      button1State = reading1;
      if (button1State == LOW) {
        currentColorIndex = ((currentColorIndex + 1) % NUM_COLORS);
        updateLEDColor();
        Serial.print("Color changed to index: ");
        Serial.println(currentColorIndex);
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

void updateLEDColor() {
  if (songState == COLOR_FADING || songState == CONFETTI_MODE) {
    songState = IDLE;
    turnOffAllLEDs();
  }

  turnOnDigit1(colorOptions[currentColorIndex]);
  turnOnDigit2(colorOptions[currentColorIndex]);
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
  Serial.println("Starting birthday song");
}

void stopSong() {
  songState = IDLE;
  noTone(BUZZER);
  updateLEDColor();
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
    updateLEDColor();
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
      updateLEDColor();
      break;
  }
}
