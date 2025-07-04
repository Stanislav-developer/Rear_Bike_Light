/*
Rear Bike Light - Firmware_2(Alternative firmware)

* Author: Stanislav (GitHub: https://github.com/Stanislav-developer || Youtube: https://www.youtube.com/@TehnoMaisterna)
* Репозиторій проекту: https://github.com/Stanislav-developer/rear_bike_light
* Date: 01-06-2025
* Ліцензія: MIT License – проєкт відкритий, вільно використовуй з зазначенням автора 

* Version: V1.0
  Функціонал:
  * Статичне світло з регулюванням яскравості в 3 ступені
  * Режими миготіння: повільний, швидкий, еко режим
  * Режим дихаючого світлодіода (FADE)

* Опис:

  Прошивка заднього велоліхтарика, призначена для мікроконтролера ATtiny13/ATtiny13A

  Реалізує такий функціонал:

    1.Вкл/викл ліхтарик коротким нажаттям на кнопку

    2.Обробка довгого та короткого нажаття на кнопку та фільтрація дребезгу контактів

    3.Довге нажаття на кнопку(>1сек) включає наступний режим зі списку:
      1.Максимальна яскравість (Споживання: 350 мА)
      2.Середня яскравість (Споживання: ~150 мА)
      3.Мінімальна яскравість (Споживання: ~40 мА)
      4.Режим миготіння повільний
      5.Режим миготіння швидкий
      6.Дихаючий режим або ж FADE (Плавне згасання та загорання світлодіода)
      7.Еко режим (Спалахує кожні 1,5 секунди. Береже заряд акумулятору та добре помітний у день)

    4.Збереження останнього вибраного режиму у енергонезалежну EEPROM память
      Вам не доведеться постійно перемикати всі режими щоб включити якийсь конкретний.
      Після виключення ліхтарика останній вибраний режим зберігається у енергонезалежну EEPROM память та при наступному включенні ліхтарика активується останній вибраний вами режим

    5.Реалізована функція SleepMode:
      Коли ліхтарик виключений, ATtiny13 активує переривання на піні INT0 та переходить у режим сну (SLEEP_PWR_DOWN).
      Споживання у режимі сну близько 0,00003 А тобто 30 мікроАмпер що дуже мало! Потрібно близько 4 років щоб повністю розрядити акумулятор 1Ah при такому режимі)

* Конфігурація пінів:
  5 пін(PB0) - керуючий вихідний pwm
  6 пін(PB1) - вхід для тактової кнопки з підтяжкою до VCC та перериванням INT0

*/

// ПІДКЛЮЧЕННЯ БІБЛІОТЕК
#include <Arduino.h> // Підключаємо бібліотеку Ардуїнівських функцій
#include <avr/sleep.h> // Підключаємо бібліотеку для роботи з sleep_mode

// ПРИВЯЗКА ПІНІВ
#define butPin 1 // 6 Пін кнопки (PB1)
#define ledPin 0 // 5 Пін виходу pwm (PB0)

// ОГОЛОШЕННЯ ЗМІННИХ

//Обробка кнопки:
#define debounceInterval 50 // Інтервал для фільтрації дребезгу контактів кнопки. За потреби (наприклад, при неякісній кнопці) збільшіть значення
#define buttonLongPressDuration 1000 // Час, після якого програма сприймає натискання кнопки як довге
bool previousStateButton = LOW; // Попередній стан кнопки (По дефолту не натиснута)
bool longPressState = false; // Стан довгого натиску (По дефолту FALSE)
unsigned long buttonLastPress; // Запамятовує момент коли кнопку натиснули(Час останнього натискання кнопки)
unsigned long previousButtonMillis; // Запамятовує час останнього зчитування стану кнопки з урахуванням debounceInterval
unsigned long buttonPressDuration; // Зберігає данні про те скільки часу кнопка утримується

// LightMode
byte lightMode = 7; // Зберігає номер поточного режиму світіння (По дефолту це 7 режим, світлодіод не світить)
#define EEPROM_ADDR 0 // Оголошуємо адрес eeprom памяті куди зберігатиметься змінна lightMode

// sleep_mode
volatile bool sleepState = true; // Показує чи перебуває мікроконтролер у режимі сну, якщо true - спить, якщо false - працює
volatile bool isFirstPress = false; // Показує чи було вже перше коротке або довге натискання перед сном, ця змінна потрібна для коректної обробки кнопки після виходу мікроконтролера з режиму сну

// Параметри для функції ledBlink
uint16_t onDuration; // Час увімкненого стану світлодіода
uint16_t offDuration; // Час вимкненого стану світлодіода

