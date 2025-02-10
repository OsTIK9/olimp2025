#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <LiquidCrystal.h>

// Пины для NRF24L01
#define CE_PIN 9
#define CSN_PIN 8

LiquidCrystal lcd(7, 10, 5, 4, 3, 2);

byte GOOD[8] = {0x00, 0x00, 0x01, 0x02, 0x14, 0x08, 0x00, 0x00}; // Галочка
byte BAD[8] = {0x00, 0x00, 0x11, 0x0a, 0x04, 0x0a, 0x11, 0x00}; // Крестик

const int a = 5;
const int b = 3;
const int m = 26; // Размер алфавита (английский)

// Создаём объект для работы с NRF24L01
RF24 radio(CE_PIN, CSN_PIN);

// Адреса для передачи данных (должны совпадать с транслятором)
const byte address[6] = "00001";

// Буфер для хранения полученных данных
char receivedData[32];

String Temperature = "";
String TempStr = ""; // Временная строка для Temp
String Humidity = "";
String HumStr = "";  // Временная строка для Hum
String Soil = "";
String SlStr = "";  // Временная строка для Sl
String Light = "";
String LtStr = "";   // Временная строка для Lt
int index = 0;

// Функция для нахождения обратного элемента a по модулю m
int modInverse(int a, int m) {
  a = a % m;
  for (int x = 1; x < m; x++) {
    if ((a * x) % m == 1) {
      return x;
    }
  }
  return -1; // Обратный элемент не существует
}

String affineDecrypt(String encryptedText, int a, int b, int m) 
{
  String decryptedText = "";
  int aInverse = modInverse(a, m); // Находим обратный элемент для a
  for (int i = 0; i < encryptedText.length(); i++) 
  {
    if (encryptedText[i] >= 'A' && encryptedText[i] <= 'Z') 
    { // Дешифруем только заглавные буквы
      int y = encryptedText[i] - 'A'; // Преобразуем символ в число (A=0, B=1, ..., Z=25)
      int x = (aInverse * (y - b + m)) % m; // Применяем обратное преобразование
      decryptedText += char(x + 'A'); // Преобразуем число обратно в символ
    } 
    else 
    {
      decryptedText += encryptedText[i]; // Оставляем символы, не являющиеся буквами, без изменений
    }
  }
  return decryptedText;
}


