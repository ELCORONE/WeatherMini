#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <U8g2lib.h>
#include <U8x8lib.h>
#include <Wire.h>
#include <SPI.h>

U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R0);

// Настройка Wi-Fi

const char* ssid     = "SSID";
const char* password = "PASSWORD WIFI";
// Настройка подключений к серверу погоды
const String server  = "api.openweathermap.org";
const String lat     = "43.7986";     // Координата широны
const String lon     = "131.9508";    // Координата долготы
const String appid   = "KEY API OWM"; // Ключ API на OpenWeatherMap
const String url     = "http://" + server + "/data/2.5/weather?lat=" + lat + "&lon=" + lon + "&units=metric&appid=" + appid;
// Таймеры
unsigned long lastConnectionTime = 0;
unsigned long postingInterval = 5 * 60 * 1000; // Период обновление погоды (5 минут * 60 секунд * 1000)
unsigned long changedisplaytime = 0;
byte dFlag = 0;

// Переменные для получения данных с JSON
String httpData;
String main;
float temp;
int pres;
int  hum;
String tHour, tMinute;
// Получение времени с NTP-сервера
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "ntp0.ntp-servers.net", 36000, 3600000);

void setup() {
  Wire.pins(2, 0);             // Изменение адреса I²C
  Wire.begin();                // Подключение SSD1306 128x32
  u8g2.begin();                // Запуск библиотеки шрифтов
  Serial.begin(115200);        // Пуск последовательного порта на скорости 115200
  Serial.print("Подключение к Wi-Fi:");
  Serial.println(ssid);
  wifi_station_set_hostname("OpenWeatherESP"); // Имя модуля в локальной сети
  WiFi.begin(ssid, password);  // Установка логина и пароля для подключения
  // Подключение к сети Wi-Fi с заданными параметрами
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print("$");
  }
  Serial.println("Подключение успешное");
  timeClient.begin(); // Получение времени с NTP-сервера
}

void loop() {
  set_Time();
  GetWeather();
  DisplayDraw();
}

void DisplayDraw() {
  if (millis() - changedisplaytime > 5000) {
    switch (dFlag) {
      case 0: {
          u8g2.clearBuffer();
          u8g2.setFont(u8g2_font_open_iconic_all_2x_t);
          u8g2.drawStr(16, 16, "\xC9"); // Иконка погоды
          u8g2.setFont(u8g2_font_7x13_mf);
          u8g2.setCursor(0, 29);
          u8g2.print(main);
          Serial.println(main);
          u8g2.setFont(u8g2_font_inr16_mr);
          u8g2.setCursor(48, 24);
          u8g2.print(temp);
          u8g2.sendBuffer();
          dFlag = 1;
        }
        break;
      case 1: {
          u8g2.clearBuffer();
          u8g2.setFont(u8g2_font_open_iconic_all_4x_t);
          u8g2.drawStr(0, 32, "\x98"); // Иконка капельки
          u8g2.setFont(u8g2_font_inr16_mr);
          u8g2.setCursor(48, 24);
          u8g2.print(String(hum) + " %");
          u8g2.sendBuffer();
          dFlag = 2;
        }
        break;
      case 2:
        {
          u8g2.clearBuffer();
          u8g2.setFont(u8g2_font_open_iconic_all_4x_t);
          u8g2.drawStr(0, 32, "{"); // Иконка часов
          u8g2.setFont(u8g2_font_inr16_mr);
          u8g2.setCursor(48, 24);
          u8g2.print(tHour + ":" + tMinute);
          Serial.println(tHour + ":" + tMinute);
          u8g2.sendBuffer();
          dFlag = 0;
        }
        break;
    }
    changedisplaytime = millis();
  }
}

void set_Time() {
  timeClient.update();
  tHour     = timeClient.getHours();
  tMinute   = timeClient.getMinutes() < 10 ? "0" + String(timeClient.getMinutes()) : String(timeClient.getMinutes());
}

// Получение данных о погоде
void GetWeather() {
  if (millis() - lastConnectionTime > postingInterval or lastConnectionTime == 0) {
    HTTPClient client;
    Serial.println("Подключение ");
    client.begin(url);
    int httpCode = client.GET();
    if (httpCode > 0) {
      Serial.printf("\n успешно. Код подтверждения: %d\n", httpCode);
      if (httpCode == HTTP_CODE_OK) {
        httpData = client.getString();
        if (httpData.indexOf(F("\"main\":{\"temp\":")) > -1) {
        }
        else Serial.println("Не удалось получить ответ от сервера.");
      }
    }
    else Serial.printf("не удалось, ошибка: %s\n", client.errorToString(httpCode).c_str());
    client.end();
    Serial.println(httpData);
    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(httpData);
    if (!root.success()) {
      Serial.println("Ошибка чтения строки ответа.");
    }
    main     = root["weather"][0]["main"].as<String>();
    temp     = root["main"]["temp"];
    hum      = root["main"]["humidity"];
    pres     = root["main"]["pressure"];
    pres     = pres * 0.75; // Переводим давление ГектоПаскалей в мм.рт.столба
    httpData = "";
    lastConnectionTime = millis();
  }
}
