// Get this convenient little rotary encoder library from here: https://github.com/brianlow/Rotary
#include <Rotary.h>


/*
  Some of these displays are common anode, some common cathode (depending on which way the LEDs are set up,
  electricity is supposed to flow the opposite way) If you are getting strange results (specifically the
  LEDs you want on are off, and the ones you want off are on), try toggling this value.
*/
#define IS_COMMON_ANODE true

/*
  If the button is connected to the 5V line on press, set this to true/1
  If the button is connected to the GND line on press, set this to false/0
*/
#define PRESSED_BTN_VALUE 0
#define BTN_PIN A0
#define R_CLK A2
#define R_DT A1

// The pin the buzzer is connected to
#define BZR_PIN A4

/*
  First we select which digit by picking one of D1-D4 to set HIGH and setting the others low
  D1 D2 D3 D4
   1  2  3  4
      ^ If D2 is high (and the others are low), we are controlling this digit
*/
#define D1 2 // 9
#define D2 5 // 10
#define D3 6 // 11
#define D4 8 // 12

/*
  Then we use pins A-G control that selected digit
      A
     ---
  F |   | B
    | G |
     ---
  E |   | C
    |   |
     ---  . H
      D
 */
#define PIN_A 3 // 2
#define PIN_B 7 // 3
#define PIN_C 10 // 4
#define PIN_D 12 // 5
#define PIN_E 13 // 6
#define PIN_F 4 // 7
#define PIN_G 9 // 8
#define PIN_H 11 // 13


// Variables for timer
int remainingMinutes = 1;
int remainingSeconds = 10;

long millisOfLastTimerUpdate = 0;


// Variables for button
bool buttonWasPressed = false;


// Variables for display/time
bool countingDown = false;
bool waitingAtZero = false;
bool numbers[10][8] {
  // 0
  {
    true,
    true,
    true,
    true,
    true,
    true,
    false,
    false,
  },
  // 1
  {
    false,
    true,
    true,
    false,
    false,
    false,
    false,
    false,
  },
  // 2
  {
    true,
    true,
    false,
    true,
    true,
    false,
    true,
    false,
  },
  // 3
  {
    true,
    true,
    true,
    true,
    false,
    false,
    true,
    false,
  },
  // 4
  {
    false,
    true,
    true,
    false,
    false,
    true,
    true,
    false,
  },
  // 5
  {
    true,
    false,
    true,
    true,
    false,
    true,
    true,
    false,
  },
  // 6
  {
    true,
    false,
    true,
    true,
    true,
    true,
    true,
    false,
  },
  // 7
  {
    true,
    true,
    true,
    false,
    false,
    false,
    false,
    false,
  },
  // 8
  {
    true,
    true,
    true,
    true,
    true,
    true,
    true,
    false,
  },
  // 9
  {
    true,
    true,
    true,
    true,
    false,
    true,
    true,
    false,
  }
};
Rotary r = Rotary(R_CLK, R_DT);


// Variables for sound
long lastSoundTime = -999;
long buzzerEndTime;


void setup() {
  // Set all display pins as outputs
  pinMode(D1, OUTPUT);
  pinMode(D2, OUTPUT);
  pinMode(D3, OUTPUT);
  pinMode(D4, OUTPUT);

  pinMode(PIN_A, OUTPUT);
  pinMode(PIN_B, OUTPUT);
  pinMode(PIN_C, OUTPUT);
  pinMode(PIN_D, OUTPUT);
  pinMode(PIN_E, OUTPUT);
  pinMode(PIN_F, OUTPUT);
  pinMode(PIN_G, OUTPUT);
  pinMode(PIN_H, OUTPUT);

  pinMode(BTN_PIN, INPUT_PULLUP);
  pinMode(BZR_PIN, OUTPUT);

  Serial.begin(115200);
  r.begin(true);
}

void loop() {
  // Check if the button was just pressed by getting the current value and comparing it with the value last time we checked
  bool buttonIsPressed = digitalRead(BTN_PIN) == PRESSED_BTN_VALUE;
  bool buttonJustPressed = buttonIsPressed && !buttonWasPressed;
  // Store the value in the button for ^ that check ^ on the next loop()
  buttonWasPressed = buttonIsPressed;

  if (buttonJustPressed && (countingDown || waitingAtZero)) {
    countingDown = false;
    waitingAtZero = false;
    return;
  }

  if (countingDown) {
    long currentTime = millis();
    if (currentTime - millisOfLastTimerUpdate >= 1000) {
      updateTimer(currentTime);
    }
  }
  // If the button is pressed with time on the clock, start counting down from that time
  else if (buttonJustPressed && (remainingMinutes > 0 || remainingSeconds > 0)) {
    countingDown = true;
  }
  // If neither of those, check if rotary encoder has changed and if so, change the time remaining
  setDisplay(remainingMinutes * 100 + remainingSeconds);
  
  if (countingDown || waitingAtZero) {
    handleSound();
  }
  else {
    digitalWrite(BZR_PIN, LOW);
    adjustTimeBasedOnEncoder();
  }
}

void updateTimer(long currentTime) {
  millisOfLastTimerUpdate = currentTime;
  remainingSeconds--;

  if (remainingSeconds >= 0) {
    return;
  }
  
  remainingMinutes--;

  if (remainingMinutes >= 0) {
    remainingSeconds = 59;
    return;
  }

  remainingMinutes = 0;
  remainingSeconds = 0;
  countingDown = false;
  waitingAtZero = true;
}

