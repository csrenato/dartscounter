#include "Adafruit_NeoPixel.h"
#include "WS2812_Definitions.h"
#include "LedControlMS.h"
#include "mpr121.h"
#include "i2c.h"

//12 DIN // 11 CLK // 10 CS on display

// D5 IRQ // A4 SDA // A5 SCL on keyboard

LedControl lc = LedControl(12, 11, 10, 2);

unsigned long delaytime = 250; // we always wait a bit between updates of the display

// Keypad type definition - Keypad Mapping

#define MPR121_R 0xB5 // ADD pin is grounded
#define MPR121_W 0xB4 // So address is 0x5A

// Match key inputs with electrode numbers
#define STAR 0
#define SEVEN 1
#define FOUR 2
#define ONE 3
#define ZERO 4
#define EIGHT 5
#define FIVE 6
#define TWO 7
#define POUND 8
#define NINE 9
#define SIX 10
#define THREE 11
#define PIN 4 // Pin used for Neo pixel <-> Arduino communication
#define LED_COUNT 3 // Number of leds we use

Adafruit_NeoPixel leds = Adafruit_NeoPixel(LED_COUNT, PIN, NEO_GRB + NEO_KHZ800); // Define the leds we use (Neo pixel WS2812 breakout)

uint16_t touchstatus;

int irqpin = 5; // D5
int speakerPin = 9; // Speaker connected to D9
int length1 = 7; // the number of notes
char notes[] = "CgcfeC "; // a space represents a rest // start sound (when turning on power)
int beats[] = {
  1,
  1,
  1,
  1,
  1,
  2,
  1
};
char notes2[] = "CefcgC "; // a space represents a rest // game over sound
int beats2[] = {
  1,
  1,
  1,
  1,
  2,
  1,
  1
};
int tempo = 100; // Temp for playing the sounds on the piezzo speaker
int i = 0;
int j = 0;
int gameNumber = 0; // We store and compare this one to the defined games below
int game501 = 7; // We compare these in the Choose game section
int gameCricket = 9; // We compare these in the Choose game section
int number[3] = {
  0,
  0,
  0
}; // Array - each keystroke on the keypad is saved in coresponding array number
int numberSum = 0; // Integer in which the array number is saved and deducted from total points of the player
float br1 = 0; // Help with number calculation -> We have to seperate the number into factors so we can print them on the displays
int temp = 0;
int temp2 = 0;
int temp3 = 0;
int dec1Power100 = 0; // Stores the 3rd digit of the number we input/output over the display
int dec1Power10 = 0; // Stores the 2rd digit of the number we input/output over the display
int dec1Power1 = 0; // Stores the 1st digit of the number we input/output over the display
int dec2Power100 = 0; // Stores the 3rd digit of the number we input/output over the display
int dec2Power10 = 0; // Stores the 2rd digit of the number we input/output over the display
int dec2Power1 = 0; // Stores the 1st digit of the number we input/output over the display
int breakValue = 0; // Used to break out of the 501 game when a player wins
int tempScore1 = 0; // Stores 0 or 1 and is used for while break
int tempScore = 0; // Stores temp score and then we deduct that score from player score
int player1 = 0; // Stores player 1 score
int player2 = 0; // Stores player 2 score
int player1Temp = 0; // Temp for number calculation (when we print the scores on the displays)
int player2Temp = 0; // Temp for number calculation (when we print the scores on the displays)
int legs1 = 0; // Stores won legs
int legs2 = 0; // Stores won legs
int k1 = 0; // Controls what score we print of the other player while
int k2 = 0; // Controls what score we print of the other player while
int firstTurn = 0; // Stores wich player goes first
int l = 0; // Used to track where to shift the array in wich we input the numbers from the keypad for player scoring
int ledBrightness = 64; // 64 optimal //24 for testing (0-255)
int displayBrightness = 8; // 8 optimal // 3 for testing (0-16)
int turn = 0; // Used to decide if to play the next player tone or not
				// (if a player wins we don/t play a tone while pressing next player -> POUND button)
int key; // Stores the key input from the keyboard