// Параметри для функції ledFade
byte fade_period = 15; // Інтервал між кроками зміни яскравості (швидкість затухання)
byte fade_amount = 5; // Розмір кроку зміни PWM (наскільки сильно змінюється яскравість за один крок)

// ОСНОВНИЙ БЛОК КОДУ

void setup() {
  pinMode(butPin, INPUT_PULLUP); // Оголошуємо butPin як вхід з підтяжкою до VCC !Сигнал з кнопки інвертований
  pinMode(ledPin, OUTPUT); 

  MCUCR|= 1<<SE; // Вмикаємо можливість переходу в режим сну, те саме що sleep_enable()
  MCUCR|=(1<<SM1) | (0<<SM0); // Задаємо режим сну: "SLEEP_MODE_PWR_DOWN" самий економний режим сну (споживання ~30мкА)
  
  GIMSK|= 1<<INT0; // Вмикаємо переривання INT0 на 6 піні(PB1), переривання спрацьовує коли на піні зявляється LOW (низький рівень сигналу)
  sei(); // Дозволяємо глобальні переривання, насправді це не обовязково писати, але йой най буде)
}

// Функція читання з EEPROM
uint8_t eeprom_read(uint8_t addr) {
  while (EECR & (1 << EEPE));     // Чекаємо завершення попереднього запису
  EEAR = addr;                    // Записуємо адресу у регістр адреси EEPROM
  EECR |= (1 << EERE);            // Запускаємо операцію читання з EEPROM
  return EEDR;                    // Повертаємо прочитане значення з регістру даних EEPROM
}

// Функція запису в EEPROM
void eeprom_write(uint8_t addr, uint8_t data) {
  while (EECR & (1 << EEPE));     // Чекаємо, поки завершиться попередній запис
  EEAR = addr;                    // Встановлюємо адресу EEPROM, куди будемо записувати
  EEDR = data;                    // Записуємо байт даних у регістр даних EEPROM
  EECR |= (1 << EEMPE);           // Встановлюємо дозвіл на запис
  EECR |= (1 << EEPE);            // Виконуємо запис
}

// Функція мигання світлодіодом
void ledBlink(){
  static unsigned long previousMillis = 0; // Попередній час
  static bool ledState = false; // Стан світлодіоду
  
  // Якщо ledState is true та пройшло більше мілісекунд за onDuration
  if (ledState && millis() - previousMillis >= onDuration)
  {
    previousMillis = millis(); // Попередній час = поточний час
    ledState = false;
    analogWrite(ledPin, 0); // Світлодіод виключений
  }
  // Інакше якщо !ledState is true та пройшло більше мілісекунд за offDuration
  else if (!ledState && millis() - previousMillis >= offDuration)
  {
    previousMillis = millis(); // Попередній час = поточний час
    ledState = true;
    analogWrite(ledPin, 255); // Включаємо світлодіод на максимальній яскравості
  }
}

// Функція яка реалізує ефект дихаючого світла
void ledFade(){
  static unsigned long previousMillis = 0; // Попередній час
  static byte brightness = 0; // Яскравість світлодіода

  // Якщо пройшло більше мілісекунд за fade_period
  if (millis() - previousMillis >= fade_period)
  {
    previousMillis = millis(); // Попередній час = поточний час

    analogWrite(ledPin, brightness); // Включаємо світлодіод на яскравості bightness
    brightness += fade_amount; // збільшуємо яскравість світлодіода на fade_amount

    // Якщо яскравість світлодіоду мінімальна або максимальна
    if (brightness <= 0 || brightness >= 255)
    {
      fade_amount = -fade_amount; // Змінюємо знак fade_amount
    }
  }
}

// Функція обробника кнопки
void buttonProcessing() {
  
  if(millis() - previousButtonMillis > debounceInterval) // Фльтруємо дребезг кнопки
  {
    bool buttonState = !digitalRead(butPin); // Читаємо стан кнопки та інвертуємо його, оскільки кнопка з INPUT_PULLUP  

    if (buttonState == HIGH && previousStateButton == LOW && !longPressState) // Перевіряє чи кнопка натиснута
    {
      buttonLastPress = millis();
      previousStateButton = HIGH;
    }

    buttonPressDuration = millis() - buttonLastPress; // Віднімаємо поточний час millis() від buttonLastPress щоб дізнатися скільки часу кнопка натиснута  

    if (buttonState == HIGH && !longPressState && buttonPressDuration >= buttonLongPressDuration) // Обробка довгого натискання
    {
      longPressState = true;
      lightMode = (lightMode + 1) % 7; // Поступово збільшуємо lightMode, якщо lightMode > 6 ми його обнуляємо
      // 7 це номер останнього режиму який по дефолту повинен вимикати світлодіод, тому якщо ви змінили кількість режимів то також змініть це значення

      if(!isFirstPress) //Перевірка чи ще не було короткого натискання кнопки після sleep_mode
      {
        isFirstPress = true;
      }
    }
      
    if (buttonState == LOW && previousStateButton == HIGH) // Перевіряє чи кнопка відпущена
    {
      previousStateButton = LOW;
      longPressState = false;
      
      if (buttonPressDuration < buttonLongPressDuration) // Обробка короткого натискання
      {

        if (isFirstPress) // Якщо коротке натискання вже відбулося після виходу зі сну
        {
          eeprom_write(EEPROM_ADDR, lightMode); // Записуємо останній вибраний режим у EEPROM
          lightMode = 7; // lightMode присвоюємо 7, це останній режим у якому світлодіод повинен бути повністю вимкненим
          sleepState = true; // Прапорець sleepState присвоюємо true, щоб мікроконтролер зайшов у сон
        }
        else
        {
          isFirstPress = true; 
        }
      }
    }
    previousButtonMillis = millis(); // Попередній час = поточний час
  }
}

