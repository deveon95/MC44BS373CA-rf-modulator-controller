/* Controller for RF Modulator
 * 
 * Connections required:
 * I2C: A4 and A5
 * Buttons: A0, A1, A2
 * LCD: 2 (RS), 3 (E), 4 (D4), 5 (D5), 6 (D6), 7 (D7)
 */

#include <AceButton.h>
#include <AdjustableButtonConfig.h>
#include <ButtonConfig.h>
using namespace ace_button;
#include <Wire.h>
#include <LiquidCrystal.h>

#define buttonFPin A0
#define buttonUPin A2
#define buttonDPin A1

ButtonConfig FConfig;
ButtonConfig UDConfig;
AceButton buttonF(&FConfig);
AceButton buttonU(&UDConfig);
AceButton buttonD(&UDConfig);
void handleFEvent(AceButton*, uint8_t, uint8_t);
void handleUDEvent(AceButton*, uint8_t, uint8_t);

LiquidCrystal lcd(2,3,4,5,6,7);

#define fnIdle 0
#define fnChannel 1
#define fnStandard 2
uint8_t standards[] = {'M','B','I','D'};
#define fnTestPattern 3
uint8_t function = fnIdle;
uint8_t fnCursorPositions[] = {16,2,14,15};

uint8_t testPatternEnable = 1;
uint8_t standard = 2;                 // 0 = M, 1 = BG, 2 = I, 3 = DK
uint32_t frequencyTimes100 = 47125;   // Set default channel

uint8_t frequencyDivisor = 0;
#define frequencyDivisorPow (1 << frequencyDivisor)
#define frequencyIncrementH 900
#define frequencyIncrementM 800
#define frequencyIncrementL 700

void setup() {
  FConfig.setEventHandler(handleFEvent);
  FConfig.setFeature(ButtonConfig::kFeatureLongPress);
  FConfig.setFeature(ButtonConfig::kFeatureSuppressAfterLongPress);
  FConfig.setDebounceDelay(40);
  UDConfig.setEventHandler(handleUDEvent);
  UDConfig.setFeature(ButtonConfig::kFeatureClick);
  UDConfig.setFeature(ButtonConfig::kFeatureRepeatPress);
  UDConfig.setRepeatPressDelay(100);
  UDConfig.setDebounceDelay(30);
  pinMode(buttonFPin, INPUT_PULLUP);
  buttonF.init(buttonFPin, HIGH, 0);
  pinMode(buttonUPin, INPUT_PULLUP);
  buttonU.init(buttonUPin, HIGH, 1);
  pinMode(buttonDPin, INPUT_PULLUP);
  buttonD.init(buttonDPin, HIGH, 2);
  
  lcd.begin(16,1);
  lcd.cursor();

  Wire.begin();
  writeConfig();
}

void loop() {
  buttonF.check();
  buttonU.check();
  buttonD.check();
}

void handleFEvent(AceButton* button, uint8_t eventType,
    uint8_t /* buttonState */) {
  switch (eventType) {
    case AceButton::kEventReleased:
      if (function == fnChannel) {
        function = fnStandard;
      } else if (function == fnStandard) {
        function = fnTestPattern;
      } else {
        function = fnChannel;
      }
      lcd.setCursor(fnCursorPositions[function],0);
      break;
    case AceButton::kEventLongPressed:
      if (function == fnIdle) {
        function = fnChannel;
      } else {
        function = fnIdle;
      }
      lcd.setCursor(fnCursorPositions[function],0);
      break;
  }
}

void handleUDEvent(AceButton* button, uint8_t eventType,
    uint8_t /* buttonState */) {
  switch (eventType) {
    case AceButton::kEventPressed:
    case AceButton::kEventRepeatPressed: {
      uint8_t pin = button->getPin();
      switch (pin) {
        case buttonUPin:
          if (function == fnChannel) {
            incrementFrequency();
            writeConfig();
          } else if (function == fnStandard) {
            standard = (standard + 1) & 3;
            writeConfig();
          } else if (function == fnTestPattern) {
            testPatternEnable = testPatternEnable ^ 1;
            writeConfig();
          }
          break;
        case buttonDPin:
          if (function == fnChannel) {
            decrementFrequency();
            writeConfig();
          } else if (function == fnStandard) {
            standard = (standard - 1) & 3;
            writeConfig();
          } else if (function == fnTestPattern) {
            testPatternEnable = testPatternEnable ^ 1;
            writeConfig();
          }
          break;
      }
    }
  }
}

void incrementFrequency() {
  if (frequencyTimes100 < uint32_t(102325) && (frequencyTimes100 >= uint32_t(30325))) {
    frequencyTimes100 += uint32_t(frequencyIncrementM);
  } else if (frequencyTimes100 < uint32_t(102325) && (frequencyTimes100 >= uint32_t(29425))) {
    frequencyTimes100 += uint32_t(frequencyIncrementH);
  } else if (frequencyTimes100 < uint32_t(102325) && (frequencyTimes100 >= uint32_t(10525))) {
    frequencyTimes100 += uint32_t(frequencyIncrementL);
  } else if (frequencyTimes100 < uint32_t(102325) && (frequencyTimes100 >= uint32_t(9725))) {
    frequencyTimes100 += uint32_t(frequencyIncrementM);
  } else if (frequencyTimes100 < uint32_t(102325)) {
    frequencyTimes100 += uint32_t(frequencyIncrementL);
  } else {
    frequencyTimes100 = 4125;
  }
  
  setFrequencyDivider();
}