////////////////////////////////////////////////////////////////////////////////
byte mpr121Read(uint8_t address) {

  byte data;

  i2cSendStart();
  i2cWaitForComplete();

  i2cSendByte(MPR121_W); // write 0xB4
  i2cWaitForComplete();

  i2cSendByte(address); // write register address
  i2cWaitForComplete();

  i2cSendStart();

  i2cSendByte(MPR121_R); // write 0xB5
  i2cWaitForComplete();
  i2cReceiveByte(TRUE);
  i2cWaitForComplete();

  data = i2cGetReceivedByte(); // Get MSB result
  i2cWaitForComplete();
  i2cSendStop();

  cbi(TWCR, TWEN); // Disable TWI
  sbi(TWCR, TWEN); // Enable TWI

  return data;
}

void mpr121Write(unsigned char address, unsigned char data) {

  i2cSendStart();
  i2cWaitForComplete();

  i2cSendByte(MPR121_W); // write 0xB4
  i2cWaitForComplete();

  i2cSendByte(address); // write register address
  i2cWaitForComplete();

  i2cSendByte(data);
  i2cWaitForComplete();

  i2cSendStop();
}

void mpr121QuickConfig(void) {

    // Section A
    // This group controls filtering when data is > baseline.
    mpr121Write(MHD_R, 0x01);
    mpr121Write(NHD_R, 0x01);
    mpr121Write(NCL_R, 0x00);
    mpr121Write(FDL_R, 0x00);

    // Section B
    // This group controls filtering when data is < baseline.
    mpr121Write(MHD_F, 0x01);
    mpr121Write(NHD_F, 0x01);
    mpr121Write(NCL_F, 0xFF);
    mpr121Write(FDL_F, 0x02);

    // Section C
    // This group sets touch and release thresholds for each electrode
    mpr121Write(ELE0_T, TOU_THRESH);
    mpr121Write(ELE0_R, REL_THRESH);
    mpr121Write(ELE1_T, TOU_THRESH);
    mpr121Write(ELE1_R, REL_THRESH);
    mpr121Write(ELE2_T, TOU_THRESH);
    mpr121Write(ELE2_R, REL_THRESH);
    mpr121Write(ELE3_T, TOU_THRESH);
    mpr121Write(ELE3_R, REL_THRESH);
    mpr121Write(ELE4_T, TOU_THRESH);
    mpr121Write(ELE4_R, REL_THRESH);
    mpr121Write(ELE5_T, TOU_THRESH);
    mpr121Write(ELE5_R, REL_THRESH);
    mpr121Write(ELE6_T, TOU_THRESH);
    mpr121Write(ELE6_R, REL_THRESH);
    mpr121Write(ELE7_T, TOU_THRESH);
    mpr121Write(ELE7_R, REL_THRESH);
    mpr121Write(ELE8_T, TOU_THRESH);
    mpr121Write(ELE8_R, REL_THRESH);
    mpr121Write(ELE9_T, TOU_THRESH);
    mpr121Write(ELE9_R, REL_THRESH);
    mpr121Write(ELE10_T, TOU_THRESH);
    mpr121Write(ELE10_R, REL_THRESH);
    mpr121Write(ELE11_T, TOU_THRESH);
    mpr121Write(ELE11_R, REL_THRESH);

    // Section D
    // Set the Filter Configuration
    // Set ESI2
    mpr121Write(FIL_CFG, 0x04);

    // Section E
    // Electrode Configuration
    // Enable 6 Electrodes and set to run mode
    // Set ELE_CFG to 0x00 to return to standby mode
    mpr121Write(ELE_CFG, 0x0C);
  } // Enables all 12 Electrodes
  //mpr121Write(ELE_CFG, 0x06);		// Enable first 6 electrodes

// Section F
// Enable Auto Config and auto Reconfig
/*mpr121Write(ATO_CFG0, 0x0B);
   mpr121Write(ATO_CFGU, 0xC9);	// USL = (Vdd-0.7)/vdd*256 = 0xC9 @3.3V   
	 mpr121Write(ATO_CFGL, 0x82);	// LSL = 0.65*USL = 0x82 @3.3V
   mpr121Write(ATO_CFGT, 0xB5);*/ // Target = 0.9*USL = 0xB5 @3.3V

byte checkInterrupt(void)

{
  if (digitalRead(irqpin))
    return 1;
  return 0;
}

////////////////////////////////////////////////////////////////////////////////

