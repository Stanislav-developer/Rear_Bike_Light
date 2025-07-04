/*
Скетч для заднього велоліхтаря. Реалізує такий функціонал:
*Обробка короткого та довгого нажаття кнопки 
*Вибір режиму світіння
*Збереження останнього використаного режиму у енергонезалежну память EEPROM 
*Сон мікроконтролера та енергозбереження

*/
#include <Arduino.h>
#include <avr/sleep.h>

//Привязка пінів:
#define butPin 1 //Пін кнопки
#define ledPin 0 //Вихідний пін керування

#define EEPROM_ADDR 0

#define debounceInterval 50
#define buttonLongPressDuration 1000

//Змінні для обробки кнопки:
bool previousStateButton = LOW; 
bool longPressState = false;                      // previousstate of the switch    // Time we wait before we see the press as a long press
unsigned long buttonLastPressMillis;                // Time in ms when we the button was pressed
unsigned long previousButtonMillis;                 // Timestamp of the latest reading
unsigned long buttonPressDuration;                  // Time the button is pressed in ms

//Загальні змінні:

byte lightMode = 7; //Змінна яка зберігає номер режиму світіння(всього таких режимів 8)
volatile bool sleepState = 1;
volatile bool firstleepState = 0;

uint16_t offDuration;
uint16_t onDuration;

byte fade_period = 15;
byte fade_amount = 5;

void setup() {
  pinMode(butPin, INPUT_PULLUP);
  pinMode(ledPin, OUTPUT);

  MCUCR|= 1<<SE;
  MCUCR|=(1<<SM1) | (0<<SM0);
  
  GIMSK|= 1<<INT0;
  sei();

}

// Прочитати з EEPROM
uint8_t eeprom_read(uint8_t addr) {
  while (EECR & (1 << EEPE));     // Чекаємо завершення попереднього запису
  EEAR = addr;                    // Адреса
  EECR |= (1 << EERE);            // Ініціюємо читання
  return EEDR;                    // Повертаємо дані
}

// Записати в EEPROM
void eeprom_write(uint8_t addr, uint8_t data) {
  while (EECR & (1 << EEPE));     // Чекаємо завершення попереднього запису
  EEAR = addr;
  EEDR = data;
  EECR |= (1 << EEMPE);           // Дозвіл на запис
  EECR |= (1 << EEPE);            // Запис
}

void ledBlink(){
  static unsigned long previousMillis = 0;
  static bool ledState = false;

  if (ledState && millis() - previousMillis >= onDuration){
    previousMillis = millis();
    ledState = false;
    analogWrite(ledPin, 0);
  }
  else if (!ledState && millis() - previousMillis >= offDuration){
    previousMillis = millis();
    ledState = true;
    analogWrite(ledPin, 255);
  }

}

void ledFade(){
  static unsigned long previousMillis2 = 0;
  static byte brightness;

  if (millis() - previousMillis2 >= fade_period){
    previousMillis2 = millis();

    analogWrite(ledPin, brightness);
    brightness += fade_amount;

    if (brightness <= 0 || brightness >= 255){
      fade_amount = -fade_amount;
    }
  }

}


void buttonProcessing() {

  
  if(millis() - previousButtonMillis > debounceInterval) {
    
    
    bool buttonState = !digitalRead(butPin);    

    if (buttonState == HIGH && previousStateButton == LOW && !longPressState) {
      buttonLastPressMillis = millis();
      previousStateButton = HIGH;
      
    }

    buttonPressDuration = millis() - buttonLastPressMillis;

    if (buttonState == HIGH && !longPressState && buttonPressDuration >= buttonLongPressDuration) {
      longPressState = true;
      lightMode = (lightMode + 1)%7;

      if(!firstleepState){
        firstleepState = 1;
      }

    }
      
    if (buttonState == LOW && previousStateButton == HIGH) {
      previousStateButton = LOW;
      longPressState = false;
      

      if (buttonPressDuration < buttonLongPressDuration) {
        
        eeprom_write(EEPROM_ADDR, lightMode);

        if (firstleepState){
          lightMode = 7;
          sleepState = 1;
        }
        else{
          firstleepState = 1;
        }
      }
    }
    
    previousButtonMillis = millis();

  }

}

//Функція перемикання режимів світіння
void lightSwitch(){
  switch (lightMode) {
    case 0:
    digitalWrite(ledPin, HIGH);
    
    break;

    case 1:
    analogWrite(ledPin, 100);

    break;

    case 2:
    analogWrite(ledPin, 25);

    break;

    case 3:
    onDuration = 600;
    offDuration = 900;
    ledBlink();

    break;

    case 4:
    onDuration = 200;
    offDuration = 300;
    ledBlink();

    break;

    case 5:
    ledFade();

    break;

    case 6:
    onDuration = 60;
    offDuration = 1500;
    ledBlink();

    break;

    case 7:
    analogWrite(ledPin, 0);
    break;

  }
}

ISR(INT0_vect){
  lightMode = eeprom_read(EEPROM_ADDR);
  firstleepState = 0;
  sleepState = 0;
}

void sleepCheck(){
  if(sleepState){
    MCUCR|= 1<<SE;
    GIMSK|= 1<<INT0;
    sleep_mode();
  }else{
    MCUCR&= ~(1<<SE);
    GIMSK&= ~(1<<INT0);
  }
}

void loop() {
  sleepCheck();
  buttonProcessing();
  lightSwitch();

}
