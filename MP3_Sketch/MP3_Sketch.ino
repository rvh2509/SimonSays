/*
 * Richard Halbert
 * Made for University of Washington
 * CSE 490F MP3 Spring 2021
 * 
 * Runs a version of the game "Simon Says" where the user has
 * to repeat a sequence of numbers on either the buttons
 * connected to the Arduino or through holding the correct
 * number of fingers up in their browser (using p5.js, see
 * ../sketch.js for details).
 * 
 * All Serial commmuncation is based on lessons from CSE 490F:
 * https://makeabilitylab.github.io/physcomp/communication/web-serial.html
 */

#include <SPI.h>
#include <Wire.h>
// Vector implementation from tomstewart89 on GitHub:
// https://github.com/tomstewart89/Vector
#include <Vector.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

/*
 * Setup for the Adafruit SSD1306 OLED display
 * Based on the Adafruit SSD1306 ssd1306_128x64_i2c.ino example
 */
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

/*
 * Wiring constants
 */
const int BUTTON1_INPUT_PIN = 4;
const int BUTTON2_INPUT_PIN = 5;
const int BUTTON3_INPUT_PIN = 6;
const int BUTTON4_INPUT_PIN = 7;
const int BUTTON5_INPUT_PIN = 8;

/*
 * Game constants
 */
const int GAME_MODES = 2;
const int FINGERS = 4;
const int BUTTONS = 4;
const int DEBOUNCE_WINDOW = 20;
const int BUTTON_OUTLINE = 5;
const int SEQUENCE_TEXT_SIZE = 7;
const int SEQUENCE_DELAY = 600;
const int SEQUENCE_INCREASE_FREQ = 3;
const int INITIAL_SEQUENCE_LENGTH = 3;
const int MAX_SEQUENCE_LENGTH = 20;

/*
 * This enum represents the current state the game is in.
 */
enum GAME_STATE{
  HOME,
  RUNNING,
  OVER,
  ROUND_WIN
};

/*
 * Variable setup
 */
enum GAME_STATE currentState = HOME;
int currRound = 0;
int highRound = 0;
boolean newRound = true;
boolean roundStart = true;
int gameMode;
int sequenceLength = INITIAL_SEQUENCE_LENGTH;
int button1LastVal = HIGH;
int button2LastVal = HIGH;
int button3LastVal = HIGH;
int button4LastVal = HIGH;
Vector<int> correctButtonSequence;
Vector<int> currButtonSequence;

