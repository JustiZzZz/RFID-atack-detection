#include <SPI.h>
#include <MFRC522.h>
#include <EEPROM.h>

#define RST_PIN 9 // Пин RST
#define SS_PIN 10 // Пин SDA

MFRC522 mfrc522(SS_PIN, RST_PIN);  
MFRC522::MIFARE_Key key;

// Создаем многомерный массив допустимых UID
byte validUIDs[][16] = {
    {0xfd, 0xab, 0x32, 0xd4, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0xff, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f},
};

// Создаем многомерный массив заблокированных UID
byte blacklistedUIDs[][16] = {
    {0xaf, 0xff, 0xff, 0x4f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
};

/**
 * Вспомогательная процедура для вывода массива из байтов
 */
void dump_byte_array(byte *buffer, byte bufferSize) {
    for (byte i = 0; i < bufferSize; i++) {
        Serial.print(buffer[i] < 0x10 ? " 0" : " ");
        Serial.print(buffer[i], HEX);
    }
}

/**
 * Вспомогательная процедура для преобразования массива
 */
String dump_byte(byte *buffer, byte bufferSize) {
    String hexString = "";
    for (byte i = 0; i < bufferSize; i++) {
        hexString += (buffer[i] < 0x10 ? "0" : "");
        hexString += String(buffer[i], HEX);
    }
    hexString.toUpperCase();
    return hexString;
}

void setup() {
    Serial.begin(9600); // Инициализируем Serial
    while (!Serial);    // Ничего не делаем пока Serial не заработает
    SPI.begin();        // Подключаем SPI
    mfrc522.PCD_Init(); // Подключаем RC522

    // Определяем ключ (Как А так и В)
    // Используем FFFFFFFFFFFFh который является заводским 
    for (byte i = 0; i < 6; i++) {
        key.keyByte[i] = 0xFF;
    }

    Serial.println(F("Сканируйте карту для демонстрации считывания и перезаписи"));
    Serial.print(F("Используемый ключ (Для A и B):"));
    dump_byte_array(key.keyByte, MFRC522::MF_KEY_SIZE);
    Serial.println();

    Serial.println(F("Данные будут записаны в PICC, в секторе #1"));
}

void loop() {
    // Сбросьте цикл, если в датчике/считывающем устройстве нет новой карты. Это позволяет сохранить весь процесс в режиме ожидания.
    if (!mfrc522.PICC_IsNewCardPresent())
        return;

    // Ожидание карты
    if (!mfrc522.PICC_ReadCardSerial())
        return;

    // Некоторые детали о PICC карты/тега
    Serial.print(F("UID карты: "));
    dump_byte_array(mfrc522.uid.uidByte, mfrc522.uid.size);
    Serial.println();
    Serial.print(F("Тип PICC: "));
    MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
    Serial.println(mfrc522.PICC_GetTypeName(piccType));

    // Если карта не от MIFARE
    if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI
        && piccType != MFRC522::PICC_TYPE_MIFARE_1K
        && piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
        Serial.println(F("Требуется только карта MIFARE."));
        return;
    }

    byte blockUID = 5; // Блок, используемый в качестве UID
    byte uidBuffer[18]; // Значение UID
    byte blockAddr = 6; // Блок одноразового ключа
    byte buffer[18];    // Значение ключа
    byte size = sizeof(uidBuffer); // Размеры буферов
    byte sector = 1; // Номер сектора
    byte trailerBlock = 7; // Считывание до какого блока включительно

    MFRC522::StatusCode status;

    // Аутентификация с ключом A для считывания
    Serial.println(F("Аутентификация с ключом А..."));
    status = (MFRC522::StatusCode)mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("PCD_Authenticate() провалено: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        return;
    }

    status = (MFRC522::StatusCode)mfrc522.MIFARE_Read(blockUID, uidBuffer, &size);
    if (status != MFRC522::STATUS_OK) {
        Serial.print("Ошибка чтения: ");
        Serial.println(mfrc522.GetStatusCodeName(status));
        return;
    }

    // Проверка UID
    String dataString = dump_byte(uidBuffer, 16); // первые 16 байтов - это данные UID
    for (uint8_t i = 0; i < sizeof(validUIDs) / sizeof(validUIDs[0]); i++) {
        String validUidString = dump_byte(validUIDs[i], sizeof(validUIDs[i]));
        if (dataString == validUidString) {
            Serial.println("Прочитанные данные совпадают с валидным UID! Дальше!");

            // Показываем текущий вид сектора
            Serial.println(F("Текущая информация в секторе:"));
            mfrc522.PICC_DumpMifareClassicSectorToSerial(&(mfrc522.uid), &key, sector);
            Serial.println();

            byte eepromDataBlock[16];
            for (byte i = 0; i < 16; i++) {
                eepromDataBlock[i] = EEPROM.read(i);
            }
            status = (MFRC522::StatusCode)mfrc522.MIFARE_Read(blockAddr, buffer, &size);
            String dataKey = dump_byte(buffer, 16);
            String eepromKey = dump_byte(eepromDataBlock, 16);
            if (dataKey == eepromKey) {
                // Считываем сигнал для его записи 
                int analogSignal = analogRead(A0);

                // Преобразуем аналоговый сигнал в 16-разрядное число
                uint16_t signal16bit = analogSignal & 0xFFFF;

                // Разбиваем 16-разрядное число на два 8-разрядных числа
                byte highByte = (signal16bit >> 8) & 0xFF;
                byte lowByte = signal16bit & 0xFF;

                // Записываем данные в массив dataBlock
                byte dataBlock[] = {
                    highByte, lowByte, 0x32, 0xd4,
                    0x05, 0x06, 0x07, 0x08,
                    0x09, 0x0a, 0xff, 0x0b,
                    0x0c, 0x0d, 0x0e, 0x0f 
                };
                for (byte i = 0; i < 16; i++) {
                    EEPROM.write(i, dataBlock[i]);
                }
                // Аутентификация ключом B для перезаписи
                Serial.println(F("Аутентификация ключом B..."));
                status = (MFRC522::StatusCode)mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_B, trailerBlock, &key, &(mfrc522.uid));
                if (status != MFRC522::STATUS_OK) {
                    Serial.print(F("PCD_Authenticate() провалено: "));
                    Serial.println(mfrc522.GetStatusCodeName(status));
                    return;
                }

                // Записываем данные в блок
                Serial.print(F("Записываем данные в блок ")); Serial.print(blockAddr);
                Serial.println(F(" ..."));
                dump_byte_array(dataBlock, 16); Serial.println();
                status = (MFRC522::StatusCode)mfrc522.MIFARE_Write(blockAddr, dataBlock, 16);
                if (status != MFRC522::STATUS_OK) {
                    Serial.print(F("MIFARE_Write() провалено: "));
                    Serial.println(mfrc522.GetStatusCodeName(status));
                }
                Serial.println();

                // Считываем обновлённые данные в блоке
                Serial.print(F("Читаем данные из блока ")); Serial.print(blockAddr);
                Serial.println(F(" ..."));
                status = (MFRC522::StatusCode)mfrc522.MIFARE_Read(blockAddr, buffer, &size);
                if (status != MFRC522::STATUS_OK) {
                    Serial.print(F("MIFARE_Read() провалено (2): "));
                    Serial.println(mfrc522.GetStatusCodeName(status));
                }
                Serial.print(F("Данные в блоке ")); Serial.print(blockAddr); Serial.println(F(":"));
                dump_byte_array(buffer, 16); Serial.println();

                // Проверяем записанные нами данные сверкой по количеству битов
                Serial.println(F("Проверяем результат..."));
                byte count = 0;
                for (byte i = 0; i < 16; i++) {
                    // Сравниваем количество записанных данных с количеством считанных
                    if (buffer[i] == dataBlock[i])
                        count++;
                }
                Serial.print(F("Число байтов = ")); Serial.println(count);
                if (count == 16) {
                    Serial.println(F("Данные записаны правильно"));
                } else {
                    Serial.println(F("Ошибка данных"));
                    Serial.println(F("Запись была была осуществлена с ошибкой"));
                }
                Serial.println();                
            } else {
                Serial.println(F("Одноразовый ключ не совпадает, метка клонирована"));
            }   
            break;
        } else {
            Serial.println("Метки нету в базе, отклонено");
        }
    }

    // Выводим информацию из сектора
    Serial.println(F("Текущие данные в секторе:"));
    mfrc522.PICC_DumpMifareClassicSectorToSerial(&(mfrc522.uid), &key, sector);
    Serial.println();
    status = (MFRC522::StatusCode)mfrc522.MIFARE_Read(blockAddr, buffer, &size);
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("MIFARE_Read() провалено (3): "));
        Serial.println(mfrc522.GetStatusCodeName(status));
    } else {
        // Преобразование данных в строку
        String dataStr = "";
        for (byte i = 0; i < 16; i++) {
            if (buffer[i] < 0x10) dataStr += "0";
            dataStr += String(buffer[i], HEX);
        }
        // Выключаем PICC
        mfrc522.PICC_HaltA();
        // Останавливаем дешифрование PCD
        mfrc522.PCD_StopCrypto1();
    }
}
