
#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Adafruit_NeoPixel.h>
#include <Preferences.h>

#define TFT_CS    5    // VSPI Chip Select
#define TFT_DC    21   // Data / Command
#define TFT_RST   22   // Hardware Reset
#define TFT_MOSI  23   // VSPI MOSI
#define TFT_SCK   18   // VSPI SCK
#define TFT_MISO  19   // VSPI MISO
#define BTN_UP    27   // Input Pull-up
#define BTN_DOWN  14   // Input Pull-up
#define BTN_LEFT  26   // Input Pull-up
#define BTN_RIGHT 25   // Input Pull-up
#define BTN_SEL   33   // Input Pull-up (Select / Spin)
#define BUZZER_PIN 32  // Audio Tone Generation
#define NEOPIXEL_PIN 13 // 1x WS2812B RGB Status/Win Indicator
#define NUM_PIXELS 1

struct Button {
  uint8_t pin;
  bool lastState;
  bool pressed;
  unsigned long lastPressTime;
  void update() {
    bool reading = digitalRead(pin);
    pressed = false;
    unsigned long now = millis();
    if (reading == LOW) {
     
      if (lastState == HIGH && (now - lastPressTime > 50)) {
        pressed = true;
        lastPressTime = now;
        lastState = LOW;
      }
    } else {
      
      if (now - lastPressTime > 30) {
        lastState = HIGH;
      }
    }
  }
};
Button btnUp    = {BTN_UP, HIGH, false, 0};
Button btnDown  = {BTN_DOWN, HIGH, false, 0};
Button btnLeft  = {BTN_LEFT, HIGH, false, 0};
Button btnRight = {BTN_RIGHT, HIGH, false, 0};
Button btnSel   = {BTN_SEL, HIGH, false, 0};

#define COLOR_BG        0x0821  // Deep Midnight Navy / Black
#define COLOR_PANEL     0x18C4  // Dark Slate
#define COLOR_BORDER    0x3186  // Muted Slate Border
#define COLOR_CYAN      0x07FF  // Neon Cyan
#define COLOR_MAGENTA   0xF81F  // Neon Magenta
#define COLOR_YELLOW    0xFFE0  // Bright Gold / Yellow
#define COLOR_GREEN     0x07E0  // Vivid Emerald Green
#define COLOR_RED       0xF800  // Radiant Ruby Red
#define COLOR_WHITE     0xFFFF  // Pure White
#define COLOR_DARKGREY  0x4208  // Charcoal Grey
#define COLOR_ORANGE    0xFD20  // Arcade Orange
#define COLOR_GOLD      0xFEA0  // Rich Metallic Gold

enum GameState {
  STATE_SPLASH,
  STATE_MAIN_MENU,
  STATE_ROULETTE_BET,
  STATE_ROULETTE_SPIN,
  STATE_ROULETTE_RESULT,
  STATE_SLOTS_BET,
  STATE_SLOTS_SPIN,
  STATE_SLOTS_RESULT,
  STATE_SETTINGS
};
enum RouletteBetType {
  BET_STRAIGHT_NUMBER = 0,
  BET_COLOR_RED,
  BET_COLOR_BLACK,
  BET_EVEN,
  BET_ODD,
  BET_LOW_1_18,
  BET_HIGH_19_36,
  BET_BACK_TO_LOBBY,
  ROULETTE_BET_COUNT
};
const char* ROULETTE_BET_NAMES[ROULETTE_BET_COUNT] = {
  "Single Number",
  "Color: RED",
  "Color: BLACK",
  "Parity: EVEN",
  "Parity: ODD",
  "Range: LOW (1-18)",
  "Range: HIGH (19-36)",
  "< BACK TO LOBBY >"
};

const uint8_t EUROPEAN_WHEEL[37] = {
  0, 32, 15, 19, 4, 21, 2, 25, 17, 34, 6, 27, 13, 36, 11, 30,
  8, 23, 10, 5, 24, 16, 33, 1, 20, 14, 31, 9, 22, 18, 29, 7,
  28, 12, 35, 3, 26
};