void loading1() {

  //TURN OFF ALL LEDS
  clearLEDs();
  leds.setPixelColor(0, BLACK);
  leds.setPixelColor(1, BLACK);
  leds.setPixelColor(2, BLACK);
  leds.show();

  //Print out Loading Software on the displays
  lc.clearDisplay(0);
  lc.clearDisplay(1);

  // LOADING
  lc.setChar(0, 1, '9', false);
  lc.setLed(0, 2, 3, true);
  lc.setLed(0, 2, 5, true);
  lc.setLed(0, 2, 7, true);
  lc.setChar(0, 3, '1', false);
  lc.setChar(0, 4, 'D', false);
  lc.setChar(0, 5, 'A', false);
  lc.setChar(0, 6, '0', false);
  lc.setChar(0, 7, 'L', false);

  //SOFTWARE
  lc.setChar(1, 0, 'E', false);
  lc.setLed(1, 1, 5, true);
  lc.setLed(1, 1, 7, true);
  lc.setChar(1, 2, 'A', false);
  lc.setLed(1, 3, 3, true);
  lc.setLed(1, 3, 4, true);
  lc.setLed(1, 3, 5, true);
  lc.setLed(1, 4, 4, true);
  lc.setLed(1, 4, 5, true);
  lc.setLed(1, 4, 6, true);
  lc.setLed(1, 4, 7, true);
  lc.setChar(1, 5, 'F', false);
  lc.setChar(1, 6, '0', false);
  lc.setChar(1, 7, '5', false);
}

void loading1Leds() { // Leds light up blue and go out to black in 1 seccond patterns // We repeat this so the keyboard has enough time to initialize

  for (int i = 255; i > 0; i--) {

    leds.setPixelColor(0, (0, 0, i));
    leds.setPixelColor(1, (0, 0, i));
    leds.setPixelColor(2, (0, 0, i));
    leds.show();
    delay(2);
  }

  for (int i = 0; i < 255; i++) {

    leds.setPixelColor(0, (0, 0, i));
    leds.setPixelColor(1, (0, 0, i));
    leds.setPixelColor(2, (0, 0, i));
    leds.show();
    delay(2);
  }
}

int gameModeText() { // As the loading finishes we display this on the screens

  //				 _       _       _   _
  //				∣_∣ ∣   ∣_∣ ∣_∣ ∣_  ∣_∣       ∣
  //				∣   ∣_  ∣ ∣   ∣ ∣_  ∣ ∣       ∣
  //				 _       _       _   _       _
  //				∣_∣ ∣   ∣_∣ ∣_∣ ∣_  ∣_∣      _∣
  //				∣   ∣_  ∣ ∣   ∣ ∣_  ∣ ∣     ∣_

  // We turn on the player indicator leds into blue
  clearLEDs();
  leds.setPixelColor(0, BLUE);
  leds.setPixelColor(1, BLUE);
  leds.show();

  // Player 1
  lc.clearDisplay(0);
  lc.setChar(0, 0, '1', false);
  lc.setChar(0, 2, 'A', false);
  lc.setChar(0, 3, 'E', false);
  lc.setChar(0, 4, '4', false);
  lc.setChar(0, 5, 'A', false);
  lc.setChar(0, 6, 'L', false);
  lc.setChar(0, 7, 'P', false);

  // Player 2
  lc.clearDisplay(1);
  lc.setChar(1, 0, '2', false);
  lc.setChar(1, 2, 'A', false);
  lc.setChar(1, 3, 'E', false);
  lc.setChar(1, 4, '4', false);
  lc.setChar(1, 5, 'A', false);
  lc.setChar(1, 6, 'L', false);
  lc.setChar(1, 7, 'P', false);
}

void playTone(int tone, int duration) {

  for (long i = 0; i < duration * 1000 L; i += tone * 2) {
    digitalWrite(speakerPin, HIGH);
    delayMicroseconds(tone);
    digitalWrite(speakerPin, LOW);
    delayMicroseconds(tone);
  }
}

void playNote(char note, int duration) {

  char names[] = {
    'c',
    'd',
    'e',
    'f',
    'g',
    'a',
    'b',
    'C'
  };
  int tones[] = {
    1915,
    1700,
    1519,
    1432,
    1275,
    1136,
    1014,
    956
  };

  // play the tone corresponding to the note name
  for (int i = 0; i < 8; i++) {
    if (names[i] == note) {
      playTone(tones[i], duration);
    }
  }
}

