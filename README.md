# Mini Casino Arcade (ESP32 + 3.2" TFT)

A do-it-yourself handheld retro casino game console built around an ESP32 DevKit and a 3.2 inch or 3.3 inch ILI9341 SPI color screen.

The console runs two casino games with shared virtual credits, sound effects and RGB lighting feedback:
- **European Roulette (0–36)**: 37-pocket wheel layout, single-number and outside bets (Red/Black, Odd/Even, High/Low), ball physics animation with deceleration and standard casino odds.
- **Cyber 777 Slots**: 3-reel slot machine with weighted RNG, staged reel stops for suspense, winning payline detection and celebratory win animations.
- **Credits**: Shared persistent bankroll across games, sound toggle and credit reset.

Designed to be built on a breadboard or perfboard using through-hole parts and jumper wires.

---

## Photos

| Circuit Layout & Controls | Boot Splash Screen | Casino Lobby Menu |
|:---:|:---:|:---:|
| ![Mini Arcade Simulation Setup](images/simulation_setup.jpg) | ![Splash Screen](images/splash_screen.jpg) | ![Casino Lobby Gameplay](images/gameplay.jpg) |
| *ESP32, ILI9341 3.2" TFT, 5x buttons, buzzer & NeoPixel* | *Animated retro neon splash screen* | *Interactive game selection and shared credit bankroll* |

---

## Hardware & Parts List

- ESP32 Development Board (30-pin or 38-pin DevKit V1 / WROOM-32)
- 3.2 inch or 3.3 inch TFT LCD Display (ILI9341 driver, 240x320 resolution, SPI interface)
- 5x Tactile Pushbuttons (UP, DOWN, LEFT, RIGHT, SELECT / PLAY)
- 1x Passive Piezo Buzzer (for 8-bit effects and reel clicks)
- 1x WS2812B NeoPixel RGB LED (status and jackpot flash indicator)

**Power Source:**
- Standard Micro-USB / USB-C cable from a 5V power bank or PC, OR
- 1S 3.7V LiPo battery with a TP4056 charging module and power switch
- Breadboard, jumper wires and 0.1µF decoupling capacitors

---

## Schematic & Wiring

The full electrical schematic is created in KiCad. Included in this repository:
- KiCad Schematic source: [`mini_arcade.kicad_sch`](mini_arcade.kicad_sch)
- Printable PDF schematic: [`mini_arcade_schematic.pdf`](mini_arcade_schematic.pdf)

![Mini Arcade Circuit Schematic](images/schematic.svg)

### Wiring Table

| ESP32 GPIO | Connected Component | Function / Notes |
|:---|:---|:---|
| **GPIO 5** | TFT CS | SPI Chip Select |
| **GPIO 21** | TFT DC (or RS) | Data / Command Select |
| **GPIO 22** | TFT RST | Hardware Reset |
| **GPIO 23** | TFT MOSI (or SDI) | Hardware SPI Master Out |
| **GPIO 18** | TFT SCK (or CLK) | Hardware SPI Clock |
| **GPIO 19** | TFT MISO (or SDO) | Hardware SPI Master In |
| **GPIO 27** | Button UP | Active-LOW internal pull-up to GND |
| **GPIO 14** | Button DOWN | Active-LOW internal pull-up to GND |
| **GPIO 26** | Button LEFT | Active-LOW internal pull-up to GND |
| **GPIO 25** | Button RIGHT | Active-LOW internal pull-up to GND |
| **GPIO 33** | Button SELECT | Active-LOW internal pull-up to GND |
| **GPIO 32** | Passive Buzzer (+) | PWM square-wave audio (negative pin to GND) |
| **GPIO 13** | WS2812B DIN | Addressable RGB data input |
| **3.3V** | TFT VCC & LED | Display logic and backlight supply |
| **GND** | All Common Grounds | Shared system ground |

> **Wiring tip**: Buttons use internal pull-up resistors (`INPUT_PULLUP`) so simply wire one leg of each button to its assigned GPIO and the opposite diagonal leg directly to GND. No external pull-up resistors needed.

---

## Firmware Setup (Arduino IDE)

The main Arduino firmware file is [`mini_arcade.ino`](mini_arcade.ino) (or [`sketch.ino`](sketch.ino)).

1. Install Arduino IDE (version 2.x recommended).
2. Add ESP32 board support via **Preferences -> Additional Boards Manager URLs**:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
3. Open Library Manager (`Ctrl+Shift+I`) and install:
   - `Adafruit GFX Library`
   - `Adafruit ILI9341`
   - `Adafruit NeoPixel`
4. Connect your ESP32 via USB. Select **ESP32 Dev Module** under **Tools -> Board**.
5. Select the matching COM port. Click **Upload**.

---

## Playing in Wokwi Simulator

If you want to test the firmware before wiring everything:
1. Open [Wokwi.com](https://wokwi.com)
2. Create an ESP32 project.
3. Copy [`diagram.json`](diagram.json) into the simulation tab.
4. Copy [`sketch.ino`](sketch.ino) into the code tab.
5. Add the libraries to `libraries.txt`:
   ```
   Adafruit GFX Library
   Adafruit ILI9341
   Adafruit NeoPixel
   ```
6. Click **Play / Start Simulation**. Use the on-screen buttons to play.

---

## Rules

### Main Lobby
- **UP / DOWN**: Highlight a game card (Roulette, Slot Machine or Settings).
- **SELECT**: Launch the option.

### European Roulette
- **UP / DOWN**: Cycle bet types (Straight Number, Red, Black, Even, Odd, Low 1–18, High 19–36 or `< Back to Lobby>`).
- **LEFT / RIGHT**:
  - When on *Straight Number*: Scrolls the pocket number from 0 to 36.
  - When on *Outside Bets*: Adjusts your wager stake ($10, $25, $50, $100).
- **SELECT**: Lock in bet. Trigger the wheel spin.
- The ball animates around the wheel with deceleration and pocket ticks. Results are generated using the ESP32 hardware random number generator (`esp_random()`).

### Cyber 777 Slots
- **LEFT / RIGHT**: Adjust your wager amount ($10, $25, $50, $100).
- **SELECT**: Spin the reels.
- **UP or DOWN**: Return to the Main Lobby.
- Reels stop one by one (Reel 1 at 1.0s, Reel 2 at 1.8s, Reel 3 at 2.6s).
- **Paytable**:
  - `7 - 7 - 7`: 100x Jackpot
  - `Diamond Trio`: 50x
  - `Star Trifecta`: 25x
  - `Triple Bells`: 15x
  - `Triple Cherries`: 10x
  - `2x Cherries`: 3x
  - `1x Cherry`: 1x (Stake returned)
