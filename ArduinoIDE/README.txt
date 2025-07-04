Перед записом скетчу в мікроконтролер переконайтеся що у вас виставлені такі налаштування у вікні tools(інструменти):

Board(плата): ATtiny13 (MicroCore → ATtiny13)
BOD: BOD 2.7
EEPROM: EEPROM retained
Clock: 9.6 MHz

Programmer(програматор): USBasp або Arduino as ISP(Якщо ви прошиваєте через плату Arduino UNO,NANO)

Перед прошивкою рекомендується спершу записати Bootloader у мікроконтролер(для цього у самому низу натисніть Burn Bootloader (Записати завантажувач)

Тепер можна завантажити прошивку:
(Вікно Sketch → Upload Using Programmer)

Готово!

Ядро для роботи з attiny13 MicroCore:
https://mcudude.github.io/MicroCore/package_MCUdude_MicroCore_index.json

Якщо виникли проблеми, рекомендую глянути це відео:
https://youtu.be/dbkITW6Pz68?si=roI1-eetyj3U_qAi