int gameMode() {

  int touchNumber;

  while (true) {

    gameNumber = 0;

    while (checkInterrupt());

    touchNumber = 0;

    touchstatus = mpr121Read(0x01) << 8;
    touchstatus |= mpr121Read(0x00);

    for (int j = 0; j < 12; j++) {

      if ((touchstatus & (2 << j))) {

        touchNumber++;
        delay(10);
      }
    }

    if (touchNumber == 1) {

      if (touchstatus & (1 << NINE)) {

        gameNumber = 9;
        leds.setPixelColor(2, GREEN);
        leds.show();
        delay(10);
        leds.setPixelColor(2, 0);
        leds.show();
      }

      if (touchstatus & (1 << SEVEN)) {

        gameNumber = 7;
        leds.setPixelColor(2, GREEN);
        leds.show();
        delay(10);
        leds.setPixelColor(2, 0);
        leds.show();
      }
    } else if (touchNumber == 0) {

      leds.setPixelColor(2, ORANGE);
      leds.show();
      delay(50);
      leds.setPixelColor(2, 0);
      leds.show();
      continue;
    }

    if (game501 == gameNumber) {

      player1 = 501;
      player2 = player1;
      break;
    }

    if (gameCricket == gameNumber) {

      player1 = 666;
      player2 = player1;
      break;
    }
  }
}

int readKeyboard1() {

  int touchNumber;
  i = 0;
  l = 0;

  while (true) {

    while (checkInterrupt())
    ;
    touchNumber = 0;

    touchstatus = mpr121Read(0x01) << 8;
    touchstatus |= mpr121Read(0x00);

    for (int j = 0; j < 12; j++) {

      if ((touchstatus & (1 << j))) {
        touchNumber++;
        delay(10);
      }
    }

    if (touchNumber == 1) {

      if (touchstatus & (1 << STAR)) {

        delay(beats[0] * tempo);
        playNote(notes[0], beats[0] * tempo);

        for (i = 0; i < 3; i++) {
          number[i] = 0;
        }

        l = 0;
        key = -6;

        leds.setPixelColor(2, RED);

        leds.show();
        delay(50);
        leds.setPixelColor(2, 0);
        leds.show();

        continue;
      } else if (touchstatus & (1 << POUND)) {

        key = -13;
        numberSum = number[0] * 100 + number[1] * 10 + number[2];
        tempScore = numberSum;

        if ((turn == 1) && ((player1 - tempScore) != 0)) {

          playNote(notes[0], beats[0] * tempo);
          delay(beats[1] * tempo);
          playNote(notes[1], beats[1] * tempo);
        } else if ((turn == 2) && ((player2 - tempScore) != 0)) {

          playNote(notes[0], beats[0] * tempo);
          delay(beats[1] * tempo);
          playNote(notes[1], beats[1] * tempo);
        }

        for (i = 0; i < 3; i++) {

          number[i] = 0;
        }

        numberSum = 0;
        tempScore1 = 1;
        l = -2;

        leds.setPixelColor(2, GREEN);
        leds.show();
        delay(50);
        leds.setPixelColor(2, 0);
        leds.show();

        break;
      } else if (touchstatus & (1 << SEVEN)) {

        leds.setPixelColor(2, BLUE);
        leds.show();
        delay(50);
        leds.setPixelColor(2, 0);
        leds.show();
        key = 7;
      } else if (touchstatus & (1 << FOUR)) {

        leds.setPixelColor(2, BLUE);
        leds.show();
        delay(50);
        leds.setPixelColor(2, 0);
        leds.show();
        key = 4;
      } else if (touchstatus & (1 << ONE)) {

        leds.setPixelColor(2, BLUE);
        leds.show();
        delay(50);
        leds.setPixelColor(2, 0);
        leds.show();
        key = 1;
      } else if (touchstatus & (1 << ZERO)) {

        leds.setPixelColor(2, BLUE);
        leds.show();
        delay(50);
        leds.setPixelColor(2, 0);
        leds.show();
        key = 0;
      } else if (touchstatus & (1 << EIGHT)) {

        leds.setPixelColor(2, BLUE);
        leds.show();
        delay(50);
        leds.setPixelColor(2, 0);
        leds.show();
        key = 8;
      } else if (touchstatus & (1 << FIVE)) {

        leds.setPixelColor(2, BLUE);
        leds.show();
        delay(50);
        leds.setPixelColor(2, 0);
        leds.show();
        key = 5;
      } else if (touchstatus & (1 << TWO)) {

        leds.setPixelColor(2, BLUE);
        leds.show();
        delay(50);
        leds.setPixelColor(2, 0);
        leds.show();
        key = 2;
      } else if (touchstatus & (1 << NINE)) {

        leds.setPixelColor(2, BLUE);
        leds.show();
        delay(50);
        leds.setPixelColor(2, 0);
        leds.show();
        key = 9;
      } else if (touchstatus & (1 << SIX)) {

        leds.setPixelColor(2, BLUE);
        leds.show();
        delay(50);
        leds.setPixelColor(2, 0);
        leds.show();
        key = 6;
      } else if (touchstatus & (1 << THREE)) {

        leds.setPixelColor(2, BLUE);
        leds.show();
        delay(50);
        leds.setPixelColor(2, 0);
        leds.show();
        key = 3;
      }
    } else
      continue;

    if ((l == 2) && (key != -6) && (key != -13)) {

      number[0] = number[1];
      number[1] = number[2];
      number[2] = key;
      l++;
    }

    if ((l == 1) && (key != -6) && (key != -13)) {

      number[1] = number[2];
      number[2] = key;
      l++;
    }

    if ((l == 0) && (key != -6) && (key != -13)) {

      number[2] = key;
      l++;
    }
  }
}

