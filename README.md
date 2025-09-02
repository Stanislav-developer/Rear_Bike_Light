# __Alternative_Firmware__

## Альтернативна прошивка для ліхтарика з можливістю керувати двома світлодіодами. За основу взятий третій варіант прошивики Rear_Bike_Light та трішки перероблений.

## Основні зміни:
- Переніс переривання з INT0 на PCINT
- Робота з EEPROM тепер реалізується за допомогою Arduino бібліотеки 
- Тепер задіяні два піна мікрокнотролера для керування
  
## Примітки:
Оскільки переривання тепер на PCINT то ми можемо не обмежуватись тільки одним 6 піном з перериванням INT0, а використовувати кнопку на решту п'яти пінах, у коді потрібно тільки змінити змінну butPin на номер піну та номер PCINT змінити на відповідний. Також оскільки третій варіант прошивки займав не так багато місця у пам'яті МК, я вирішив реаліувати роботу з EEPROM не через чистий C а через Arduino бібліотеку яка для багатьох буде зрозумілішою.

## Розпіновка ATtiny13:
![photo1](https://camo.githubusercontent.com/07803fa9a0c8fa2cb41fb8ece99b68048f2dd247826ae0d74dad512728b6c26f/68747470733a2f2f692e696d6775722e636f6d2f364c367262534f2e706e67)


## Альтернативна схема ліхтарика:
![photo1](https://raw.githubusercontent.com/Stanislav-developer/Rear_Bike_Light/refs/heads/Alternative-Firmware/Alternative%20circuit.png)

## Фото ліхтарика:
![photo1](https://raw.githubusercontent.com/Stanislav-developer/Rear_Bike_Light/refs/heads/Alternative-Firmware/Photo1.jpg)

![photo1](https://raw.githubusercontent.com/Stanislav-developer/Rear_Bike_Light/refs/heads/Alternative-Firmware/Photo2.jpg)


### 2025.07.26