void adjustTimeBasedOnEncoder() {
  unsigned char movement = r.process();
  if (!movement) {
    return;
  }

  // Turn right = increase time
  if (movement == DIR_CCW) {
    // All of this is to make sure that under 1 minute or if there are spare seconds (eg if the user paused at 4:26)
    // then we adjust seconds, otherwise we adjust minutes
    // (unless we are at 99 minutes, then we go back to adjusting seconds up to 59 seconds)
    if (remainingMinutes < 99 && remainingMinutes > 0 && remainingSeconds == 0) {
      remainingMinutes++;
    }
    else if (remainingSeconds < 59) {
      remainingSeconds++;
    }
    else if (remainingSeconds == 59 && remainingMinutes < 99) {
      remainingSeconds = 0;
      remainingMinutes++;
    }
  }

  // Turn left = decrease time
  else if (movement == DIR_CW) {
    // All of this is to make sure that under 1 minute or if there are spare seconds (eg if the user paused at 4:26)
    // then we adjust seconds, otherwise we adjust minutes
    if (remainingMinutes > 1 && remainingSeconds == 0) {
      remainingMinutes--;
    }
    else if (remainingSeconds > 0) {
      remainingSeconds--;
    }
    else if (remainingSeconds == 0 && remainingMinutes > 0) {
      remainingSeconds = 59;
      remainingMinutes--;
    }
  }
}

void setDisplay(int toDisplay) {
  int firstDigit = toDisplay / 1000;
  int secondDigit = (toDisplay % 1000) / 100;
  int thirdDigit = (toDisplay % 100) / 10;
  int fourthDigit = toDisplay % 10;

  setDigit(1, firstDigit);
  delay(1);
  setDigit(2, secondDigit);
  delay(1);
  setDigit(3, thirdDigit);
  delay(1);
  setDigit(4, fourthDigit);
  delay(1);
}

void setDigit(int digit, int value) {
  // Some of these displays have a common anode (so D1, D2, D3, D4 are positive, A-H are negative)
  // while others are inverted, so we have to take that into account with the IS_COMMON_ANODE variable

  // Select the digit by turning it on and the others off
  // So we want the selected digit to be on if the LEDs are common anode, otherwise off (and vice versa for the other LEDs)
  // Example: if digit is 1, then digit == 1 is true. So if IS_COMMON_ANODE is true, then true == true gives us true, and 1 will be on
  // But if the digit isn't 1, it won't be on. And if IS_COMMON_ANODE was false, that would be flipped
  digitalWrite(D1, (digit == 1) == IS_COMMON_ANODE);
  digitalWrite(D2, (digit == 2) == IS_COMMON_ANODE);
  digitalWrite(D3, (digit == 3) == IS_COMMON_ANODE);
  digitalWrite(D4, (digit == 4) == IS_COMMON_ANODE);
  
  // Set each LED in that digit to the on/off based on the passed in value
  // If IS_COMMON_ANODE is true, we want segments that are true to be set to false (which will cause them to be on),
  // because this is the non-common side (and therefore the cathode)
  // So we're basically doing true != true which gives us false, so they're on)
  digitalWrite(PIN_A, numbers[value][0] != IS_COMMON_ANODE);
  digitalWrite(PIN_B, numbers[value][1] != IS_COMMON_ANODE);   
  digitalWrite(PIN_C, numbers[value][2] != IS_COMMON_ANODE);   
  digitalWrite(PIN_D, numbers[value][3] != IS_COMMON_ANODE);   
  digitalWrite(PIN_E, numbers[value][4] != IS_COMMON_ANODE);   
  digitalWrite(PIN_F, numbers[value][5] != IS_COMMON_ANODE);
  digitalWrite(PIN_G, numbers[value][6] != IS_COMMON_ANODE);
  digitalWrite(PIN_H, (digit == 2) != IS_COMMON_ANODE);
}

void handleSound() {
  // Sound only played during last minute
  if (remainingMinutes > 1 || (remainingMinutes == 1 && remainingSeconds > 0)) {
    return;
  }
  
  checkEndBuzzer();
  
  int soundDelay;
  int soundLength;
  if (remainingMinutes > 0) {
    soundDelay = 10000;
    soundLength = 100;
  }
  else if (remainingSeconds < 1) {
    soundDelay = 100;
    soundLength = 50;
  }
  else if (remainingSeconds < 5) {
    soundDelay = 250;
    soundLength = 50;
  }
  else if (remainingSeconds < 10) {
    soundDelay = 500;
    soundLength = 50;
  }
  else if (remainingSeconds < 20) {
    soundDelay = 1000;
    soundLength = 50;
  }
  else if (remainingSeconds < 30) {
    soundDelay = 5000;
    soundLength = 50;
  }
  else {
    soundDelay = 10000;
    soundLength = 50;
  }

  long currentTime = millis();

  if (currentTime - lastSoundTime >= soundDelay) {
    startBuzzer(soundLength, currentTime);
  }
}

void checkEndBuzzer() {
  if (millis() > buzzerEndTime) {
    digitalWrite(BZR_PIN, LOW);
  }
}

void startBuzzer(int timeToPlay, long currentTime) {
  lastSoundTime = currentTime;
  buzzerEndTime = currentTime + timeToPlay;
  digitalWrite(BZR_PIN, HIGH);
}