// Функція перемикання режимів
void lightSwitch(){

  switch (lightMode) 
  {
    // 1 Режим (Максимальна яскравість)
    case 0:
    digitalWrite(ledPin, HIGH); // Cвітимо на максимальну яскравість (Споживання ~350мА)
    break;

    // 2 Режим (Середня яскравість)
    case 1:
    analogWrite(ledPin, 100); // Яскравість: 100 із 255, середня яскравість (Споживання ~140мА)
    break;

    // 3 Режим (Мінімальна яскравість)
    case 2:
    analogWrite(ledPin, 25); // Яскравість 25 із 255, мінімальна яскравість (Споживання ~40мА)
    break;

    // 4 Режим (Блимання повільне)
    case 3:
    // Задаємо значення перед викликом функції
    onDuration = 600;  // Світлодіод світить 600ms
    offDuration = 900; // Світлодіод виключений 900ms
    ledBlink();        // Викликаємо функцію
    break;

    // 5 Режим (Блимання швидке)
    case 4:
    // Задаємо значення перед викликом функції
    onDuration = 200;  // Світлодіод світить 200ms
    offDuration = 300; // Світлодіод виключений 300ms
    ledBlink();        // Викликаємо функцію
    break;

    // 6 Режим (Дихаючий ефект)
    case 5:
    ledFade();         // Викликаємо функцію
    break;

    // 7 Режим (Еко режим, тільки короткочасні імпульси)
    case 6:
    // Задаємо значення перед викликом функції
    onDuration = 60;    // Світлодіод світить 60ms
    offDuration = 1500; // Світлодіод виключений 1500ms
    ledBlink();         // Викликаємо функцію
    break;

    // Стан світлодіоду по замовчуванню (Виключений)
    case 7:
    analogWrite(ledPin, 0); // Яскравість 0 із 255, світлодіод виключений
    break;
  }
}

// Обробник переривань, запускається коли мікроконтролер знаходиться у режимі сну та спрацьовує переривання INT0 на PB1
ISR(INT0_vect){
  lightMode = eeprom_read(EEPROM_ADDR); // Читаємо останній режим світіння з EEPROM та присвоюєм його lightMode
  isFirstPress = false; // За замовчуванням, першого натиску на кнопку ще не було
  sleepState = false; // Прапорець sleepState виставляємо в false, тим самим забороняємо режим сну та виходимо з нього у функції sleepCheck
}

// Функція перевірки сну
void sleepCheck(){
  if(sleepState) // Якщо sleepState = true, тобто режим сну дозволено
  {
    MCUCR|= 1<<SE; // Вмикаємо можливість переходу в режим сну, те саме що sleep_enable()
    GIMSK|= 1<<INT0; // Вмикаємо переривання INT0 на 6 піні(PB1), переривання спрацьовує коли на піні зявляється LOW (низький рівень сигналу)
    sleep_mode(); // Солодко засинаємо :)
  }
  else
  {
    MCUCR&= ~(1<<SE); // Забороняємо перехід в режим сну, те саме що sleep_disable()
    GIMSK&= ~(1<<INT0); // Вимикаємо переривання INT0 на PB1
  }
}

// Головний цикл
void loop() {
  sleepCheck(); // Викликаємо функцію перевірки сну
  buttonProcessing(); // Викликаємо функцію обробника кнопки
  lightSwitch(); // Викликаємо функцію перемикання режимів світіння
}

// Версія прошивки: V1.0
// Усі режими стабільні, збереження в EEPROM працює
// Режим сну перевірено — 30μA в sleep_mode

// Код протестовано на ATtiny13A @9.6 MHz, BOD level - 2.7V, живлення 3.7В Li-ion (Fuses: lfuse: 7A hfuse: FB)