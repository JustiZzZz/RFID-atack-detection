#include <SPI.h>
#include <MFRC522.h>

#define RST_PIN         9           // Конфигурируемый, см. типичную схему подключения выше
#define SS_PIN          10          // Конфигурируемый, см. типичную схему подключения выше

MFRC522 mfrc522(SS_PIN, RST_PIN);   // Создаем экземпляр MFRC522.

byte buffer[18];
byte block;
byte waarde[64][16];
MFRC522::StatusCode status;
    
MFRC522::MIFARE_Key key;

// Количество известных стандартных ключей (жестко закодированные)
#define NR_KNOWN_KEYS   8
byte knownKeys[NR_KNOWN_KEYS][MFRC522::MF_KEY_SIZE] =  {
    {0xff, 0xff, 0xff, 0xff, 0xff, 0xff}, // FF FF FF FF FF FF = заводской стандарт
    {0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5}, // A0 A1 A2 A3 A4 A5
    {0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5}, // B0 B1 B2 B3 B4 B5
    {0x4d, 0x3a, 0x99, 0xc3, 0x51, 0xdd}, // 4D 3A 99 C3 51 DD
    {0x1a, 0x98, 0x2c, 0x7e, 0x45, 0x9a}, // 1A 98 2C 7E 45 9A
    {0xd3, 0xf7, 0xd3, 0xf7, 0xd3, 0xf7}, // D3 F7 D3 F7 D3 F7
    {0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff}, // AA BB CC DD EE FF
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}  // 00 00 00 00 00 00
};

char choice;
/*
 * Инициализация.
 */
void setup() {
    Serial.begin(9600);         // Инициализация последовательной связи с ПК
    while (!Serial);            // Ничего не делать, если последовательный порт не открыт
    SPI.begin();                // Инициализация шины SPI
    mfrc522.PCD_Init();         // Инициализация карты MFRC522
    Serial.println(F("Попытка использования наиболее часто используемых стандартных ключей для чтения блоков 0-63 карты MIFARE."));
    Serial.println("1.Прочитать карту \n2.Записать на карту \n3.Скопировать данные.");

    for (byte i = 0; i < 6; i++) {
        key.keyByte[i] = 0xFF;
    }
}

//Чтение байтов в шестнадцатеричном формате через серийный монитор
 
void dump_byte_array(byte *buffer, byte bufferSize) {
    for (byte i = 0; i < bufferSize; i++) {
        Serial.print(buffer[i] < 0x10 ? " 0" : " ");
        Serial.print(buffer[i], HEX);
    }
}

//Чтение байтов в формате ASCII через серийный монитор

void dump_byte_array1(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.write(buffer[i]);
  }
}

/*
 * Попытка использовать PICC (метку/карту) с заданным ключом для доступа к блокам 0-63.
 * При успехе будут показаны детали ключа и данные блока будут выведены в сериал.
 *
 * @return true если ключ сработал, иначе false.
 */
 
bool try_key(MFRC522::MIFARE_Key *key)
{
    bool result = false;
    
    for(byte block = 0; block < 64; block++){
      
    // Serial.println(F("Аутентификация с использованием ключа A..."));
    status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, key, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("PCD_Authenticate() не удалось: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        return false;
    }

    // Чтение блока
    byte byteCount = sizeof(buffer);
    status = mfrc522.MIFARE_Read(block, buffer, &byteCount);
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("MIFARE_Read() не удалось: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
    }
    else {
        // Успешное чтение
        result = true;
        Serial.print(F("Успех с ключом:"));
        dump_byte_array((*key).keyByte, MFRC522::MF_KEY_SIZE);
        Serial.println();
        
        // Вывод данных блока
        Serial.print(F("Блок ")); Serial.print(block); Serial.print(F(":"));
        dump_byte_array1(buffer, 16); //Преобразование из HEX в ASCII
        Serial.println();
        
        for (int p = 0; p < 16; p++) //Чтение 16 бит из блока
        {
          waarde [block][p] = buffer[p];
          Serial.print(waarde[block][p]);
          Serial.print(" ");
        }
        
        }
    }
    Serial.println();
    
    Serial.println("1.Прочитать карту \n2.Записать на карту \n3.Скопировать данные.");

    mfrc522.PICC_HaltA();       // Остановка PICC
    mfrc522.PCD_StopCrypto1();  // Остановка шифрования на PCD
    return result;
    
    start();
}

/*
 * Основной цикл.
 */
void loop() {
  start();
    
}

void start(){
  choice = Serial.read();
  
  if(choice == '1')
  {
    Serial.println("Чтение карты");
    keuze1();
      
    }
    else if(choice == '2')
    {
      Serial.println("Проверка данных в переменных");
      keuze2();
    }
    else if(choice == '3')
    {
      Serial.println("Копирование данных на новую карту");
      keuze3();
    }
}