void setup() {
  Serial.begin(115200); // set baud rate to 115200
  randomSeed(analogRead(0)); // unused pin used to create random seed

  // set pin modes
  pinMode(BUTTON1_INPUT_PIN, INPUT_PULLUP);
  pinMode(BUTTON2_INPUT_PIN, INPUT_PULLUP);
  pinMode(BUTTON3_INPUT_PIN, INPUT_PULLUP);
  pinMode(BUTTON4_INPUT_PIN, INPUT_PULLUP);

  // start the OLED display
  if(!oled.begin(SSD1306_SWITCHCAPVCC, 0x3D)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  oled.clearDisplay();
}

void loop() {
  oled.clearDisplay();
  
  if (currentState == HOME) {
    renderHome();
  } else if (currentState == RUNNING) {
    renderGame();
  } else if (currentState == OVER) {
    renderGameOver();
  } else if (currentState == ROUND_WIN) {
    renderRoundWin();
  }
  
  oled.display();
}

/*
 * Renders the home screen
 */
void renderHome() {
  int16_t x, y;
  uint16_t textWidth, textHeight;
  const char introText[] = "Simon Says";
  
  oled.setTextSize(1);
  oled.setTextColor(WHITE, BLACK);
  
  oled.getTextBounds(introText, 0, 0, &x, &y, &textWidth, &textHeight);
  int16_t textX = oled.width() / 2 - textWidth / 2;
  int16_t textY = oled.height() / 4 - textHeight / 2;
  oled.setCursor(textX, textY);
  oled.print(introText);

  const char startText[] = "Press Button 1";

  oled.getTextBounds(startText, 0, 0, &x, &y, &textWidth, &textHeight);
  textX = oled.width() / 2 - textWidth / 2;
  textY = oled.height() * 3 / 4 - textHeight / 2;
  oled.setCursor(textX, textY);
  oled.print(startText);
  
  oled.drawRect(textX - BUTTON_OUTLINE, textY - BUTTON_OUTLINE, textWidth + 2 * BUTTON_OUTLINE, textHeight + 2 * BUTTON_OUTLINE, SSD1306_WHITE);

  int buttonCurr = digitalRead(BUTTON1_INPUT_PIN);

  if (isButtonPressed(BUTTON1_INPUT_PIN, buttonCurr, button1LastVal)) {
    button1LastVal = buttonCurr;
    if (buttonCurr == LOW) {
      currentState = RUNNING;
      button1LastVal = HIGH;
      button2LastVal = HIGH;
      button3LastVal = HIGH;
      button4LastVal = HIGH;
      newRound = true;
      currRound = 0;
      sequenceLength = INITIAL_SEQUENCE_LENGTH;
    }
  }
}

/*
 * Renders the game itself, updating game state
 */
void renderGame() {
  if (newRound) {
    // start a random game mode
    gameMode = random(GAME_MODES);
    roundStart = true;
    newRound = false;
  }

  if (gameMode == 0) {
    // holding up fingers
    if (roundStart) {
      // create a new sequence
      int sequence[sequenceLength];
      for (int i = 0; i < sequenceLength; i++) {
        int nextNum = random(1, FINGERS + 1);

        // This makes it so that there aren't two numbers
        // in a row that are the same for this mode. This
        // was done because it forces the user to change
        // how many fingers they're holding up.
        if (i != 0) {
          while (nextNum == sequence[i - 1]) {
            nextNum = random(1, FINGERS + 1);
          }
        }
        
        sequence[i] = nextNum;
      }
      displaySequence(sequence, false);
      roundStart = false;

      // send the sequence to p5.js
      Serial.print(0);
      Serial.print(",");
      Serial.print(sequenceLength);
      Serial.print(",");
      Serial.print(sequence[0]);
      for (int i = 1; i < sequenceLength; i++) {
        Serial.print(",");
        Serial.print(sequence[i]);
      }
      Serial.println();
    } else {
      // wait until p5.js tells the Arduino if
      // the user won the round or not
      waiting();
      // Check to see if there is any incoming serial data
      if(Serial.available() > 0){
        String rcvdSerialData = Serial.readStringUntil('\n');
        int win = rcvdSerialData.toInt();

        if (win == 1) {
          currentState = ROUND_WIN;
          currRound++;
        } else {
          currentState = OVER;
        }
      }
    }
  } else {
    // press buttons
    if (roundStart) {
      correctButtonSequence.Clear();
      currButtonSequence.Clear();

      // create a new sequence
      int sequence[sequenceLength];
      for (int i = 0; i < sequenceLength; i++) {
        sequence[i] = random(1, BUTTONS + 1);
      }
      displaySequence(sequence, true);
      correctButtonSequence.Assign(sequence, sequenceLength);
      roundStart = false;
    } else {
      waiting();
      int button1Curr = digitalRead(BUTTON1_INPUT_PIN);
      int button2Curr = digitalRead(BUTTON2_INPUT_PIN);
      int button3Curr = digitalRead(BUTTON3_INPUT_PIN);
      int button4Curr = digitalRead(BUTTON4_INPUT_PIN);

      boolean button1Pressed = false;
      boolean button2Pressed = false;
      boolean button3Pressed = false;
      boolean button4Pressed = false;
      
      if (isButtonPressed(BUTTON1_INPUT_PIN, button1Curr, button1LastVal)) {
        button1LastVal = button1Curr;
        button1Pressed = button1Curr == LOW;
      }
      
      if (isButtonPressed(BUTTON2_INPUT_PIN, button2Curr, button2LastVal)) {
        button2LastVal = button2Curr;
        button2Pressed = button2Curr == LOW;
      }
      
      if (isButtonPressed(BUTTON3_INPUT_PIN, button3Curr, button3LastVal)) {
        button3LastVal = button3Curr;
        button3Pressed = button3Curr == LOW;
      }
      
      if (isButtonPressed(BUTTON4_INPUT_PIN, button4Curr, button4LastVal)) {
        button4LastVal = button4Curr;
        button4Pressed = button4Curr == LOW;
      }

      int inputNumber = -1;
      if (button1Pressed) {
        inputNumber = 1;
      } else if (button2Pressed) {
        inputNumber = 2;
      } else if (button3Pressed) {
        inputNumber = 3;
      } else if (button4Pressed) {
        inputNumber = 4;
      }

      if (inputNumber != -1) {
        currButtonSequence.PushBack(inputNumber);
        displayNumber(inputNumber);
      }

      if (currButtonSequence.Size() == sequenceLength) {
        boolean win = true;

        for (int i = 0; i < sequenceLength; i++) {
          if (correctButtonSequence[i] != currButtonSequence[i]) {
            win = false;
            break;
          }
        }

        if (win) {
          currentState = ROUND_WIN;
          currRound++;
        } else {
          currentState = OVER;
        }
      }
    }
  }
}

/*
 * Renders the game over screen with current/high scores
 * and button to return to home screen
 */
void renderGameOver() {
  // update high score
  if (currRound > highRound) {
    highRound = currRound;
  }
  
  int16_t x, y;
  uint16_t textWidth, textHeight;

  const char overText[] = "Game Over";
  oled.setTextSize(1);
  oled.setTextColor(WHITE, BLACK);
    
  oled.getTextBounds(overText, 0, 0, &x, &y, &textWidth, &textHeight);
  int16_t textX = oled.width() / 2 - textWidth / 2;
  int16_t textY = oled.height() / 4 - textHeight;
  oled.setCursor(textX, textY);
  oled.print(overText);

  const char yourText[] = "Your Score: ";
  oled.getTextBounds(yourText, 0, 0, &x, &y, &textWidth, &textHeight);
  oled.setCursor(0, oled.height() / 2 - textHeight);
  oled.print(yourText);

  char buff[33];
  itoa(currRound, buff, 10);
  oled.setCursor(textWidth, oled.height() / 2 - textHeight);
  oled.print(buff);

  // write out the high score
  const char highText[] = "High Score: ";
  oled.getTextBounds(highText, 0, 0, &x, &y, &textWidth, &textHeight);
  oled.setCursor(0, oled.height() / 2);
  oled.print(highText);
  
  itoa(highRound, buff, 10);
  oled.setCursor(textWidth, oled.height() / 2);
  oled.print(buff);

  const char homeText[] = "Press Button 1";

  oled.getTextBounds(homeText, 0, 0, &x, &y, &textWidth, &textHeight);
  textX = oled.width() / 2 - textWidth / 2;
  textY = oled.height() - (2 * textHeight);
  oled.setCursor(textX, textY);
  oled.print(homeText);
  
  oled.drawRect(textX - BUTTON_OUTLINE, textY - BUTTON_OUTLINE, textWidth + 2 * BUTTON_OUTLINE, textHeight + 2 * BUTTON_OUTLINE, SSD1306_WHITE);

  int buttonCurr = digitalRead(BUTTON1_INPUT_PIN);

  if (isButtonPressed(BUTTON1_INPUT_PIN, buttonCurr, button1LastVal)) {
    button1LastVal = buttonCurr;
    if (buttonCurr == LOW) {
      currentState = HOME;
    }
  }
}

/*
 * Renders the round win screen, displaying the current score
 */
void renderRoundWin() {
  int16_t x, y;
  uint16_t textWidth, textHeight;
  const char winText[] = "You win!";
  
  oled.setTextSize(1);
  oled.setTextColor(WHITE, BLACK);
  
  oled.getTextBounds(winText, 0, 0, &x, &y, &textWidth, &textHeight);
  int16_t textX = oled.width() / 2 - textWidth / 2;
  int16_t textY = oled.height() / 4 - textHeight / 2;
  oled.setCursor(textX, textY);
  oled.print(winText);

  const char completedText[] = "Rounds Completed:";
  
  oled.setTextSize(1);
  oled.setTextColor(WHITE, BLACK);
  
  oled.getTextBounds(completedText, 0, 0, &x, &y, &textWidth, &textHeight);
  textX = oled.width() / 2 - textWidth / 2;
  textY = oled.height() / 2 - textHeight / 2;
  oled.setCursor(textX, textY);
  oled.print(completedText);

  char buff[33];
  itoa(currRound, buff, 10);
  oled.setCursor(textX + textWidth, textY);
  oled.print(buff);

  const char nextRoundText[] = "Press 1 To Continue";

  oled.getTextBounds(nextRoundText, 0, 0, &x, &y, &textWidth, &textHeight);
  textX = oled.width() / 2 - textWidth / 2;
  textY = oled.height() * 3 / 4 - textHeight / 2;
  oled.setCursor(textX, textY);
  oled.print(nextRoundText);
  
  oled.drawRect(textX - BUTTON_OUTLINE, textY - BUTTON_OUTLINE, textWidth + 2 * BUTTON_OUTLINE, textHeight + 2 * BUTTON_OUTLINE, SSD1306_WHITE);

  int buttonCurr = digitalRead(BUTTON1_INPUT_PIN);

  if (isButtonPressed(BUTTON1_INPUT_PIN, buttonCurr, button1LastVal)) {
    button1LastVal = buttonCurr;
    if (buttonCurr == LOW) {
      if (currRound % SEQUENCE_INCREASE_FREQ == 0 && currRound != 0 && sequenceLength != MAX_SEQUENCE_LENGTH) {
        sequenceLength++;
      }
      newRound = true;
      currentState = RUNNING;
    }
  }
}

/*
 * Checks if the button at buttonPin is pressed, using debouncing
 */
boolean isButtonPressed(int buttonPin, int buttonCurr, int lastButtonVal) {
  delay(DEBOUNCE_WINDOW);
  int buttonCurr2 = digitalRead(buttonPin);
  return buttonCurr == buttonCurr2 && lastButtonVal != buttonCurr;
}

/*
 * Displays the given sequence on the OLED,
 * telling the user if they need to mimic
 * it on the Arduino or p5.js
 */
void displaySequence(int sequence[], boolean arduino) {
  int16_t x, y;
  uint16_t textWidth, textHeight;

  const char mimicText[] = "Mimic this sequence";
  oled.setTextSize(1);
  oled.setTextColor(WHITE, BLACK);
    
  oled.getTextBounds(mimicText, 0, 0, &x, &y, &textWidth, &textHeight);
  int16_t textX = oled.width() / 2 - textWidth / 2;
  int16_t textY = oled.height() / 4 - textHeight / 2;
  oled.setCursor(textX, textY);
  oled.print(mimicText);
  
  if (arduino) {
    const char arduinoText[] = "on Arduino!";
    
    oled.setTextSize(1);
    oled.setTextColor(WHITE, BLACK);
    
    oled.getTextBounds(arduinoText, 0, 0, &x, &y, &textWidth, &textHeight);
    int16_t textX = oled.width() / 2 - textWidth / 2;
    int16_t textY = oled.height() / 2 - textHeight / 2;
    oled.setCursor(textX, textY);
    oled.print(arduinoText);
  } else {
    const char p5jsText[] = "on p5.js!";
    
    oled.setTextSize(1);
    oled.setTextColor(WHITE, BLACK);
    
    oled.getTextBounds(p5jsText, 0, 0, &x, &y, &textWidth, &textHeight);
    int16_t textX = oled.width() / 2 - textWidth / 2;
    int16_t textY = oled.height() / 2 - textHeight / 2;
    oled.setCursor(textX, textY);
    oled.print(p5jsText);
  }
  oled.display();
  delay(3 * SEQUENCE_DELAY);
  
  for (int i = 0; i < sequenceLength; i++) {
    oled.clearDisplay();
    oled.display();
    delay(SEQUENCE_DELAY);
    
    char buff[33];
    itoa(sequence[i], buff, 10);
    
    oled.setTextSize(SEQUENCE_TEXT_SIZE);
    oled.setTextColor(WHITE, BLACK);
    
    oled.getTextBounds(buff, 0, 0, &x, &y, &textWidth, &textHeight);
    int16_t textX = oled.width() / 2 - textWidth / 2;
    int16_t textY = oled.height() / 2 - textHeight / 2;

    oled.setCursor(textX, textY);
    oled.print(buff);
    
    oled.display();
    delay(SEQUENCE_DELAY);
  }
}

/*
 * Renders the screen showing while the arduino is waiting for input
 */
void waiting() {
  int16_t x, y;
  uint16_t textWidth, textHeight;

  const char waitingText[] = "Waiting for input...";
  oled.setTextSize(1);
  oled.setTextColor(WHITE, BLACK);
    
  oled.getTextBounds(waitingText, 0, 0, &x, &y, &textWidth, &textHeight);
  int16_t textX = oled.width() / 2 - textWidth / 2;
  int16_t textY = oled.height() / 2 - textHeight / 2;
  oled.setCursor(textX, textY);
  oled.print(waitingText);
  oled.display();
}

/*
 * Displays a number on the OLED tempoarily,
 * used to show the user what number they pressed
 */
void displayNumber(int n) {
  int16_t x, y;
  uint16_t textWidth, textHeight;
  
  oled.clearDisplay();
  oled.display();
        
  char buff[33];
  itoa(n, buff, 10);
        
  oled.setTextSize(SEQUENCE_TEXT_SIZE);
  oled.setTextColor(WHITE, BLACK);
        
  oled.getTextBounds(buff, 0, 0, &x, &y, &textWidth, &textHeight);
  int16_t textX = oled.width() / 2 - textWidth / 2;
  int16_t textY = oled.height() / 2 - textHeight / 2;
    
  oled.setCursor(textX, textY);
  oled.print(buff);
        
  oled.display();
  delay(SEQUENCE_DELAY / 2);
  oled.clearDisplay();
  oled.display();
}