void clearLEDs() {

  for (int i = 0; i < LED_COUNT; i++) {

    leds.setPixelColor(i, 0);
  }
}

void printDisplay1() {

  lc.clearDisplay(0);
  br1 = player1 / 100;
  dec1Power100 = br1;
  temp = player1 - dec1Power100 * 100;
  br1 = temp / 10;
  dec1Power10 = br1;
  dec1Power1 = player1 - dec1Power100 * 100 - dec1Power10 * 10;

  if (dec1Power100 > 0) {
    lc.setDigit(0, 0, dec1Power1, false);
    lc.setDigit(0, 1, dec1Power10, false);
    lc.setDigit(0, 2, dec1Power100, false);
  }

  if ((dec1Power100 == 0) && (dec1Power10 > 0)) {

    lc.setDigit(0, 0, dec1Power1, false);
    lc.setDigit(0, 1, dec1Power10, false);
  } else {

    lc.setDigit(0, 0, dec1Power1, false);
  }

  dec1Power10 = legs1 / 10;
  dec1Power1 = legs1 - dec1Power10 * 10;
  lc.setChar(0, 4, dec1Power1, false);
  lc.setChar(0, 5, dec1Power10, false);
  lc.setChar(0, 6, '-', false);
  lc.setChar(0, 7, 'L', false);

}

void printDisplay2() {

  lc.clearDisplay(1);
  br1 = player2 / 100;
  dec2Power100 = br1;
  temp = player2 - dec2Power100 * 100;
  br1 = temp / 10;
  dec2Power10 = br1;
  dec2Power1 = player2 - dec2Power100 * 100 - dec2Power10 * 10;

  if (dec2Power100 > 0) {
    lc.setDigit(1, 0, dec2Power1, false);
    lc.setDigit(1, 1, dec2Power10, false);
    lc.setDigit(1, 2, dec2Power100, false);
  }

  if ((dec2Power100 == 0) && (dec2Power10 > 0)) {

    lc.setDigit(1, 0, dec2Power1, false);
    lc.setDigit(1, 1, dec2Power10, false);
  } else {

    lc.setDigit(1, 0, dec2Power1, false);
  }

  dec2Power10 = legs2 / 10;
  dec2Power1 = legs2 - dec2Power10 * 10;
  lc.setChar(1, 4, dec2Power1, false);
  lc.setChar(1, 5, dec2Power10, false);
  lc.setChar(1, 6, '-', false);
  lc.setChar(1, 7, 'L', false);
}