void setup() 
{
  Serial.begin(9600);

  pinMode(6, OUTPUT);
  analogWrite(6, 90); // Установи среднюю контрастность (0-255)
  lcd.begin(16, 2);
  lcd.createChar(0, GOOD);
  lcd.createChar(1, BAD);

  // Инициализация модуля NRF24L01
  if (!radio.begin()) 
  {
    Serial.println("NRF24L01 initialization failed!");
    while (1); // Останавливаем выполнение кода, если модуль не работает
  }

  radio.openReadingPipe(0, address);
  radio.setPALevel(RF24_PA_LOW); // Снижаем уровень мощности для устранения возможных помех
  radio.setDataRate(RF24_250KBPS); // Устанавливаем низкую скорость передачи для надёжности
  radio.setChannel(90); // Устанавливаем канал, совпадающий с транслятором
  radio.startListening(); // Переключаем модуль в режим приёма
//----------------------------------------------------------------------------
  if (radio.isChipConnected()) 
  {
    Serial.println("NRF24L01 is ready for receiving data.");
  } 
  else 
  {
    Serial.println("NRF24L01 is not ready for receiving!");
    while (1); // Останавливаем выполнение, если модуль не готов
  }
}
//----------------------------------------------------------------------------
void loop() 
{
  // Проверяем, пришли ли данные
  if (radio.available()) 
  {
    radio.read(&receivedData, sizeof(receivedData));
    String message = String(receivedData);
    Serial.println("Полученное сообщение: " + message);
    // Очистка переменных перед обработкой нового сообщения
    Temperature = "";
    TempStr = "";
    Humidity = "";
    HumStr = "";
    Soil = "";
    SlStr = "";
    Light = "";
    LtStr = "";
    index = 0;
//----------------------------------------------------------------------------
    // Извлечение Temperature (все буквы до первой цифры)
    while (index < message.length() && !isdigit(message[index])) 
    {
      Temperature += message[index];
      index++;
    }

    // Извлечение Temp (все цифры до первой буквы)
    while (index < message.length() && isdigit(message[index])) 
    {
      TempStr += message[index];
      index++;
    }
    int Temp = TempStr.toInt(); // Преобразуем строку в int
//----------------------------------------------------------------------------
    // Извлечение Humidity (все буквы до следующей цифры)
    while (index < message.length() && !isdigit(message[index])) 
    {
      Humidity += message[index];
      index++;
    }

    // Извлечение Hum (все цифры до следующей буквы)
    while (index < message.length() && isdigit(message[index])) 
    {
      HumStr += message[index];
      index++;
    }
    int Hum = HumStr.toInt(); // Преобразуем строку в int
//----------------------------------------------------------------------------
    // Извлечение Soil (все буквы до следующей цифры)
    while (index < message.length() && !isdigit(message[index])) 
    {
      Soil += message[index];
      index++;
    }

    // Извлечение Sl (все цифры до следующей буквы)
    while (index < message.length() && isdigit(message[index])) 
    {
      SlStr += message[index];
      index++;
    }
    int Sl = SlStr.toInt(); // Преобразуем строку в int
//----------------------------------------------------------------------------
    // Извлечение Light (все буквы до следующей цифры)
    while (index < message.length() && !isdigit(message[index])) 
    {
      Light += message[index];
      index++;
    }

    // Извлечение Lt (все цифры до конца строки)
    while (index < message.length() && isdigit(message[index])) 
    {
      LtStr += message[index];
      index++;
    }
    int Lt = LtStr.toInt(); // Преобразуем строку в int
//----------------------------------------------------------------------------
String decryptedTemperature = affineDecrypt(Temperature, a, b, m);
String decryptedHumidity =    affineDecrypt(Humidity, a, b, m);
String decryptedSoil =        affineDecrypt(Soil, a, b, m);
String decryptedLight =       affineDecrypt(Light, a, b, m);
//----------------------------------------------------------------------------
    Serial.println("Раскодированные данные:");
    // Вывод данных в монитор порта
    Serial.print(decryptedTemperature + ": ");
    Serial.print(Temp);
    Serial.println("°C");

    Serial.print(decryptedHumidity + ": ");
    Serial.print(Hum);
    Serial.println("%");

    Serial.print(decryptedSoil + ": ");
    Serial.print(Sl);
    Serial.println("%");

    Serial.print(decryptedLight + ": ");
    Serial.print(Lt);
    Serial.println("%");

// Очищаем дисплей перед выводом нового текста
  lcd.clear();
//----------------------------------------------------------------------------
  lcd.setCursor(0, 0);
  if (Temp == 999)
  {
    lcd.print(decryptedTemperature + ": Err");
  }
  else if (Temp >= 17 && Temp <= 24)
  {
    lcd.print(decryptedTemperature + ": " + TempStr + "\xDF ");
    lcd.write((uint8_t)0);
    lcd.setCursor(0, 10);
  }
  else
  {
    lcd.print(decryptedTemperature + ": " + TempStr + "\xDF ");
    lcd.write((uint8_t)1);
  }
  lcd.setCursor(0, 1);
  lcd.print("OPTIMAL: 17\xDF\-24\xDF");
  delay(3000);
  lcd.clear();
//----------------------------------------------------------------------------
  lcd.setCursor(0, 0);
  if (Hum == 999)
  {
    lcd.print(decryptedHumidity + ": Err");
  }
  else if (Hum >= 60 && Hum <= 70)
  {
    lcd.print(decryptedHumidity + ": " + HumStr + "% ");
    lcd.write((uint8_t)0);
  }
  else
  {
    lcd.print(decryptedHumidity + ": " + HumStr + "% ");
    lcd.write((uint8_t)1);
  }
  lcd.setCursor(0, 1);
  lcd.print("OPTIMAL: 60%-70%");
  delay(3000);
  lcd.clear();
//----------------------------------------------------------------------------
  lcd.setCursor(0, 0);
  if (Sl == 999)
  {
    lcd.print(decryptedSoil + ": Err");
  }
  else if (Sl >= 70 && Sl <= 80)
  {
    lcd.print(decryptedSoil + ": " + SlStr + "% ");
    lcd.write((uint8_t)0);
  }
  else
  {
    lcd.print(decryptedSoil + ": " + SlStr + "% ");
    lcd.write((uint8_t)1);
  }
  lcd.setCursor(0, 1);
  lcd.print("OPTIMAL: 70%-80%");
  delay(3000);
  lcd.clear();
//----------------------------------------------------------------------------
  lcd.setCursor(0, 0);
  if (Lt == 999)
  {
    lcd.print(decryptedLight + ": Err");
  }
  else if (Lt >= 40 && Lt <= 60)
  {
    lcd.print(decryptedLight + ": "+ LtStr + "% ");
    lcd.write((uint8_t)0);
  }
  else
  {
    lcd.print(decryptedLight + ": " + LtStr + "% ");
    lcd.write((uint8_t)1);
  }
  lcd.setCursor(0, 1);
  lcd.print("OPTIMAL: 40%-60%");
  delay(3000);
  lcd.clear();
  //----------------------------------------------------------------------------
    // Очистка буфера после вывода
    memset(receivedData, 0, sizeof(receivedData)); // Очищаем буфер receivedData
  } 
  else 
  {
    delay(500); // Задержка для снижения нагрузки
  }
}