void decrementFrequency() {
  if (frequencyTimes100 > uint32_t(30325)) {
    frequencyTimes100 -= frequencyIncrementM;
  } else if (frequencyTimes100 > uint32_t(29425)) {
    frequencyTimes100 -= frequencyIncrementH;
  } else if (frequencyTimes100 > uint32_t(10525)) {
    frequencyTimes100 -= frequencyIncrementL;
  } else if (frequencyTimes100 > uint32_t(9725)) {
    frequencyTimes100 -= frequencyIncrementM;
  } else if (frequencyTimes100 > uint32_t(4125)) {
    frequencyTimes100 -= frequencyIncrementL;
  } else {
    frequencyTimes100 = uint32_t(102325);
  }
  
  setFrequencyDivider();
}

void setFrequencyDivider() {
  if (frequencyTimes100 > uint32_t(42325)) {
    frequencyDivisor = 0;
  } else if (frequencyTimes100 > uint32_t(21025)) {
    frequencyDivisor = 1;
  } else if (frequencyTimes100 > uint32_t(10525)) {
    frequencyDivisor = 2;
  } else if (frequencyTimes100 >= uint32_t(4125)) {
    frequencyDivisor = 3;
  } else {
    frequencyDivisor = 4;
  }
}

void writeConfig() {
  writeConfig(0,0,0,frequencyDivisor,0,0,0,0,standard,0,testPatternEnable,frequencyTimes100);
}

void writeConfig(
  uint8_t SO, 
  uint8_t LOP, 
  uint8_t PS, 
  uint8_t X, 
  uint8_t SYSL, 
  uint8_t PWC, 
  uint8_t OSC, 
  uint8_t ATT, 
  uint8_t SFD, 
  uint8_t SREF, 
  uint8_t TPEN, 
  uint32_t frequencyTimes100)
{
  uint16_t frequencyBits = (uint32_t(frequencyDivisorPow) * frequencyTimes100) / uint32_t(25);

  // Calculate channel number
  uint8_t channelNumber;
  uint8_t channelDig3;
  uint8_t channelDig2;
  uint8_t channelDig1;
  if (frequencyTimes100 >= 47125) {
    channelDig3 = 'C';
    channelNumber = (frequencyTimes100 - 47125) / 800 + 21;
  } else if (frequencyTimes100 < 47125 && frequencyTimes100 >= 30325) {
    channelDig3 = 'S';
    channelNumber = (frequencyTimes100 - 30325) / 800 + 21;
  } else if (frequencyTimes100 < 47125 && frequencyTimes100 >= 23125) {
    channelDig3 = 'S';
    channelNumber = (frequencyTimes100 - 23125) / 700 + 11;
  } else if (frequencyTimes100 < 23125 && frequencyTimes100 >= 17525) {
    channelDig3 = 'C';
    channelNumber = (frequencyTimes100 - 17525) / 700 + 5;
  } else if (frequencyTimes100 < 17525 && frequencyTimes100 >= 10525) {
    channelDig3 = 'S';
    channelNumber = (frequencyTimes100 - 10525) / 700 + 1;
  } else if (frequencyTimes100 < 6925 && frequencyTimes100 >= 4025) {
    channelDig3 = 'C';
    channelNumber = (frequencyTimes100 - 4025) / 700 + 1;
  } else {
    channelDig3 = '-';
    channelNumber = 0;
  }
  channelDig2 = channelNumber ? channelNumber / 10 + 48 : '-';
  channelDig1 = channelNumber ? channelNumber % 10 + 48 : '-';
  
  // Write data to the LCD
  lcd.setCursor(0,0);
  lcd.write(channelDig3);
  lcd.write(channelDig2);
  lcd.write(channelDig1);
  lcd.write(' ');
  char dig6 = (frequencyTimes100 / 100000);
  lcd.write(dig6 ? dig6 + 48 : ' ');
  char dig5 = (frequencyTimes100 / 10000 % 10);
  lcd.write(dig6 || dig5 ? dig5 + 48 : ' ');
  lcd.write((frequencyTimes100 / 1000 % 10) + 48);
  lcd.write((frequencyTimes100 / 100 % 10) + 48);
  lcd.write('.');
  lcd.write((frequencyTimes100 / 10 % 10) + 48);
  lcd.write((frequencyTimes100 % 10) + 48);
  lcd.write(' ');
  lcd.write(frequencyDivisorPow + 48);
  lcd.write(' ');
  lcd.write(standards[standard]);
  lcd.write(testPatternEnable ? 'T' : ' ');

  lcd.setCursor(fnCursorPositions[function],0);
  
  uint8_t C1;
  uint8_t C0;
  uint8_t FM;
  uint8_t FL;
  C1 = B10000000 | ((SO & 1) << 5) | ((LOP & 1) << 4) | ((PS & 1) << 3) |
        ((X & B1100) >> 1) | (SYSL & 1);
  C0 = ((PWC & 1) << 7) | ((OSC & 1) << 6) | ((ATT & 1) << 5) |
        ((SFD & 3) << 3) | ((SREF & 1) << 2) | ((X & B110000) >> 4);
  FM = ((TPEN & 1) << 6) | (uint8_t(0B111111 & (uint16_t(frequencyBits) >> 6)));
  FL = ((frequencyBits & B111111) << 2) | (X & 3);
  writeConfigRaw(C1,C0,FM,FL);
}

void writeConfigRaw(uint8_t C1, uint8_t C0, uint8_t FM, uint8_t FL) {
  Wire.beginTransmission(0xCA >> 1);
  Wire.write(C1);
  Wire.write(C0);
  Wire.write(FM);
  Wire.write(FL);
  Wire.endTransmission();
}

