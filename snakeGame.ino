// Import of LedControl library
#include <LedControl.h>

// Initialized pins 
#define DIN_PIN 12 // Data
#define CLK_PIN 11 // CLock 
#define CS_PIN 10 // Chip select

// Joystick pins
#define JOY_X A3
#define JOY_Y A1
#define JOY_SW 2 // Switch

// Start/Pause button pin
#define START_BUTTON 3

// Game constants
#define MATRIX_SIZE 8 // Screen size 
#define GAME_SPEED 350
#define MAX_SNAKE_LENGTH 64

// LED Matrix controller
LedControl lc = LedControl(DIN_PIN, CLK_PIN, CS_PIN, 1);

// Snake data
int snakeX[MAX_SNAKE_LENGTH];
int snakeY[MAX_SNAKE_LENGTH];
int snakeLength = 2;
int dirX = 1;
int dirY = 0;

// Food position
int foodX, foodY;

// Game state
bool gameOver = false;
bool gamePaused = false;
unsigned long lastUpdate = 0;
int score = 0;

// Interrupt flag
bool buttonPressed = false;

void setup() {
  // Initialize LED Matrix
  lc.shutdown(0, false);      // Wake up display
  lc.setIntensity(0, 8);      // Sæt lysstyrke (0-15)
  lc.clearDisplay(0);         // Ryd display
  
  // Initialize start/pause button with interrupt
  pinMode(START_BUTTON, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(START_BUTTON), buttonISR, FALLING);
  
  // Initialize joystick button (not used for start anymore)
  pinMode(JOY_SW, INPUT_PULLUP);
  
  // Start spillet
  randomSeed(analogRead(A2));  // Random seed fra ubrugt analog pin
  initGame();
  
}

void loop() {
  // Handle button press from interrupt
  if (buttonPressed) {
    buttonPressed = false;
    handleButtonPress();
  }
  
  if (!gameOver && !gamePaused) {
    // Læs joystick input
    readJoystick();
    
    // Update spil med fastsat interval
    if (millis() - lastUpdate > GAME_SPEED) {
      lastUpdate = millis();
      updateGame();
      drawGame();
    }
  } else if (gameOver) {
    // Game over animation
    showGameOver();
  } else if (gamePaused) {
    // Show pause indicator (blink snake)
    showPauseIndicator();
  }
}

// Interrupt Service Routine - keep it SHORT and FAST
void buttonISR() {
  static unsigned long lastInterruptTime = 0;
  unsigned long interruptTime = millis();
  
  // Debounce: ignore interrupts within 200ms of each other
  if (interruptTime - lastInterruptTime > 200) {
    buttonPressed = true;
    lastInterruptTime = interruptTime;
  }
}

void handleButtonPress() {
  if (gameOver) {
    // Restart game
    Serial.println("Nyt spil starter!");
    initGame();
    gameOver = false;
    gamePaused = false;
  } else {
    // Toggle pause
    gamePaused = !gamePaused;
    if (gamePaused) {
      Serial.println("Spil sat på pause");
    } else {
      Serial.println("Spil genoptaget");
      lastUpdate = millis(); // Reset timer to avoid instant move
    }
  }
}

void initGame() {
  // Reset snake
  snakeLength = 2;
  dirX = 1;
  dirY = 0;
  score = 0;
  
  // Placer snake i midten
  int startX = MATRIX_SIZE / 2;
  int startY = MATRIX_SIZE / 2;
  
  for (int i = 0; i < snakeLength; i++) {
    snakeX[i] = startX - i;
    snakeY[i] = startY;
  }
  
  // Spawn første mad
  spawnFood();
}

void readJoystick() {
  int xVal = analogRead(JOY_X);
  int yVal = analogRead(JOY_Y);
  
  // Deadzone omkring midten (512 ± 200)
  // Forhindre også at slangen kan vende 180 grader
  
  if (xVal < 300 && dirX == 0) {
    // Venstre
    dirX = -1;
    dirY = 0;
  } else if (xVal > 700 && dirX == 0) {
    // Højre
    dirX = 1;
    dirY = 0;
  } else if (yVal < 300 && dirY == 0) {
    // Op
    dirX = 0;
    dirY = -1;
  } else if (yVal > 700 && dirY == 0) {
    // Ned
    dirX = 0;
    dirY = 1;
  }
}

void updateGame() {
  // Beregn ny hoved position
  int newHeadX = snakeX[0] + dirX;
  int newHeadY = snakeY[0] + dirY;
  
  // Tjek kollision med vægge
  if (newHeadX < 0 || newHeadX >= MATRIX_SIZE || 
      newHeadY < 0 || newHeadY >= MATRIX_SIZE) {
    gameOver = true;
    return;
  }
  
  // Tjek kollision med sig selv
  for (int i = 1; i < snakeLength; i++) {
    if (snakeX[i] == newHeadX && snakeY[i] == newHeadY) {
      gameOver = true;
      return;
    }
  }
  
  // Flyt snake body (start fra halen)
  for (int i = snakeLength - 1; i > 0; i--) {
    snakeX[i] = snakeX[i - 1];
    snakeY[i] = snakeY[i - 1];
  }
  
  // Opdater hoved position
  snakeX[0] = newHeadX;
  snakeY[0] = newHeadY;
  
  // Tjek om mad er spist
  if (newHeadX == foodX && newHeadY == foodY) {
    snakeLength++;
    score++;
    spawnFood();
  }
}

void drawGame() {
  // Ryd display
  lc.clearDisplay(0);
  
  // Tegn snake kroppen
  for (int i = 0; i < snakeLength; i++) {
    lc.setLed(0, snakeY[i], snakeX[i], true);
  }
  
  // Tegn mad med blink effekt
  if ((millis() / 150) % 2 == 0) {
    lc.setLed(0, foodY, foodX, true);
  }
}

void spawnFood() {
  bool validPosition = false;
  int attempts = 0;
  
  while (!validPosition && attempts < 100) {
    foodX = random(MATRIX_SIZE);
    foodY = random(MATRIX_SIZE);
    
    validPosition = true;
    attempts++;
    
    // Tjek om mad spawner på snake
    for (int i = 0; i < snakeLength; i++) {
      if (snakeX[i] == foodX && snakeY[i] == foodY) {
        validPosition = false;
        break;
      }
    }
  }
}

void showGameOver() {
  // Blink hele displayet
  unsigned long currentTime = millis();
  
  if ((currentTime / 300) % 2 == 0) {
    // Tænd alle LEDs
    for (int row = 0; row < 8; row++) {
      lc.setRow(0, row, 0xFF);
    }
  } else {
    // Sluk alle LEDs
    lc.clearDisplay(0);
  }
}

void showPauseIndicator() {
  // Blink snake mens pause
  unsigned long currentTime = millis();
  
  if ((currentTime / 500) % 2 == 0) {
    drawGame();
  } else {
    lc.clearDisplay(0);
  }
}