void printPlayer1LessThan40() {

  lc.clearDisplay(0);
  player1Temp = player1 / 2;
  br1 = player1Temp / 100;
  dec1Power100 = br1;
  temp = player1Temp - dec1Power100 * 100;
  br1 = temp / 10;
  dec1Power10 = br1;
  dec1Power1 = player1Temp - dec1Power100 * 100 - dec1Power10 * 10;

  if (dec1Power10 == 0) {

    lc.setDigit(0, 0, dec1Power1, false);
    lc.setChar(0, 3, 'D', false);
  } else {

    lc.setDigit(0, 0, dec1Power1, false);
    lc.setDigit(0, 1, dec1Power10, false);
    lc.setChar(0, 3, 'D', false);
  }

  dec1Power10 = legs1 / 10;
  dec1Power1 = legs1 - dec1Power10 * 10;
  lc.setChar(0, 4, dec1Power1, false);
  lc.setChar(0, 5, dec1Power10, false);
  lc.setChar(0, 6, '-', false);
  lc.setChar(0, 7, 'L', false);
}

void printPlayer2LessThan40() {

  lc.clearDisplay(1);
  player2Temp = player2 / 2;
  br1 = player2Temp / 100;
  dec2Power100 = br1;
  temp = player2Temp - dec2Power100 * 100;
  br1 = temp / 10;
  dec2Power10 = br1;
  dec2Power1 = player2Temp - dec2Power100 * 100 - dec2Power10 * 10;

  if (dec2Power10 == 0) {

    lc.setDigit(1, 0, dec2Power1, false);
    lc.setChar(1, 3, 'D', false);
  } else {

    lc.setDigit(1, 0, dec2Power1, false);
    lc.setDigit(1, 1, dec2Power10, false);
    lc.setChar(1, 3, 'D', false);
  }

  dec2Power10 = legs2 / 10;
  dec2Power1 = legs2 - dec2Power10 * 10;
  lc.setChar(1, 4, dec2Power1, false);
  lc.setChar(1, 5, dec2Power10, false);
  lc.setChar(1, 6, '-', false);
  lc.setChar(1, 7, 'L', false);
}

void printBULLPlayer1() {

  lc.clearDisplay(0);
  dec1Power10 = legs1 / 10;
  dec1Power1 = legs1 - dec1Power10 * 10;

  lc.setChar(0, 0, 'L', false);
  lc.setChar(0, 1, 'L', false);
  lc.setLed(0, 2, 2, true);
  lc.setLed(0, 2, 3, true);
  lc.setLed(0, 2, 4, true);
  lc.setLed(0, 2, 5, true);
  lc.setLed(0, 2, 6, true);
  lc.setChar(0, 3, 'B', false);
  lc.setChar(0, 4, dec1Power1, false);
  lc.setChar(0, 5, dec1Power10, false);
  lc.setChar(0, 6, '-', false);
  lc.setChar(0, 7, 'L', false);

}

void printBULLPlayer2() {

  lc.clearDisplay(1);
  dec2Power10 = legs2 / 10;
  dec2Power1 = legs2 - dec2Power10 * 10;

  lc.setChar(1, 0, 'L', false);
  lc.setChar(1, 1, 'L', false);
  lc.setLed(1, 2, 2, true);
  lc.setLed(1, 2, 3, true);
  lc.setLed(1, 2, 4, true);
  lc.setLed(1, 2, 5, true);
  lc.setLed(1, 2, 6, true);
  lc.setChar(1, 3, 'B', false);
  lc.setChar(1, 4, dec2Power1, false);
  lc.setChar(1, 5, dec2Power10, false);
  lc.setChar(1, 6, '-', false);
  lc.setChar(1, 7, 'L', false);
}