enum SlotSymbol {
  SYM_CHERRY = 0,
  SYM_BELL,
  SYM_STAR,
  SYM_DIAMOND,
  SYM_SEVEN,
  SLOT_SYM_COUNT
};
const char* SLOT_SYM_NAMES[SLOT_SYM_COUNT] = {
  "CHERRY", "BELL", "STAR", "DIAMOND", "LUCKY 7"
};
const uint16_t SLOT_SYM_COLORS[SLOT_SYM_COUNT] = {
  COLOR_RED, COLOR_GOLD, COLOR_YELLOW, COLOR_CYAN, COLOR_MAGENTA
};

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
Adafruit_NeoPixel rgbLed(NUM_PIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
Preferences prefs;
// Global Game State Variables
GameState currentState = STATE_SPLASH;
int32_t credits = 1000;
bool soundEnabled = true;
const int BET_STAKES[] = {10, 25, 50, 100};
const int NUM_STAKES = 4;
int currentStakeIdx = 0;
// Roulette Session Data
RouletteBetType rBetType = BET_STRAIGHT_NUMBER;
int rSelectedNumber = 7;
int rWinningNumber = 0;
int32_t rPayout = 0;
bool rWon = false;
float wheelAngle = 0.0f;
float ballAngle = 0.0f;
float wheelSpeed = 0.0f;
float ballSpeed = 0.0f;
unsigned long spinStartTime = 0;
// Slot Machine Session Data
int slotReelFinal[3] = {0, 0, 0};
int slotReelDisplay[3] = {0, 0, 0};
unsigned long slotSpinStart = 0;
bool slotReelStopped[3] = {false, false, false};
int32_t slotPayout = 0;
bool slotWon = false;
const char* slotWinTitle = "";
// Menu Navigation
int menuSelection = 0; // 0=Roulette, 1=Slots, 2=Settings
int settingsSelection = 0; // 0=Sound Toggle, 1=Reset Credits, 2=Exit
// Non-blocking Tone Engine
unsigned long toneStopTime = 0;
bool toneActive = false;
// ----------------------------------------------------------------------------
// SOUND & AUDIO FUNCTIONS
// ----------------------------------------------------------------------------
void playTone(unsigned int freq, unsigned long duration) {
  if (!soundEnabled) return;
  tone(BUZZER_PIN, freq);
  toneActive = true;
  toneStopTime = millis() + duration;
}
void soundTick() {
  playTone(880, 15);
}
void soundButton() {
  playTone(1200, 25);
}
void soundSelect() {
  playTone(1760, 40);
}
void soundWin() {
  if (!soundEnabled) return;
  playTone(1046, 70); // C6
}
void soundJackpot() {
  if (!soundEnabled) return;
  playTone(2093, 120); // C7
}
void soundLose() {
  if (!soundEnabled) return;
  playTone(220, 180);
}
void updateAudio() {
  if (toneActive && millis() >= toneStopTime) {
    noTone(BUZZER_PIN);
    toneActive = false;
  }
}
// ----------------------------------------------------------------------------
// RGB LED FEEDBACK
// ----------------------------------------------------------------------------
void setLedColor(uint8_t r, uint8_t g, uint8_t b) {
  rgbLed.setPixelColor(0, rgbLed.Color(r, g, b));
  rgbLed.show();
}
void ledPulse(uint8_t r, uint8_t g, uint8_t b) {
  setLedColor(r, g, b);
}
// ----------------------------------------------------------------------------
// INPUT SCANNING
// ----------------------------------------------------------------------------
void scanInputs() {
  btnUp.update();
  btnDown.update();
  btnLeft.update();
  btnRight.update();
  btnSel.update();
  if (btnUp.pressed)    Serial.println(">>> [BUTTON] UP Pressed");
  if (btnDown.pressed)  Serial.println(">>> [BUTTON] DOWN Pressed");
  if (btnLeft.pressed)  Serial.println(">>> [BUTTON] LEFT Pressed");
  if (btnRight.pressed) Serial.println(">>> [BUTTON] RIGHT Pressed");
  if (btnSel.pressed)   Serial.println(">>> [BUTTON] SELECT Pressed");
}
// ----------------------------------------------------------------------------
// HELPER DRAWING FUNCTIONS
// ----------------------------------------------------------------------------
void drawHeader(const char* title) {
  tft.fillRect(0, 0, 320, 28, COLOR_PANEL);
  tft.drawFastHLine(0, 28, 320, COLOR_CYAN);
  tft.setTextSize(2);
  tft.setTextColor(COLOR_YELLOW, COLOR_PANEL);
  tft.setCursor(10, 6);
  tft.print(title);
  // Credit Badge
  tft.fillRect(190, 4, 125, 20, COLOR_BG);
  tft.drawRect(190, 4, 125, 20, COLOR_GOLD);
  tft.setTextSize(1);
  tft.setTextColor(COLOR_GOLD, COLOR_BG);
  tft.setCursor(196, 10);
  tft.print("$ CR:");
  tft.setTextSize(1);
  tft.setTextColor(COLOR_WHITE, COLOR_BG);
  tft.setCursor(240, 10);
  tft.printf("%6d", credits);
}
void drawFooter(const char* hint) {
  tft.fillRect(0, 222, 320, 18, COLOR_PANEL);
  tft.drawFastHLine(0, 221, 320, COLOR_BORDER);
  tft.setTextSize(1);
  tft.setTextColor(COLOR_CYAN, COLOR_PANEL);
  tft.setCursor(8, 226);
  tft.print(hint);
}
// European Roulette Color Lookup
bool isRouletteRed(int num) {
  const uint8_t RED_NUMS[] = {1, 3, 5, 7, 9, 12, 14, 16, 18, 19, 21, 23, 25, 27, 30, 32, 34, 36};
  for (int i = 0; i < 18; i++) {
    if (RED_NUMS[i] == num) return true;
  }
  return false;
}
uint16_t getRouletteNumberColor(int num) {
  if (num == 0) return COLOR_GREEN;
  if (isRouletteRed(num)) return COLOR_RED;
  return COLOR_DARKGREY;
}
// ----------------------------------------------------------------------------
// STATE: SPLASH SCREEN
// ----------------------------------------------------------------------------
unsigned long splashTimer = 0;
void enterSplash() {
  currentState = STATE_SPLASH;
  splashTimer = millis();
  Serial.println("[STATE] Splash Screen started. Press SELECT or wait 3.5s to enter Lobby.");
  tft.fillScreen(COLOR_BG);
  // Glowing Retro Banner
  for (int b = 0; b < 4; b++) {
    tft.drawRoundRect(15 - b, 20 - b, 290 + (b * 2), 170 + (b * 2), 12, (b == 0) ? COLOR_MAGENTA : COLOR_PANEL);
  }
  tft.setTextSize(3);
  tft.setTextColor(COLOR_CYAN, COLOR_BG);
  tft.setCursor(45, 45);
  tft.print("MINI ARCADE");
  tft.setTextSize(2);
  tft.setTextColor(COLOR_YELLOW, COLOR_BG);
  tft.setCursor(65, 85);
  tft.print("CASINO RETRO");
  // Decorative Casino Icons
  tft.setTextSize(1);
  tft.setTextColor(COLOR_GREEN, COLOR_BG);
  tft.setCursor(60, 120);
  tft.print("[ ROULETTE ]  [ SLOTS ]");
  tft.setTextColor(COLOR_GOLD, COLOR_BG);
  tft.setCursor(75, 145);
  tft.print("ESP32 + 3.3\" TFT");
  tft.fillRect(0, 210, 320, 30, COLOR_PANEL);
  tft.setTextSize(1);
  tft.setTextColor(COLOR_WHITE, COLOR_PANEL);
  tft.setCursor(65, 220);
  tft.print("PRESS [SELECT] TO START");
  setLedColor(0, 80, 80);
  playTone(880, 80);
}
void handleSplash() {
  // Flashing prompt
  if ((millis() / 450) % 2 == 0) {
    tft.setTextColor(COLOR_YELLOW, COLOR_PANEL);
  } else {
    tft.setTextColor(COLOR_WHITE, COLOR_PANEL);
  }
  tft.setCursor(65, 220);
  tft.print("PRESS [SELECT] TO START");
  if (btnSel.pressed || (millis() - splashTimer > 3500)) {
    soundSelect();
    enterMainMenu();
  }
}
// ----------------------------------------------------------------------------
// STATE: MAIN MENU
// ----------------------------------------------------------------------------
void drawMenuCard(int index, const char* title, const char* subtitle, int x, int y, int w, int h, bool selected) {
  uint16_t borderColor = selected ? COLOR_CYAN : COLOR_BORDER;
  uint16_t bgColor = selected ? COLOR_PANEL : COLOR_BG;
  tft.fillRoundRect(x, y, w, h, 8, bgColor);
  tft.drawRoundRect(x, y, w, h, 8, borderColor);
  if (selected) {
    tft.drawRoundRect(x + 1, y + 1, w - 2, h - 2, 7, COLOR_WHITE);
  }
  tft.setTextSize(2);
  tft.setTextColor(selected ? COLOR_YELLOW : COLOR_WHITE, bgColor);
  tft.setCursor(x + 16, y + 14);
  tft.print(title);
  tft.setTextSize(1);
  tft.setTextColor(selected ? COLOR_CYAN : COLOR_DARKGREY, bgColor);
  tft.setCursor(x + 18, y + 38);
  tft.print(subtitle);
}
void renderMainMenu() {
  drawHeader("CASINO LOBBY");
  drawFooter("UP/DOWN: Navigate | SELECT: Enter Game");
  drawMenuCard(0, "1. ROULETTE", "European 0-36, Straight & Outside Bets", 20, 42, 280, 52, menuSelection == 0);
  drawMenuCard(1, "2. SLOT MACHINE", "3-Reel Classic Cyber Vegas, 100x Jackpot", 20, 100, 280, 52, menuSelection == 1);
  drawMenuCard(2, "3. SETTINGS & CR", "Sound Toggle, Reset Balance, Info", 20, 158, 280, 52, menuSelection == 2);
}
void enterMainMenu() {
  currentState = STATE_MAIN_MENU;
  tft.fillScreen(COLOR_BG);
  setLedColor(20, 20, 50);
  renderMainMenu();
  Serial.printf("[STATE] Entered Casino Lobby (Selection: %d)\n", menuSelection);
}
void handleMainMenu() {
  if (btnUp.pressed) {
    soundTick();
    menuSelection = (menuSelection - 1 + 3) % 3;
    Serial.printf("[LOBBY] Highlighted: %d (%s)\n", menuSelection, (menuSelection == 0 ? "Roulette" : (menuSelection == 1 ? "Slots" : "Settings")));
    renderMainMenu();
  }
  if (btnDown.pressed) {
    soundTick();
    menuSelection = (menuSelection + 1) % 3;
    Serial.printf("[LOBBY] Highlighted: %d (%s)\n", menuSelection, (menuSelection == 0 ? "Roulette" : (menuSelection == 1 ? "Slots" : "Settings")));
    renderMainMenu();
  }
  if (btnSel.pressed) {
    soundSelect();
    Serial.printf("[LOBBY] Entering option %d\n", menuSelection);
    if (menuSelection == 0) enterRouletteBet();
    else if (menuSelection == 1) enterSlotsBet();
    else if (menuSelection == 2) enterSettings();
  }
}
// ----------------------------------------------------------------------------
// STATE: ROULETTE — BET CONFIGURATION
// ----------------------------------------------------------------------------
void renderRouletteBetUI() {
  drawHeader("EUROPEAN ROULETTE");
  drawFooter("UP/DN: Bet Type | L/R: Stake | SEL: SPIN");
  // Bet Configuration Box
  tft.fillRoundRect(15, 38, 290, 175, 8, COLOR_PANEL);
  tft.drawRoundRect(15, 38, 290, 175, 8, COLOR_CYAN);
  // Bet Type Heading
  tft.setTextSize(1);
  tft.setTextColor(COLOR_YELLOW, COLOR_PANEL);
  tft.setCursor(26, 48);
  tft.print("CHOSEN BET TYPE: (UP/DOWN to switch)");
  tft.fillRect(25, 62, 270, 26, COLOR_BG);
  tft.drawRect(25, 62, 270, 26, COLOR_MAGENTA);
  tft.setTextSize(2);
  tft.setTextColor(COLOR_WHITE, COLOR_BG);
  tft.setCursor(30, 67);
  tft.print(ROULETTE_BET_NAMES[rBetType]);
  // Straight Number Selector Sub-panel
  if (rBetType == BET_STRAIGHT_NUMBER) {
    tft.setTextSize(1);
    tft.setTextColor(COLOR_CYAN, COLOR_PANEL);
    tft.setCursor(26, 96);
    tft.print("TARGET NUMBER (0 - 36):");
    uint16_t numColor = getRouletteNumberColor(rSelectedNumber);
    tft.fillCircle(160, 126, 22, numColor);
    tft.drawCircle(160, 126, 22, COLOR_WHITE);
    tft.drawCircle(160, 126, 23, COLOR_GOLD);
    tft.setTextSize(2);
    tft.setTextColor(COLOR_WHITE, numColor);
    if (rSelectedNumber < 10) tft.setCursor(154, 120);
    else tft.setCursor(148, 120);
    tft.print(rSelectedNumber);
    tft.setTextSize(1);
    tft.setTextColor(COLOR_GOLD, COLOR_PANEL);
    tft.setCursor(195, 122);
    tft.print("Payout: 35 to 1");
  } else if (rBetType == BET_BACK_TO_LOBBY) {
    tft.setTextSize(1);
    tft.setTextColor(COLOR_YELLOW, COLOR_PANEL);
    tft.setCursor(26, 102);
    tft.print("Return to Casino Main Menu.");
    tft.setCursor(26, 122);
    tft.setTextColor(COLOR_CYAN, COLOR_PANEL);
    tft.print("Press [SELECT] to return to Lobby.");
  } else {
    // Explanation for Outside Bets
    tft.setTextSize(1);
    tft.setTextColor(COLOR_WHITE, COLOR_PANEL);
    tft.setCursor(26, 102);
    if (rBetType == BET_COLOR_RED) tft.print("All 18 RED Numbers win.");
    else if (rBetType == BET_COLOR_BLACK) tft.print("All 18 BLACK Numbers win.");
    else if (rBetType == BET_EVEN) tft.print("Wins on 2, 4, 6... 36 (0 loses).");
    else if (rBetType == BET_ODD) tft.print("Wins on 1, 3, 5... 35.");
    else if (rBetType == BET_LOW_1_18) tft.print("Wins on numbers 1 to 18.");
    else if (rBetType == BET_HIGH_19_36) tft.print("Wins on numbers 19 to 36.");
    tft.setCursor(26, 122);
    tft.setTextColor(COLOR_GOLD, COLOR_PANEL);
    tft.print("Standard European Payout: 1 to 1 (Double)");
  }
  // Stake Selection
  tft.fillRect(25, 155, 270, 48, COLOR_BG);
  tft.drawRect(25, 155, 270, 48, COLOR_BORDER);
  if (rBetType == BET_BACK_TO_LOBBY) {
    tft.fillRoundRect(35, 160, 250, 38, 6, COLOR_PANEL);
    tft.drawRoundRect(35, 160, 250, 38, 6, COLOR_GOLD);
    tft.setTextSize(2);
    tft.setTextColor(COLOR_YELLOW, COLOR_PANEL);
    tft.setCursor(55, 172);
    tft.print("< EXIT TO LOBBY >");
    drawFooter("SELECT: Exit to Lobby | UP/DN: Change");
    return;
  }
  tft.setTextSize(1);
  tft.setTextColor(COLOR_CYAN, COLOR_BG);
  tft.setCursor(32, 162);
  tft.print("WAGER STAKE: [LEFT / RIGHT]");
  for (int s = 0; s < NUM_STAKES; s++) {
    int sx = 32 + (s * 64);
    int sy = 175;
    bool activeStake = (s == currentStakeIdx);
    tft.fillRoundRect(sx, sy, 56, 22, 4, activeStake ? COLOR_GOLD : COLOR_PANEL);
    tft.setTextSize(1);
    tft.setTextColor(activeStake ? COLOR_BG : COLOR_WHITE, activeStake ? COLOR_GOLD : COLOR_PANEL);
    tft.setCursor(sx + 8, sy + 7);
    tft.printf("$%d", BET_STAKES[s]);
  }
}
void enterRouletteBet() {
  currentState = STATE_ROULETTE_BET;
  tft.fillScreen(COLOR_BG);
  setLedColor(0, 100, 0);
  renderRouletteBetUI();
  Serial.println("[STATE] Entered European Roulette Betting");
}
void handleRouletteBet() {
  bool redraw = false;
  if (btnUp.pressed) {
    soundTick();
    rBetType = (RouletteBetType)((rBetType - 1 + ROULETTE_BET_COUNT) % ROULETTE_BET_COUNT);
    redraw = true;
  }
  if (btnDown.pressed) {
    soundTick();
    rBetType = (RouletteBetType)((rBetType + 1) % ROULETTE_BET_COUNT);
    redraw = true;
  }
  if (btnLeft.pressed) {
    soundTick();
    if (rBetType == BET_STRAIGHT_NUMBER) {
      rSelectedNumber = (rSelectedNumber - 1 + 37) % 37;
    } else if (rBetType != BET_BACK_TO_LOBBY) {
      currentStakeIdx = (currentStakeIdx - 1 + NUM_STAKES) % NUM_STAKES;
    }
    redraw = true;
  }
  if (btnRight.pressed) {
    soundTick();
    if (rBetType == BET_STRAIGHT_NUMBER) {
      rSelectedNumber = (rSelectedNumber + 1) % 37;
    } else if (rBetType != BET_BACK_TO_LOBBY) {
      currentStakeIdx = (currentStakeIdx + 1) % NUM_STAKES;
    }
    redraw = true;
  }
  if (btnSel.pressed) {
    if (rBetType == BET_BACK_TO_LOBBY) {
      soundButton();
      enterMainMenu();
      return;
    }
    int stake = BET_STAKES[currentStakeIdx];
    if (credits < stake) {
      soundLose();
      tft.fillRect(40, 95, 240, 50, COLOR_RED);
      tft.drawRect(40, 95, 240, 50, COLOR_WHITE);
      tft.setTextSize(2);
      tft.setTextColor(COLOR_WHITE, COLOR_RED);
      tft.setCursor(55, 110);
      tft.print("LOW BALANCE!");
      delay(800);
      redraw = true;
    } else {
      soundSelect();
      // Deduct stake exactly once
      credits -= stake;
      enterRouletteSpin();
      return;
    }
  }
  if (redraw) {
    renderRouletteBetUI();
  }
}
// ----------------------------------------------------------------------------
// STATE: ROULETTE — ANIMATED SPIN & SETTLEMENT
// ----------------------------------------------------------------------------
void enterRouletteSpin() {
  currentState = STATE_ROULETTE_SPIN;
  tft.fillScreen(COLOR_BG);
  drawHeader("WHEEL IS SPINNING...");
  drawFooter("NO MORE BETS! BALL IS IN MOTION");
  // Settle True RNG Result independently from visual animation!
  rWinningNumber = esp_random() % 37;
  // Determine Win and Payout
  int stake = BET_STAKES[currentStakeIdx];
  rWon = false;
  rPayout = 0;
  if (rBetType == BET_STRAIGHT_NUMBER) {
    if (rWinningNumber == rSelectedNumber) {
      rWon = true;
      rPayout = stake * 36; // 35:1 profit + original stake
    }
  } else if (rWinningNumber != 0) { // European rules: 0 loses all outside bets
    if (rBetType == BET_COLOR_RED && isRouletteRed(rWinningNumber)) {
      rWon = true;
      rPayout = stake * 2;
    } else if (rBetType == BET_COLOR_BLACK && !isRouletteRed(rWinningNumber)) {
      rWon = true;
      rPayout = stake * 2;
    } else if (rBetType == BET_EVEN && (rWinningNumber % 2 == 0)) {
      rWon = true;
      rPayout = stake * 2;
    } else if (rBetType == BET_ODD && (rWinningNumber % 2 == 1)) {
      rWon = true;
      rPayout = stake * 2;
    } else if (rBetType == BET_LOW_1_18 && (rWinningNumber >= 1 && rWinningNumber <= 18)) {
      rWon = true;
      rPayout = stake * 2;
    } else if (rBetType == BET_HIGH_19_36 && (rWinningNumber >= 19 && rWinningNumber <= 36)) {
      rWon = true;
      rPayout = stake * 2;
    }
  }
  spinStartTime = millis();
  wheelAngle = 0;
  ballAngle = 0;
  wheelSpeed = 0.35f;
  ballSpeed = -0.55f;
  setLedColor(80, 40, 0);
}
void handleRouletteSpin() {
  unsigned long elapsed = millis() - spinStartTime;
  // Wheel Graphic center: (160, 125), Radius: 70
  int cx = 160;
  int cy = 125;
  int r = 68;
  // Draw static wheel frame once
  if (elapsed < 50) {
    tft.drawCircle(cx, cy, r + 4, COLOR_GOLD);
    tft.drawCircle(cx, cy, r + 2, COLOR_WHITE);
    tft.drawCircle(cx, cy, r - 15, COLOR_PANEL);
    tft.fillCircle(cx, cy, 28, COLOR_PANEL);
    tft.drawCircle(cx, cy, 28, COLOR_CYAN);
  }
  // Physics Deceleration
  if (elapsed > 1200) {
    ballSpeed *= 0.985f;
    wheelSpeed *= 0.990f;
  }
  // Clear previous ball
  int oldBx = cx + (int)((r - 6) * cos(ballAngle));
  int oldBy = cy + (int)((r - 6) * sin(ballAngle));
  tft.fillCircle(oldBx, oldBy, 4, COLOR_BG);
  // Update positions
  wheelAngle += wheelSpeed;
  ballAngle += ballSpeed;
  // Draw new ball
  int bx = cx + (int)((r - 6) * cos(ballAngle));
  int by = cy + (int)((r - 6) * sin(ballAngle));
  tft.fillCircle(bx, by, 4, COLOR_WHITE);
  // Audio tick on pocket pass
  static int lastTickStep = 0;
  int tickStep = (int)(abs(ballAngle) * 5.89f);
  if (tickStep != lastTickStep) {
    soundTick();
    lastTickStep = tickStep;
  }
  // Center Hub Info: Dynamic Speed / Status
  tft.setTextSize(1);
  tft.setTextColor(COLOR_YELLOW, COLOR_PANEL);
  tft.setCursor(cx - 15, cy - 4);
  tft.print("SPIN");
  // Spin termination after 3.8 seconds
  if (elapsed > 3800 || abs(ballSpeed) < 0.02f) {
    enterRouletteResult();
  }
}
// ----------------------------------------------------------------------------
// STATE: ROULETTE — RESULT & PAYOUT
// ----------------------------------------------------------------------------
void enterRouletteResult() {
  currentState = STATE_ROULETTE_RESULT;
  if (rWon) {
    credits += rPayout;
    soundWin();
    setLedColor(0, 150, 0); // Vibrant Green
  } else {
    soundLose();
    setLedColor(150, 0, 0); // Vibrant Red
  }
  tft.fillScreen(COLOR_BG);
  drawHeader("ROULETTE RESULT");
  drawFooter("SELECT: Play Again | LEFT: Lobby Menu");
  // Result Badge
  uint16_t numColor = getRouletteNumberColor(rWinningNumber);
  tft.fillCircle(160, 85, 34, numColor);
  tft.drawCircle(160, 85, 34, COLOR_WHITE);
  tft.drawCircle(160, 85, 36, COLOR_GOLD);
  tft.setTextSize(3);
  tft.setTextColor(COLOR_WHITE, numColor);
  if (rWinningNumber < 10) tft.setCursor(152, 75);
  else tft.setCursor(144, 75);
  tft.print(rWinningNumber);
  // Result Description Banner
  tft.setTextSize(2);
  if (rWinningNumber == 0) {
    tft.setTextColor(COLOR_GREEN, COLOR_BG);
    tft.setCursor(105, 130);
    tft.print("ZERO (GREEN)");
  } else if (isRouletteRed(rWinningNumber)) {
    tft.setTextColor(COLOR_RED, COLOR_BG);
    tft.setCursor(130, 130);
    tft.print("RED");
  } else {
    tft.setTextColor(COLOR_WHITE, COLOR_BG);
    tft.setCursor(120, 130);
    tft.print("BLACK");
  }
  // Win / Loss Announcement
  tft.fillRoundRect(25, 155, 270, 56, 8, rWon ? COLOR_PANEL : COLOR_BG);
  tft.drawRoundRect(25, 155, 270, 56, 8, rWon ? COLOR_GREEN : COLOR_RED);
  tft.setTextSize(2);
  tft.setCursor(45, 165);
  if (rWon) {
    tft.setTextColor(COLOR_GREEN, COLOR_PANEL);
    tft.print("YOU WON! +$");
    tft.print(rPayout);
    tft.setTextSize(1);
    tft.setTextColor(COLOR_GOLD, COLOR_PANEL);
    tft.setCursor(45, 192);
    tft.printf("Net Profit: +$%d credits", rPayout - BET_STAKES[currentStakeIdx]);
  } else {
    tft.setTextColor(COLOR_RED, COLOR_BG);
    tft.print("HOUSE WINS");
    tft.setTextSize(1);
    tft.setTextColor(COLOR_WHITE, COLOR_BG);
    tft.setCursor(45, 192);
    tft.printf("Lost Stake: -$%d credits", BET_STAKES[currentStakeIdx]);
  }
}
void handleRouletteResult() {
  if (btnSel.pressed) {
    soundSelect();
    enterRouletteBet();
  }
  if (btnLeft.pressed || btnDown.pressed) {
    soundButton();
    enterMainMenu();
  }
}
// ----------------------------------------------------------------------------
// STATE: SLOT MACHINE — SETUP & PAYTABLE
// ----------------------------------------------------------------------------
void drawSlotSymbolGraphic(int x, int y, int sym) {
  // Draw clean, high-contrast, scalable icon
  tft.fillRect(x, y, 54, 54, COLOR_PANEL);
  tft.drawRect(x, y, 54, 54, COLOR_BORDER);
  switch (sym) {
    case SYM_CHERRY:
      tft.fillCircle(x + 18, y + 34, 10, COLOR_RED);
      tft.fillCircle(x + 36, y + 34, 10, COLOR_RED);
      tft.drawLine(x + 18, y + 24, x + 27, y + 14, COLOR_GREEN);
      tft.drawLine(x + 36, y + 24, x + 27, y + 14, COLOR_GREEN);
      break;
    case SYM_BELL:
      tft.fillRoundRect(x + 15, y + 18, 24, 22, 6, COLOR_GOLD);
      tft.fillCircle(x + 27, y + 42, 5, COLOR_WHITE);
      break;
    case SYM_STAR:
      tft.fillTriangle(x + 27, y + 10, x + 12, y + 44, x + 42, y + 44, COLOR_YELLOW);
      tft.fillTriangle(x + 27, y + 46, x + 12, y + 18, x + 42, y + 18, COLOR_YELLOW);
      break;
    case SYM_DIAMOND:
      tft.fillTriangle(x + 27, y + 10, x + 10, y + 27, x + 44, y + 27, COLOR_CYAN);
      tft.fillTriangle(x + 27, y + 44, x + 10, y + 27, x + 44, y + 27, COLOR_CYAN);
      break;
    case SYM_SEVEN:
      tft.setTextSize(4);
      tft.setTextColor(COLOR_MAGENTA, COLOR_PANEL);
      tft.setCursor(x + 16, y + 13);
      tft.print("7");
      break;
  }
}
void renderSlotsBetUI() {
  drawHeader("CYBER 777 SLOTS");
  drawFooter("L/R: Stake | SELECT: SPIN | DOWN: Exit");
  // Reels Window Frame
  tft.fillRoundRect(15, 36, 290, 80, 8, COLOR_BG);
  tft.drawRoundRect(15, 36, 290, 80, 8, COLOR_GOLD);
  tft.drawRoundRect(16, 37, 288, 78, 7, COLOR_MAGENTA);
  // Reel 1, Reel 2, Reel 3
  drawSlotSymbolGraphic(35, 48, slotReelDisplay[0]);
  drawSlotSymbolGraphic(133, 48, slotReelDisplay[1]);
  drawSlotSymbolGraphic(231, 48, slotReelDisplay[2]);
  // Mini Paytable Box
  tft.fillRoundRect(15, 122, 290, 48, 6, COLOR_PANEL);
  tft.drawRoundRect(15, 122, 290, 48, 6, COLOR_BORDER);
  tft.setTextSize(1);
  tft.setTextColor(COLOR_GOLD, COLOR_PANEL);
  tft.setCursor(24, 128);
  tft.print("7-7-7: 100x JACKPOT! | DIAMOND: 50x | STAR: 25x");
  tft.setCursor(24, 142);
  tft.setTextColor(COLOR_CYAN, COLOR_PANEL);
  tft.print("BELL: 15x | 3x CHERRY: 10x | 2x CHERRY: 3x");
  // Stake Selection
  tft.fillRect(15, 176, 290, 42, COLOR_BG);
  tft.drawRect(15, 176, 290, 42, COLOR_BORDER);
  tft.setTextSize(1);
  tft.setTextColor(COLOR_WHITE, COLOR_BG);
  tft.setCursor(24, 182);
  tft.print("BET AMOUNT: (L/R)");
  for (int s = 0; s < NUM_STAKES; s++) {
    int sx = 24 + (s * 68);
    int sy = 194;
    bool activeStake = (s == currentStakeIdx);
    tft.fillRoundRect(sx, sy, 58, 20, 4, activeStake ? COLOR_GOLD : COLOR_PANEL);
    tft.setTextSize(1);
    tft.setTextColor(activeStake ? COLOR_BG : COLOR_WHITE, activeStake ? COLOR_GOLD : COLOR_PANEL);
    tft.setCursor(sx + 10, sy + 6);
    tft.printf("$%d", BET_STAKES[s]);
  }
}
void enterSlotsBet() {
  currentState = STATE_SLOTS_BET;
  tft.fillScreen(COLOR_BG);
  setLedColor(80, 0, 80);
  renderSlotsBetUI();
  Serial.println("[STATE] Entered Cyber 777 Slots Betting");
}
void handleSlotsBet() {
  bool redraw = false;
  if (btnLeft.pressed) {
    soundTick();
    currentStakeIdx = (currentStakeIdx - 1 + NUM_STAKES) % NUM_STAKES;
    redraw = true;
  }
  if (btnRight.pressed) {
    soundTick();
    currentStakeIdx = (currentStakeIdx + 1) % NUM_STAKES;
    redraw = true;
  }
  if (btnDown.pressed || btnUp.pressed) {
    soundButton();
    enterMainMenu();
    return;
  }
  if (btnSel.pressed) {
    int stake = BET_STAKES[currentStakeIdx];
    if (credits < stake) {
      soundLose();
      tft.fillRect(40, 70, 240, 50, COLOR_RED);
      tft.drawRect(40, 70, 240, 50, COLOR_WHITE);
      tft.setTextSize(2);
      tft.setTextColor(COLOR_WHITE, COLOR_RED);
      tft.setCursor(55, 85);
      tft.print("LOW BALANCE!");
      delay(800);
      redraw = true;
    } else {
      soundSelect();
      credits -= stake;
      enterSlotsSpin();
      return;
    }
  }
  if (redraw) {
    renderSlotsBetUI();
  }
}
// ----------------------------------------------------------------------------
// STATE: SLOT MACHINE — ANIMATED MULTI-STAGE REEL SPIN
// ----------------------------------------------------------------------------
void enterSlotsSpin() {
  currentState = STATE_SLOTS_SPIN;
  drawHeader("REELS SPINNING...");
  drawFooter("GOOD LUCK! WATCH THE PAYLINE");
  // Determine True RNG Target Outcome BEFORE animating
  for (int i = 0; i < 3; i++) {
    // Weighted probabilities: 7s and Diamonds are rarer!
    uint32_t r = esp_random() % 100;
    if (r < 8)       slotReelFinal[i] = SYM_SEVEN;    // 8% chance
    else if (r < 22) slotReelFinal[i] = SYM_DIAMOND;  // 14% chance
    else if (r < 44) slotReelFinal[i] = SYM_STAR;     // 22% chance
    else if (r < 70) slotReelFinal[i] = SYM_BELL;     // 26% chance
    else             slotReelFinal[i] = SYM_CHERRY;   // 30% chance
  }
  // Calculate Paytable Settlement
  int stake = BET_STAKES[currentStakeIdx];
  slotWon = false;
  slotPayout = 0;
  slotWinTitle = "";
  int s0 = slotReelFinal[0];
  int s1 = slotReelFinal[1];
  int s2 = slotReelFinal[2];
  if (s0 == SYM_SEVEN && s1 == SYM_SEVEN && s2 == SYM_SEVEN) {
    slotWon = true;
    slotPayout = stake * 100;
    slotWinTitle = "JACKPOT 777!";
  } else if (s0 == SYM_DIAMOND && s1 == SYM_DIAMOND && s2 == SYM_DIAMOND) {
    slotWon = true;
    slotPayout = stake * 50;
    slotWinTitle = "DIAMOND TRIO!";
  } else if (s0 == SYM_STAR && s1 == SYM_STAR && s2 == SYM_STAR) {
    slotWon = true;
    slotPayout = stake * 25;
    slotWinTitle = "STAR TRIFECTA!";
  } else if (s0 == SYM_BELL && s1 == SYM_BELL && s2 == SYM_BELL) {
    slotWon = true;
    slotPayout = stake * 15;
    slotWinTitle = "TRIPLE BELLS!";
  } else if (s0 == SYM_CHERRY && s1 == SYM_CHERRY && s2 == SYM_CHERRY) {
    slotWon = true;
    slotPayout = stake * 10;
    slotWinTitle = "TRIPLE CHERRIES!";
  } else {
    // Partial 2x or 1x Cherry Wins
    int cherryCount = 0;
    if (s0 == SYM_CHERRY) cherryCount++;
    if (s1 == SYM_CHERRY) cherryCount++;
    if (s2 == SYM_CHERRY) cherryCount++;
    if (cherryCount == 2) {
      slotWon = true;
      slotPayout = stake * 3;
      slotWinTitle = "2x CHERRY PAIR!";
    } else if (cherryCount == 1) {
      slotWon = true;
      slotPayout = stake * 1;
      slotWinTitle = "1x CHERRY RETURN";
    }
  }
  slotSpinStart = millis();
  slotReelStopped[0] = false;
  slotReelStopped[1] = false;
  slotReelStopped[2] = false;
}
void handleSlotsSpin() {
  unsigned long elapsed = millis() - slotSpinStart;
  // Staggered Reel Stopping Times for suspense!
  // Reel 0 stops at 1000ms
  // Reel 1 stops at 1800ms
  // Reel 2 stops at 2600ms
  if (elapsed >= 1000 && !slotReelStopped[0]) {
    slotReelStopped[0] = true;
    slotReelDisplay[0] = slotReelFinal[0];
    drawSlotSymbolGraphic(35, 48, slotReelDisplay[0]);
    soundButton();
  }
  if (elapsed >= 1800 && !slotReelStopped[1]) {
    slotReelStopped[1] = true;
    slotReelDisplay[1] = slotReelFinal[1];
    drawSlotSymbolGraphic(133, 48, slotReelDisplay[1]);
    soundButton();
  }
  if (elapsed >= 2600 && !slotReelStopped[2]) {
    slotReelStopped[2] = true;
    slotReelDisplay[2] = slotReelFinal[2];
    drawSlotSymbolGraphic(231, 48, slotReelDisplay[2]);
    soundButton();
  }
  // Animate spinning reels before they stop
  static unsigned long lastTick = 0;
  if (millis() - lastTick > 75) {
    lastTick = millis();
    soundTick();
    if (!slotReelStopped[0]) {
      slotReelDisplay[0] = (slotReelDisplay[0] + 1) % SLOT_SYM_COUNT;
      drawSlotSymbolGraphic(35, 48, slotReelDisplay[0]);
    }
    if (!slotReelStopped[1]) {
      slotReelDisplay[1] = (slotReelDisplay[1] + 1) % SLOT_SYM_COUNT;
      drawSlotSymbolGraphic(133, 48, slotReelDisplay[1]);
    }
    if (!slotReelStopped[2]) {
      slotReelDisplay[2] = (slotReelDisplay[2] + 1) % SLOT_SYM_COUNT;
      drawSlotSymbolGraphic(231, 48, slotReelDisplay[2]);
    }
    // Flashing NeoPixel during spin
    setLedColor(esp_random() % 100, esp_random() % 100, esp_random() % 100);
  }
  // When all 3 reels stop, transition to result
  if (slotReelStopped[0] && slotReelStopped[1] && slotReelStopped[2]) {
    enterSlotsResult();
  }
}
// ----------------------------------------------------------------------------
// STATE: SLOT MACHINE — RESULT & CELEBRATION
// ----------------------------------------------------------------------------
void enterSlotsResult() {
  currentState = STATE_SLOTS_RESULT;
  if (slotWon) {
    credits += slotPayout;
    if (slotPayout >= BET_STAKES[currentStakeIdx] * 50) {
      soundJackpot();
      setLedColor(255, 215, 0); // Brilliant Gold
    } else {
      soundWin();
      setLedColor(0, 180, 0); // Green
    }
  } else {
    soundLose();
    setLedColor(120, 0, 0); // Red
  }
  // Draw Win Banner Box over bottom panel
  tft.fillRoundRect(15, 122, 290, 94, 8, slotWon ? COLOR_PANEL : COLOR_BG);
  tft.drawRoundRect(15, 122, 290, 94, 8, slotWon ? COLOR_GOLD : COLOR_RED);
  tft.setTextSize(2);
  tft.setCursor(30, 134);
  if (slotWon) {
    tft.setTextColor(COLOR_YELLOW, COLOR_PANEL);
    tft.print(slotWinTitle);
    tft.setTextSize(2);
    tft.setTextColor(COLOR_GREEN, COLOR_PANEL);
    tft.setCursor(30, 160);
    tft.printf("PAYOUT: +$%d", slotPayout);
    tft.setTextSize(1);
    tft.setTextColor(COLOR_WHITE, COLOR_PANEL);
    tft.setCursor(30, 192);
    tft.print("SELECT: Spin Again | DOWN: Main Menu");
  } else {
    tft.setTextColor(COLOR_RED, COLOR_BG);
    tft.print("NO WIN THIS TIME");
    tft.setTextSize(1);
    tft.setTextColor(COLOR_WHITE, COLOR_BG);
    tft.setCursor(30, 165);
    tft.printf("Lost Stake: -$%d credits", BET_STAKES[currentStakeIdx]);
    tft.setCursor(30, 192);
    tft.setTextColor(COLOR_CYAN, COLOR_BG);
    tft.print("SELECT: Spin Again | DOWN: Main Menu");
  }
  // Refresh Credit Badge in Header
  drawHeader("CYBER 777 SLOTS");
}
void handleSlotsResult() {
  if (btnSel.pressed) {
    soundSelect();
    enterSlotsBet();
  }
  if (btnDown.pressed || btnLeft.pressed) {
    soundButton();
    enterMainMenu();
  }
}
// ----------------------------------------------------------------------------
// STATE: SETTINGS & RESET
// ----------------------------------------------------------------------------
void renderSettingsUI() {
  drawHeader("SETTINGS & CREDITS");
  drawFooter("UP/DOWN: Option | SELECT: Toggle/Apply");
  tft.fillRoundRect(20, 42, 280, 165, 8, COLOR_PANEL);
  tft.drawRoundRect(20, 42, 280, 165, 8, COLOR_CYAN);
  // Option 0: Sound Effects
  bool sel0 = (settingsSelection == 0);
  tft.fillRoundRect(30, 54, 260, 40, 6, sel0 ? COLOR_BG : COLOR_PANEL);
  if (sel0) tft.drawRoundRect(30, 54, 260, 40, 6, COLOR_GOLD);
  tft.setTextSize(2);
  tft.setTextColor(sel0 ? COLOR_YELLOW : COLOR_WHITE, sel0 ? COLOR_BG : COLOR_PANEL);
  tft.setCursor(40, 64);
  tft.print("Sound: ");
  tft.setTextColor(soundEnabled ? COLOR_GREEN : COLOR_RED, sel0 ? COLOR_BG : COLOR_PANEL);
  tft.print(soundEnabled ? "ON" : "OFF");
  // Option 1: Reset Credits to 1000
  bool sel1 = (settingsSelection == 1);
  tft.fillRoundRect(30, 104, 260, 40, 6, sel1 ? COLOR_BG : COLOR_PANEL);
  if (sel1) tft.drawRoundRect(30, 104, 260, 40, 6, COLOR_GOLD);
  tft.setTextSize(2);
  tft.setTextColor(sel1 ? COLOR_YELLOW : COLOR_WHITE, sel1 ? COLOR_BG : COLOR_PANEL);
  tft.setCursor(40, 114);
  tft.print("Reset Balance");
  tft.setTextSize(1);
  tft.setCursor(185, 120);
  tft.setTextColor(COLOR_CYAN, sel1 ? COLOR_BG : COLOR_PANEL);
  tft.print("($1000)");
  // Option 2: Exit to Main Menu
  bool sel2 = (settingsSelection == 2);
  tft.fillRoundRect(30, 154, 260, 40, 6, sel2 ? COLOR_BG : COLOR_PANEL);
  if (sel2) tft.drawRoundRect(30, 154, 260, 40, 6, COLOR_GOLD);
  tft.setTextSize(2);
  tft.setTextColor(sel2 ? COLOR_YELLOW : COLOR_WHITE, sel2 ? COLOR_BG : COLOR_PANEL);
  tft.setCursor(40, 164);
  tft.print("< Back to Menu");
}
void enterSettings() {
  currentState = STATE_SETTINGS;
  settingsSelection = 0;
  tft.fillScreen(COLOR_BG);
  renderSettingsUI();
  Serial.println("[STATE] Entered Settings Menu");
}
void handleSettings() {
  bool redraw = false;
  if (btnUp.pressed) {
    soundTick();
    settingsSelection = (settingsSelection - 1 + 3) % 3;
    redraw = true;
  }
  if (btnDown.pressed) {
    soundTick();
    settingsSelection = (settingsSelection + 1) % 3;
    redraw = true;
  }
  if (btnSel.pressed) {
    soundSelect();
    if (settingsSelection == 0) {
      soundEnabled = !soundEnabled;
      redraw = true;
    } else if (settingsSelection == 1) {
      credits = 1000;
      soundWin();
      tft.fillRect(40, 110, 240, 35, COLOR_GREEN);
      tft.setTextSize(2);
      tft.setTextColor(COLOR_WHITE, COLOR_GREEN);
      tft.setCursor(55, 120);
      tft.print("CREDITS RESET!");
      delay(700);
      redraw = true;
    } else if (settingsSelection == 2) {
      enterMainMenu();
      return;
    }
  }
  if (redraw) {
    renderSettingsUI();
  }
}
// ----------------------------------------------------------------------------
// ARDUINO SETUP & MAIN LOOP
// ----------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("=== MINI CASINO ARCADE INITIALIZING ===");
  // Initialize Buttons with Internal Pull-ups
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);
  pinMode(BTN_SEL, INPUT_PULLUP);
  // Initialize Buzzer
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  // Initialize NeoPixel
  rgbLed.begin();
  rgbLed.setBrightness(40);
  setLedColor(0, 0, 0);
  // Configure hardware SPI bus without claiming CS pin (Adafruit manages CS in software)
  SPI.begin(TFT_SCK, TFT_MISO, TFT_MOSI);
  // Initialize Display
  tft.begin();
  tft.setRotation(1); // Landscape (320 wide x 240 tall)
  tft.fillScreen(COLOR_BG);
  Serial.println("Display initialized successfully!");
  // Launch Splash Screen
  enterSplash();
}
void loop() {
  // 1. Scan Inputs
  scanInputs();
  // 2. Audio State Update
  updateAudio();
  // 3. Finite State Machine Dispatch
  switch (currentState) {
    case STATE_SPLASH:
      handleSplash();
      break;
    case STATE_MAIN_MENU:
      handleMainMenu();
      break;
    case STATE_ROULETTE_BET:
      handleRouletteBet();
      break;
    case STATE_ROULETTE_SPIN:
      handleRouletteSpin();
      break;
    case STATE_ROULETTE_RESULT:
      handleRouletteResult();
      break;
    case STATE_SLOTS_BET:
      handleSlotsBet();
      break;
    case STATE_SLOTS_SPIN:
      handleSlotsSpin();
      break;
    case STATE_SLOTS_RESULT:
      handleSlotsResult();
      break;
    case STATE_SETTINGS:
      handleSettings();
      break;
  }
  // Maintain responsiveness without blocking delays
  delay(10);
}