#include <SPI.h>
#include <Wire.h>
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

const int BUTTON1_INPUT_PIN = 4;
const int BUTTON2_INPUT_PIN = 5;
const int BUTTON3_INPUT_PIN = 6;
const int BUTTON4_INPUT_PIN = 7;

const int GAME_MODES = 3;
const int MAX_RAND = 4;
const int DEBOUNCE_WINDOW = 40;
const int BUTTON_OUTLINE = 5;
const int SEQUENCE_TEXT_SIZE = 7;
const int SEQUENCE_DELAY = 600;

enum GAME_STATE{
  HOME,
  RUNNING,
  OVER,
  NUM_STATES
};

enum GAME_STATE currentState = HOME;
int currRound = 1;
boolean finishedRound = true;
boolean roundStart = true;
int gameMode;
int sequenceLength = 3;
int button1LastVal = LOW;
int button2LastVal = LOW;
int button3LastVal = LOW;
int button4LastVal = LOW;

void setup() {
  Serial.begin(115200); // set baud rate to 115200
  randomSeed(analogRead(0));
  
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
  }
  
  oled.display();
}

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

  // normal mode button (joystick)
  const char normalText[] = "Press Button 1";

  oled.getTextBounds(normalText, 0, 0, &x, &y, &textWidth, &textHeight);
  textX = oled.width() / 2 - textWidth / 2;
  textY = oled.height() * 3 / 4 - textHeight / 2;
  oled.setCursor(textX, textY);
  oled.print(normalText);
  
  oled.drawRect(textX - BUTTON_OUTLINE, textY - BUTTON_OUTLINE, textWidth + 2 * BUTTON_OUTLINE, textHeight + 2 * BUTTON_OUTLINE, SSD1306_WHITE);

  int buttonCurr = digitalRead(BUTTON1_INPUT_PIN);

  if (isButtonPressed(BUTTON1_INPUT_PIN, buttonCurr, button1LastVal)) {
    button1LastVal = buttonCurr;
    if (buttonCurr == LOW) {
      currentState = RUNNING;
    }
  }
}

void renderGame() {
  if (finishedRound) {
    //gameMode = random(GAME_MODES);
    gameMode = 0;
    finishedRound = false;
  }

  if (gameMode == 0) {
    // holding up fingers
    if (roundStart) {
      int sequence[sequenceLength];
      for (int i = 0; i < sequenceLength; i++) {
        sequence[i] = (random(1, MAX_RAND + 1));
      }
      displaySequence(sequence);
      //currentSequence = sequence;
      roundStart = false;

      Serial.print(0);
      Serial.print(",");
      Serial.print(sequenceLength);
      Serial.print(",");
      Serial.print(sequence[0]);
      for (int i = 0; i < sequenceLength; i++) {
        Serial.print(",");
        Serial.print(sequence[i]);
      }
      Serial.println();
    } else {
      // Check to see if there is any incoming serial data
      if(Serial.available() > 0){
        // If we're here, then serial data has been received
        // Read data off the serial port until we get to the endline delimeter ('\n')
        // Store all of this data into a string
        String rcvdSerialData = Serial.readStringUntil('\n');
        int win = rcvdSerialData.toInt();

        if (win == 1) {
          // win stuff
          int dd[] = {1, 1, 1};
          displaySequence(dd);
        } else {
          // game over stuff
          int dd[] = {0, 0, 0};
          displaySequence(dd);
        }
      }
    }
  } else if (gameMode == 1) {
    // match expression
    if (roundStart) {
      Serial.print(gameMode);
      Serial.print(",");
      Serial.println(sequenceLength);
    }
    
  } else {
    // press buttons
    if (roundStart) {
      int sequence[sequenceLength];
      for (int i = 0; i < sequenceLength; i++) {
        sequence[i] = random(1, MAX_RAND + 1);
      }
      displaySequence(sequence);
      //currentSequence = sequence;
      roundStart = false;
      
      Serial.print(gameMode);
      Serial.print(",");
      Serial.println(sequenceLength);
    }
  }
}

void renderGameOver() {
  
}

/*
 * Checks if the button at buttonPin is pressed, using debouncing
 */
boolean isButtonPressed(int buttonPin, int buttonCurr, int lastButtonVal) {
  delay(DEBOUNCE_WINDOW);
  int buttonCurr2 = digitalRead(buttonPin);
  return buttonCurr == buttonCurr2 && lastButtonVal != buttonCurr;
}

void displaySequence(int sequence[]) {
  oled.clearDisplay();
  delay(SEQUENCE_DELAY);
  for (int i = 0; i < sequenceLength; i++) {
    char buff[33];
    itoa(sequence[i], buff, 10);
    int16_t x, y;
    uint16_t textWidth, textHeight;
    
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