void player1ScoreAndDisplay() {

  clearLEDs();
  leds.setPixelColor(0, GREEN);
  leds.setPixelColor(1, RED);
  leds.show();

  if (player1 == 50) {

    printBULLPlayer1();
  } else if (player1 <= 40) {

    if (player1 % 2 == 0) {

      printPlayer1LessThan40();
    } else {

      printDisplay1();
    }
  } else if (player1 > 50) {

    printDisplay1();
  } else {

    printDisplay1();
  }

  while (tempScore1 == 0) {

    readKeyboard1();
  }

  tempScore1 = 0;
  temp = player1 - tempScore;

  if (temp > 1) {

    player1 = player1 - tempScore;
    tempScore = 0;
  } else if (player1 == tempScore) {

    player1 = player1 - tempScore;
    tempScore = 0;
  }

  tempScore = 0;

  if (player1 == 50) {

    printBULLPlayer1();
  } else if (player1 <= 40) {

    if (player1 % 2 == 0) {

      printPlayer1LessThan40();
    } else {

      printDisplay1();
    }
  } else if (player1 > 50) {

    printDisplay1();
  } else {

    printDisplay1();
  }

  if (player1 == 0) {

    lc.clearDisplay(0);
    lc.clearDisplay(1);

    lc.setChar(0, 0, '5', false);
    lc.setLed(0, 1, 3, true);
    lc.setLed(0, 1, 7, true);
    lc.setLed(0, 1, 5, true);
    lc.setLed(0, 2, 3, true);
    lc.setLed(0, 3, 3, true);
    lc.setLed(0, 3, 4, true);
    lc.setLed(0, 3, 5, true);
    lc.setChar(0, 6, '1', false);
    lc.setChar(0, 7, 'P', false);

    lc.setChar(1, 0, 'E', false);
    lc.setChar(1, 1, '5', false);
    lc.setChar(1, 2, '0', false);
    lc.setChar(1, 3, 'L', false);
    lc.setChar(1, 6, '2', false);
    lc.setChar(1, 7, 'P', false);

    for (int i = 0; i < length1; i++) { // game over tone

      if (notes2[i] == ' ') {
        delay(beats2[i] * tempo); // rest
      } else {

        playNote(notes2[i], beats2[i] * tempo);
      }

      delay(tempo / 2); // pause between notes
    }

    legs1 = legs1 + 1;
    breakValue = 1;
  }
}

void player2ScoreAndDisplay() {

  clearLEDs();
  leds.setPixelColor(0, RED);
  leds.setPixelColor(1, GREEN);
  leds.show();

  if (player2 == 50) {

    printBULLPlayer2();
  } else if (player2 <= 40) {

    if (player2 % 2 == 0) {

      printPlayer2LessThan40();
    } else {

      printDisplay2(); // Prints Player 2 score
    }
  } else if (player2 > 50) {

    printDisplay2(); // Prints Player 1 score
  } else {

    printDisplay2(); // Prints Player 2 score
  }

  while (tempScore1 == 0) {

    readKeyboard1();
  }

  tempScore1 = 0;
  temp = player2 - tempScore;

  if (temp > 1) {

    player2 = player2 - tempScore;
    tempScore = 0;
  } else if (player2 == tempScore) {

    player2 = player2 - tempScore;

    tempScore = 0;
  }

  if (player2 == 50) {

    printBULLPlayer2();
  } else if (player2 <= 40) {

    if (player2 % 2 == 0) {

      printPlayer2LessThan40();
    } else {

      printDisplay2(); // Prints Player 2 score
    }
  } else if (player2 > 50) {

    printDisplay2(); // Prints Player 1 score
  } else {

    printDisplay2(); // Prints Player 2 score
  }

  if (player2 == 0) {

    lc.clearDisplay(0);
    lc.clearDisplay(1);

    lc.setChar(1, 0, '5', false);
    lc.setLed(1, 1, 3, true);
    lc.setLed(1, 1, 7, true);
    lc.setLed(1, 1, 5, true);
    lc.setLed(1, 2, 3, true);
    lc.setLed(1, 3, 3, true);
    lc.setLed(1, 3, 4, true);
    lc.setLed(1, 3, 5, true);
    lc.setChar(1, 6, '2', false);
    lc.setChar(1, 7, 'P', false);

    lc.setChar(0, 0, 'E', false);
    lc.setChar(0, 1, '5', false);
    lc.setChar(0, 2, '0', false);
    lc.setChar(0, 3, 'L', false);
    lc.setChar(0, 6, '1', false);
    lc.setChar(0, 7, 'P', false);

    // game over tone

    for (int i = 0; i < length1; i++) {

      if (notes2[i] == ' ') {

        delay(beats2[i] * tempo); // rest
      } else {

        playNote(notes2[i], beats2[i] * tempo);
      }

      delay(tempo / 2); // pause between notes
    }

    legs2 = legs2 + 1;
    breakValue = 1;

  }
}

////////////////////////////////////////////////////////////////////////////////

