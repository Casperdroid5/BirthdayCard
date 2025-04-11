
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
      <p>Ik heb hem met alle liefde voor jou gemaakt en ik hoop dat je er net zo blij mee ben als ik dat ben met jou</p>
      <p>Van harte gefeliciteerd met je 18e verjaardag en ik kijk uit naar alle jaren die we verder samen gaan hebben.</p>
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
  
