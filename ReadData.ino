#include <MFRC522.h> 

#define RST_PIN  9  // Пин сброса
#define SS_PIN  10  // Пин выбора ведомого устройства

MFRC522 mfrc522(SS_PIN, RST_PIN); // Создаем экземпляр MFRC522

void setup() {
 Serial.begin(9600);  // Инициализируем монитор последовательного порта
 while (!Serial);  // Ничего не делаем, пока монитор порта не открыт (для Arduino на чипе ATMEGA32U4)
 SPI.begin();   // Инициализируем SPI шину
 mfrc522.PCD_Init();  // Инициализируем RFID модуль
 ShowReaderDetails(); // Выводим данные о модуле MFRC522
 Serial.println(F("Сканируйте PICC для получения UID, типа и блоков данных..."));
}

void loop() {
 // Ищем новую карту
 if ( ! mfrc522.PICC_IsNewCardPresent()) {
  return;
 }

 // Выбираем одну из карт
 if ( ! mfrc522.PICC_ReadCardSerial()) {
  return;
 }

 // Выводим данные с карты
 mfrc522.PICC_DumpToSerial(&(mfrc522.uid));
}

void ShowReaderDetails() {
 // Получаем номер версии модуля
 byte v = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
 Serial.print(F("Программная версия MFRC522: 0x"));
 Serial.print(v, HEX);
 if (v == 0x91)
  Serial.print(F(" = v1.0"));
 else if (v == 0x92)
  Serial.print(F(" = v2.0"));
 else
  Serial.print(F(" (неизвестная версия)"));
 Serial.println("");
 // Если получаем 0x00 или 0xFF, передача данных нарушена
 if ((v == 0x00) || (v == 0xFF)) {
  Serial.println(F("ВНИМАНИЕ: Ошибка связи, модуль MFRC522 подключен правильно?"));
 }
}