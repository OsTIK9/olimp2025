#include <DHT.h> // Библиотека для работы с датчиком DHT11
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

// Параметры для датчика температуры
DHT dht(10, DHT11); // Передаём номер пина, к которому подключён датчик, и тип датчика

// Пины для датчика влажности почвы
#define sensorPower 7
#define sensorPin A1

// Пин для фоторезистора
#define lightSensorPin A0

// Пины для NRF24L01
#define CE_PIN 9
#define CSN_PIN 8

// Создаём объект для работы с NRF24L01
RF24 radio(CE_PIN, CSN_PIN);

// Адреса для передачи данных (можно использовать любой 5-байтовый адрес)
const byte address[6] = "00001";

// Переменные для меток
String labelTemperature = "TEMP";
String labelSoil = "SOIL";
String labelLight = "LIGHT";
String labelHum = "HUM";

// Ключи для афинного шифра
const int a = 5;
const int b = 3;
const int m = 26; // Размер алфавита (английский)

// Переменные для хранения данных
String Temp = ""; // Температура
String Hum = "";  // Влажность воздуха
String Soil = ""; // Влажность почвы
String Light = ""; // Освещённость

// Функция для афинного шифрования
String affineEncrypt(String text, int a, int b, int m) 
{
  String encryptedText = "";
  for (int i = 0; i < text.length(); i++) 
  {
    if (text[i] >= 'A' && text[i] <= 'Z') 
    { // Шифруем только заглавные буквы
      int x = text[i] - 'A'; // Преобразуем символ в число (A=0, B=1, ..., Z=25)
      int y = (a * x + b) % m; // Применяем афинный шифр
      encryptedText += char(y + 'A'); // Преобразуем число обратно в символ
    } 
    else 
    {
      encryptedText += text[i]; // Оставляем символы, не являющиеся буквами, без изменений
    }
  }
  return encryptedText;
}

void setup() 
{
  // Настраиваем датчики
  Serial.begin(9600);
  
  // Инициализация датчика DHT11
  dht.begin();
  
  // Настраиваем пин для питания датчика влажности почвы
  pinMode(sensorPower, OUTPUT);
  digitalWrite(sensorPower, LOW); // Сначала выключаем датчик
  
  // Инициализация модуля NRF24L01
  if (!radio.begin()) 
  {
    Serial.println("NRF24L01 initialization failed!");
    while (1); // Останавливаем выполнение, если модуль не работает
  }
  radio.openWritingPipe(address);
  radio.setPALevel(RF24_PA_LOW); // Снижаем уровень мощности для устранения возможных помех
  radio.setDataRate(RF24_250KBPS); // Устанавливаем низкую скорость передачи для надёжности
  radio.setChannel(90); // Изменяем частоту канала для минимизации помех
  radio.stopListening(); // Переключаем модуль в режим передачи

  Serial.println("NRF24L01 is ready for trasmitting data");
}

void loop() 
{
  // Проверка подключения и чтение данных с датчиков
  checkDHT11();
  checkSoilMoisture();
  checkPhotoresistor();
  // ----- Шифрование меток -----
  String encryptedLabelTemperature = affineEncrypt(labelTemperature, a, b, m);
  String encryptedLabelHum = affineEncrypt(labelHum, a, b, m);
  String encryptedLabelSoil = affineEncrypt(labelSoil, a, b, m);
  String encryptedLabelLight = affineEncrypt(labelLight, a, b, m);

  // ----- Вывод данных -----
  Serial.println("Исходные данные | Закодированные данные");

  Serial.print(labelTemperature + ": " + Temp + "°C      | ");
  Serial.println(encryptedLabelTemperature + Temp);

  Serial.print(labelHum + ": " + Hum + "%        | ");
  Serial.println(encryptedLabelHum + Hum);

  Serial.print(labelSoil + ": " + Soil + "%       | ");
  Serial.println(encryptedLabelSoil + Soil);

  Serial.print(labelLight + ": " + Light + "%       | ");
  Serial.println(encryptedLabelLight + Light);

  // ----- Формирование данных -----
  String dataToSend = encryptedLabelTemperature + Temp + 
                      encryptedLabelHum + Hum +
                      encryptedLabelSoil + Soil + 
                      encryptedLabelLight + Light;

  Serial.println("Отправляемое сообщение: " + dataToSend);

  // Преобразование строки в массив char для передачи
  char data[dataToSend.length() + 1];
  dataToSend.toCharArray(data, sizeof(data));

  // Отправка данных по радиоканалу
  bool success = radio.write(&data, sizeof(data));

  // Задержка перед следующим измерением
  delay(11500);
}
//-------------------------------------------------------------------------------
//Функция для проверки температуры и влажности воздуха DHT11
bool checkDHT11() 
{
  int humidity = dht.readHumidity();     // Чтение влажности
  int temperature = dht.readTemperature();// Чтение температуры

  if (humidity == 0 && temperature == 0)
  {
    Hum = "999"; // Очищаем значение, если данные не удалось прочитать
    Temp = "999";
    return false;
  } 
  else 
  {
    Hum = String(humidity); // Записываем значение влажности
    Temp = String(temperature); // Записываем значение температуры
    return true;
  }
}
//-------------------------------------------------------------------------------
// Функция для проверки фоторезистора
bool checkPhotoresistor() 
{
  int photoresistorValue = analogRead(lightSensorPin);

  if (photoresistorValue <= 10) 
  {
    Light = "999"; // Очищаем значение, если данные не удалось прочитать
    return false;
  } 
  else 
  {
    pinMode(lightSensorPin, INPUT);
    Light = String(map(photoresistorValue, 0, 1023, 100, 0)); // Записываем значение освещённости
    return true;
  }
}
//-------------------------------------------------------------------------------
// Функция для проверки датчика влажности почвы FC-28
bool checkSoilMoisture() 
{
  int soilMoistureValue = readSoil();
  // Расширенная проверка подключения
  if (soilMoistureValue <= 10) 
  {
    Soil = "999"; // Очищаем значение, если данные не удалось прочитать
    return false;
  } 
  else 
  {
    Soil = String(map(soilMoistureValue, 0, 1023, 100, 0)); // Записываем значение влажности почвы
    return true;
  }
}
//-------------------------------------------------------------------------------
// Функция для считывания влажности почвы
int readSoil() 
{
  digitalWrite(sensorPower, HIGH); // Включаем питание датчика
  delay(10); // Даём время стабилизироваться
  int value = analogRead(sensorPin); // Читаем значение
  digitalWrite(sensorPower, LOW); // Отключаем питание датчика
  return value;
}