void setup() {

  pinMode(speakerPin, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(irqpin, INPUT);

  leds.begin();

  leds.setBrightness(ledBrightness); // 64 optimal //24 for testing (0-255)

  clearLEDs();
  leds.setPixelColor(0, BLUE);
  leds.setPixelColor(1, BLUE);
  leds.setPixelColor(2, BLUE);
  leds.show();

  playNote(notes[2], beats[2] * tempo); // Play On sound
  delay(50);

  digitalWrite(irqpin, HIGH);

  DDRC |= 0b00010011;
  PORTC = 0b00110000; // Pull-ups on I2C Bus
  i2cInit();

  delay(100);

  mpr121QuickConfig();

  lc.shutdown(0, false); // Turn on the displays
  lc.shutdown(1, false);

  lc.setIntensity(0, displayBrightness);
  lc.setIntensity(1, displayBrightness);

  lc.clearDisplay(0);
  lc.clearDisplay(1);

  loading1();

  for (j = 0; j < 6; j++) { // All 3 leds do a "breathing" animation while we wait
									// for the keypad to initialize (6 seconds + code before that)

    loading1Leds();

  }

  gameModeText();

  clearLEDs();
  leds.setPixelColor(0, BLUE);
  leds.setPixelColor(1, BLUE);
  leds.setPixelColor(2, BLACK);
  leds.show();

  for (int i = 0; i < length1; i++) { // start tone -> play the melody

    if (notes[i] == ' ') {

      delay(beats[i] * tempo); // rest
    } else {

      playNote(notes[i], beats[i] * tempo);
    }

    delay(tempo / 2); // pause between notes

  }

}

////////////////////////////////////////////////////////////////////////////////

void loop() {

    while (true) {

      gameMode();

      if (gameNumber != 7)
        if (gameNumber != 9)
          continue;

      breakValue = 0;

      lc.clearDisplay(0);
      lc.clearDisplay(1);

      printDisplay1();
      printDisplay2();

      k1 = 0;
      k2 = 0;

      while ((player1 > 0) && (player2 > 0)) {

        breakValue = 0;

        if (firstTurn == 0) {

          // PLAYER1

          turn = 1;

          player1ScoreAndDisplay();

          if (breakValue == 1) {

            if (firstTurn == 0) {

              firstTurn = 1;
            } else {
              firstTurn = 0;
            }

            break;
          }

          // PLAYER2

          turn = 2;

          player2ScoreAndDisplay();

          if (breakValue == 1) {

            if (firstTurn == 0) {

              firstTurn = 1;
            } else {

              firstTurn = 0;
            }

            break;
          }
        }

        if (firstTurn == 1) {

          // PLAYER2

          turn = 2;

          player2ScoreAndDisplay();

          if (breakValue == 1) {

            if (firstTurn == 0) {
              firstTurn = 1;
            } else {
              firstTurn = 0;
            }

            break;
          }

          // PLAYER1

          turn = 1;

          player1ScoreAndDisplay();

          if (breakValue == 1) {

            if (firstTurn == 0) {

              firstTurn = 1;
            } else {

              firstTurn = 0;
            }

            break;
          }

        }
      }
    } // while (true)
  } // void loop

// -> Game start -> Player 1
//		Player 1
//   Player 2
//   _       _       _   _
//  ∣_∣ ∣   ∣_∣ ∣_∣ ∣_  ∣_∣       ∣
//  ∣   ∣_  ∣ ∣   ∣ ∣_  ∣ ∣       ∣
//   _       _       _   _       _
//  ∣_∣ ∣   ∣_∣ ∣_∣ ∣_  ∣_∣      _∣
//  ∣   ∣_  ∣ ∣   ∣ ∣_  ∣ ∣     ∣_
//
//
//  -> Score -> 501 Game:
//
//  L-00 501
//  L-00 501

//	         _               _   _
//  ∣    _  ∣ ∣   ∣     ∣_∣ ∣ ∣  _∣
//  ∣_      ∣_∣   ∣       ∣ ∣_∣  _∣

//	         _   _       _   _
//  ∣    _  ∣ ∣  _∣     ∣_  ∣ ∣   ∣
//  ∣_      ∣_∣  _∣      _∣ ∣_∣   ∣

//  Cricket:
//
//  20  19  18  17   Score
//   _   _   _   _           _   _
//  ∣ ∣ ∣   ∣ ∣ ∣ ∣     ∣_∣ ∣ ∣  _∣
//  ∣   ∣_  ∣_∣           ∣ ∣_∣  _∣
//  16  15  BULL
//********** '01 Double Out **********//