void keuze2(){ //Тестирование значений в блоках
  
  for(block = 4; block <= 62; block++){
    if(block == 7 || block == 11 || block == 15 || block == 19 || block == 23 || block == 27 || block == 31 || block == 35 || block == 39 || block == 43 || block == 47 || block == 51 || block == 55 || block == 59){
      block ++;
    }
  
  Serial.print(F("Запись данных в блок ")); 
  Serial.print(block);
  Serial.println("\n");
  
    for(int j = 0; j < 16; j++){
      Serial.print(waarde[block][j]);
      Serial.print(" ");
    }
    Serial.println("\n");
    
  }
  
  Serial.println("1.Прочитать карту \n2.Записать на карту \n3.Скопировать данные.");
  start();
}

void keuze3(){ //Копирование данных на новую карту
Serial.println("Вставьте новую карту...");
  // Поиск новых карт
    if ( ! mfrc522.PICC_IsNewCardPresent())
        return;

    // Выбор одной из карт
    if ( ! mfrc522.PICC_ReadCardSerial())
        return;

    // Показать некоторые детали карты
    Serial.print(F("UID карты:"));
    dump_byte_array(mfrc522.uid.uidByte, mfrc522.uid.size);
    Serial.println();
    Serial.print(F("Тип PICC: "));
    MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
    Serial.println(mfrc522.PICC_GetTypeName(piccType));
    
    // Попытка использовать известные стандартные ключи
    /*MFRC522::MIFARE_Key key;
    for (byte k = 0; k < NR_KNOWN_KEYS; k++) {
        // Копирование известного ключа в структуру MIFARE_Key
        for (byte i = 0; i < MFRC522::MF_KEY_SIZE; i++) {
            key.keyByte[i] = knownKeys[k][i];
        }
    }*/
    for (byte i = 0; i < 6; i++) {
        key.keyByte[i] = 0xFF;
    }

  for(int i = 4; i <= 62; i++){ //Копирование блоков 4 до 62, кроме следующих блоков (поскольку это блоки аутентификации)
    if(i == 7 || i == 11 || i == 15 || i == 19 || i == 23 || i == 27 || i == 31 || i == 35 || i == 39 || i == 43 || i == 47 || i == 51 || i == 55 || i == 59){
      i++;
    }
    block = i;
    
      // Аутентификация с использованием ключа A
    Serial.println(F("Аутентификация с использованием ключа A..."));
    status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("PCD_Authenticate() не удалось: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        return;
    }
    
    // Аутентификация с использованием ключа B
    Serial.println(F("Повторная аутентификация с использованием ключа B..."));
    status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_B, block, &key, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("PCD_Authenticate() не удалось: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        return;
    }
    
    // Запись данных в блок
    Serial.print(F("Запись данных в блок ")); 
    Serial.print(block);
    Serial.println("\n");
          
    dump_byte_array(waarde[block], 16); 
    
          
     status = (MFRC522::StatusCode) mfrc522.MIFARE_Write(block, waarde[block], 16);
      if (status != MFRC522::STATUS_OK) {
        Serial.print(F("MIFARE_Write() не удалось: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
      }
    
        
     Serial.println("\n");
     
  }
  mfrc522.PICC_HaltA();       // Остановка PICC
  mfrc522.PCD_StopCrypto1();  // Остановка шифрования на PCD
  
  Serial.println("1.Прочитать карту \n2.Записать на карту \n3.Скопировать данные.");
  start();
}

void keuze1(){ //Чтение карты
  Serial.println("Вставьте карту...");
  // Поиск новых карт
    if ( ! mfrc522.PICC_IsNewCardPresent())
        return;

    // Выбор одной из карт
    if ( ! mfrc522.PICC_ReadCardSerial())
        return;

    // Показать некоторые детали карты
    Serial.print(F("UID карты:"));
    dump_byte_array(mfrc522.uid.uidByte, mfrc522.uid.size);
    Serial.println();
    Serial.print(F("Тип PICC: "));
    MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
    Serial.println(mfrc522.PICC_GetTypeName(piccType));
    
    // Попытка использовать известные стандартные ключи
    MFRC522::MIFARE_Key key;
    for (byte k = 0; k < NR_KNOWN_KEYS; k++) {
        // Копирование известного ключа в структуру MIFARE_Key
        for (byte i = 0; i < MFRC522::MF_KEY_SIZE; i++) {
            key.keyByte[i] = knownKeys[k][i];
        }
        // Попытка использования ключа
        if (try_key(&key)) {
            // Найден и сообщен ключ и блок,
            // нет необходимости пытаться использовать другие ключи для этой карты
            break;
        }
    }
}
