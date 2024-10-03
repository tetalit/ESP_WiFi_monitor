#include <Adafruit_GFX.h> // графическая библиотека
#include <Adafruit_ST7735.h> // библиотека для дисплея
#include <SPI.h>
#include <WiFi.h> //Библиотека для работы с wifi
#include <HTTPClient.h> //библиотека для отправки http запросов серверу
#include <ArduinoJson.h>//библиотека для парсинга JSON
#include <WiFiClient.h> //необходима для работы http client по wifi
#include "bitmap.h"

#define CODE_VERS  "2.0.0"
#define TFT_CS         15
#define TFT_RST        4
#define TFT_DC         2

const String clientIP = "http://192.168.0.163:8085/data.json";// указываем адрес сервера с данными
//http://192.168.1.108:8085/data.json - завод


//смена цветов bitmap-a в зависимости от темпиратуры комплектующей---------------------

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

const char* ssid = "aaa 2.4G"; // имя сети upm
const char* password = "221C2E0000064"; // пароль upm12345

void setup() {
  Serial.begin(115200);

  //инициализация  экрана
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(1);  
  Serial.println(F("screen initialized"));
  tft.fillScreen(ST77XX_BLACK);

  //tft.drawRect(1,1,127,159,ST77XX_GREEN);

  splashScreenHorizontal();

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("conecting to Wi-Fi");
    tft.setTextSize(1);
    tft.setCursor(25, 60);
    tft.println("connecting to Wi-Fi");
  }
  Serial.println("WiFi connectd");
  WiFiClient client; //инициализируем клиента для передачи данных по wifi
  HTTPClient http; //Инициализируем библиотеку httpclient
  tft.fillScreen(ST77XX_BLACK);
  //tft.drawRect(1,1,127,159,ST77XX_GREEN);
}

void loop() {
  displayChange();
  //delay(60000);
  /*
    //hardwareMonitor();
    getCpuInfo();
    delay(3000);
    tft.fillScreen(ST77XX_BLACK);
    getGpuInfo();
    delay(3000);
    tft.fillScreen(ST77XX_BLACK);
    getRamInfo();
    delay(3000);
    tft.fillScreen(ST77XX_BLACK);
    //tft.fillRect(85, 18, 75, 110, ST77XX_BLACK);
  */
}

//Функция получения параметров ПК
void hardwareMonitor() {
  WiFiClient client; //инициализируем клиента для передачи данных по wifi
  HTTPClient http; //Инициализируем библиотеку httpclient
  // Отправляем запрос
  http.useHTTP10(true); //устанвливаем версию http протокола 1.1
  http.begin(client, "http://192.168.1.108:8085/data.json"); //указываем библиотеке через какой интерфейс отправлять данные и на какой адрес
  //Здесь проверяем доступен ли воообще JSON по указанному нами адресу
  int httpCode = http.GET(); //отправляем запрос и получаем код ответа

  // httpCode равен -1 при ошибке
  if (httpCode > 0) {
    // HTTP запрос отправлен
    // файл найден на сервере
    if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
    }
  } else {
    Serial.printf("sensors http sync failed, error: %s\n", http.errorToString(httpCode).c_str()); //содержимое файла не может быть прочитано
    return;
  }

  if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) { //если файл получен начинаем обработку
    //фильтруем содержимое для экономии памяти микроконтроллера
    StaticJsonDocument<224> filter;//создаем вспомогательный Json документ для фильтрации получаемых значений

    JsonObject filter_Children_0_Children_0 = filter["Children"][0]["Children"].createNestedObject();
    filter_Children_0_Children_0["Text"] = true;

    JsonObject filter_Children_0_Children_0_Children_0_Children_0 = filter_Children_0_Children_0["Children"][0]["Children"].createNestedObject();
    filter_Children_0_Children_0_Children_0_Children_0["Text"] = true;
    filter_Children_0_Children_0_Children_0_Children_0["Value"] = true;
    filter_Children_0_Children_0_Children_0_Children_0["Children"][0]["Value"] = true;
    // непосредственно парсинг основоного JSON.
    DynamicJsonDocument doc(18000); //выделяем память
    deserializeJson(doc, http.getStream(), DeserializationOption::Filter(filter), DeserializationOption::NestingLimit(12));//читаем файл и применяем фильтр
    doc.shrinkToFit();//освобождаем неиспользованную память
    //Присваиваем значения переменным
    String cpuName = doc["Children"][0]["Children"][1]["Text"];//название процессора
    String cpuNamePars2 = cpuName.substring(-6, cpuName.length() - 8);
    String cpuTempPackage = doc["Children"][0]["Children"][0]["Children"][0]["Children"][1]["Children"][0]["Value"];//температура процессора
    cpuTempPackage = cpuTempPackage.substring(0, cpuTempPackage.length() - 3);
    String cpuLoad = doc["Children"][0]["Children"][1]["Children"][4]["Children"][0]["Value"];//загруженность цп %
    String cpuClock = doc["Children"][0]["Children"][1]["Children"][2]["Children"][1]["Value"]; //частота 1ого ядра цп
    String cpuClockPars = cpuClock.substring(0, cpuClock.length() - 4);
    String gpuName = doc["Children"][0]["Children"][3]["Text"];//название видеокарты
    String gpuName2 = gpuName.substring(-7, gpuName.length()); // без надписи nvidia
    String gpuCoreClock = doc["Children"][0]["Children"][3]["Children"][0]["Children"][0]["Value"];
    String gpuCoreClockPars = gpuCoreClock.substring(0, gpuCoreClock.length() - 3);
    String gpuHotSpot = doc["Children"][0]["Children"][3]["Children"][1]["Children"][0]["Value"];//температура видеокарты
    gpuHotSpot = gpuHotSpot.substring(0, gpuHotSpot.length() - 3);
    String gpuLoad = doc["Children"][0]["Children"][3]["Children"][2]["Children"][0]["Value"];//загруженность видеокарты
    String usedRAM = doc["Children"][0]["Children"][2]["Children"][1]["Children"][0]["Value"];//использованная память ОЗУ
    String freeRAM = doc["Children"][0]["Children"][2]["Children"][1]["Children"][1]["Value"];//свободная память ОЗУ
    String loadRAM = doc["Children"][0]["Children"][2]["Children"][0]["Children"][0]["Value"]; //загрузка озу в %
    String gpuRAMused = doc["Children"][0]["Children"][3]["Children"][5]["Children"][1]["Value"];//Использованная видеопамять
    String gpuRAM = doc["Children"][0]["Children"][3]["Children"][5]["Children"][0]["Value"];//свободная видеопамять
    String cpuFANpercent = doc["Children"][0]["Children"][0]["Children"][0]["Children"][3]["Children"][0]["Value"]; //скорость работы вентилятор процессора в процентах
    String gpuFANpercent = doc["Children"][0]["Children"][3]["Children"][4]["Children"][0]["Value"];//скорость работы вентилятора видеокарты в процентах
    String uploadSpeed = doc["Children"][0]["Children"][12]["Children"][2]["Children"][0]["Value"];//скорость передачи данных по сети
    String downloadSpeed = doc["Children"][0]["Children"][12]["Children"][2]["Children"][1]["Value"];//скорость загрузки данных по сети
    String cpuFAN = doc["Children"][0]["Children"][0]["Children"][0]["Children"][2]["Children"][1]["Value"];
    //Serial.println(cpuName);
    Serial.println(cpuNamePars2);
    Serial.println(cpuTempPackage);
    Serial.println(cpuClock);
    Serial.println(cpuLoad);
    Serial.println(gpuName2);
    Serial.println(gpuCoreClock);
    Serial.println(gpuHotSpot);
    Serial.println(gpuLoad);
    Serial.println(usedRAM);
    Serial.println(freeRAM);
    Serial.println(loadRAM);
    Serial.println(gpuRAMused);
    Serial.println(gpuRAM);
    Serial.println(cpuFANpercent);
    Serial.println(gpuFANpercent);
    Serial.println(uploadSpeed);
    Serial.println(downloadSpeed);
    Serial.println(cpuFAN);
    Serial.println("--------------------------");
    //--------------CPU----------------
    /*
      tft.drawBitmap(1,22, cpuBigBMP, 80, 80,ST77XX_WHITE);
      tft.setCursor(13, 56);
      tft.setTextSize(1);
      tft.println(cpuNamePars2);
      tft.setTextSize(2);
      tft.setCursor(85, 20);
      tft.println(cpuClockPars);
      tft.setCursor(85, 60);
      tft.println(cpuTempPackage);tft.setCursor(135, 60); tft.print("C");
      tft.setCursor(85, 100);
      tft.println(cpuLoad);

      tft.drawBitmap(5,20, cpuBMP, 32, 32,ST77XX_WHITE);
      tft.setTextSize(1);
      tft.setCursor(10, 5);
      tft.println(cpuName);
      //tft.setCursor(5,20); tft.print("Clock:");
      tft.setCursor(55, 20);
      tft.println(cpuClock);
      //tft.setCursor(5,30); tft.print("Temp:");
      tft.setCursor(55, 30);
      tft.println(cpuTempPackage); tft.setCursor(85, 30); tft.print("C");
      //tft.setCursor(5,40); tft.print("Load:");
      tft.setCursor(55, 40);
      tft.println(cpuLoad);
    */

    //--------------GPU----------------
    /*
      tft.drawBitmap(1,50, gpuBigBMP, 79, 36,ST77XX_WHITE);
      tft.setCursor(1, 1);
      tft.setTextSize(1);
      tft.println(gpuName);
      tft.setTextSize(2);
      tft.setCursor(85, 20);
      tft.println(gpuCoreClockPars);
      tft.setCursor(85, 60);
      tft.println(gpuHotSpot);tft.setCursor(135, 60); tft.print("C");
      tft.setCursor(85, 100);
      tft.println(gpuLoad);
    */

    /*
      tft.setTextSize(1);
      tft.drawBitmap(3,80, gpu1BMP, 42, 26,ST77XX_WHITE);
      tft.setCursor(10, 65);
      tft.println(gpuName);
      //tft.setCursor(5,80); tft.print("Clock:");
      tft.setCursor(55, 80);
      tft.println(gpuCoreClock);
      //tft.setCursor(5,90); tft.print("Temp:");
      tft.setCursor(55, 90);
      tft.println(gpuHotSpot); tft.setCursor(85, 90); tft.print("C");
      //tft.setCursor(5,100); tft.print("Load:");
      tft.setCursor(55,100);
      tft.println(gpuLoad);
    */
    //--------------RAM----------------
    /*
      tft.drawBitmap(5,130, ram2BMP, 36, 18,ST77XX_WHITE);
      tft.setCursor(55, 120);
      tft.println("RAM");
      //tft.setCursor(5,125); tft.print("Load:");
      tft.setCursor(55, 135);
      tft.println(loadRAM);
    */
  }
  http.end();//закрываем http соединение
}
/*
    uint16_t interpolateColor(uint8_t value) {
    uint8_t green = map(value, 0, 255, 255, 0);
    uint8_t red = map(value, 0, 255, 0, 255);

    return tft.color565(red, green, 0); // Создаем цвет в формате RGB565
  }
*/
void getGpuInfo() {
  WiFiClient client; //инициализируем клиента для передачи данных по wifi
  HTTPClient http; //Инициализируем библиотеку httpclient
  http.useHTTP10(true); //устанвливаем версию http протокола 1.1
  http.begin(client, clientIP); //указываем библиотеке через какой интерфейс отправлять данные и на какой адрес
  //Здесь проверяем доступен ли воообще JSON по указанному нами адресу
  int httpCode = http.GET(); //отправляем запрос и получаем код ответа

  // httpCode равен -1 при ошибке
  if (httpCode > 0) {
    // HTTP запрос отправлен
    // файл найден на сервере
    if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
    }
  } else {
    Serial.printf("sensors http sync failed, error: %s\n", http.errorToString(httpCode).c_str()); //содержимое файла не может быть прочитано
    return;
  }

  if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) { //если файл получен начинаем обработку
    //фильтруем содержимое для экономии памяти микроконтроллера
    StaticJsonDocument<224> filter;//создаем вспомогательный Json документ для фильтрации получаемых значений

    JsonObject filter_Children_0_Children_0 = filter["Children"][0]["Children"].createNestedObject();
    filter_Children_0_Children_0["Text"] = true;

    JsonObject filter_Children_0_Children_0_Children_0_Children_0 = filter_Children_0_Children_0["Children"][0]["Children"].createNestedObject();
    filter_Children_0_Children_0_Children_0_Children_0["Text"] = true;
    filter_Children_0_Children_0_Children_0_Children_0["Value"] = true;
    filter_Children_0_Children_0_Children_0_Children_0["Children"][0]["Value"] = true;
    // непосредственно парсинг основоного JSON.
    DynamicJsonDocument doc(18000); //выделяем память
    deserializeJson(doc, http.getStream(), DeserializationOption::Filter(filter), DeserializationOption::NestingLimit(12));//читаем файл и применяем фильтр
    doc.shrinkToFit();//освобождаем неиспользованную память
    String gpuName = doc["Children"][0]["Children"][3]["Text"];//название видеокарты
    String gpuName2 = gpuName.substring(7); // без надписи nvidia
    String gpuName3 = gpuName.substring(0, 6); // надпись nvidia
    String gpuCoreClock = doc["Children"][0]["Children"][3]["Children"][0]["Children"][0]["Value"];
    String gpuCoreClockPars = gpuCoreClock.substring(0, gpuCoreClock.length() - 3);
    String gpuHotSpot = doc["Children"][0]["Children"][3]["Children"][1]["Children"][0]["Value"];//температура видеокарты
    int HotSpotToInt = gpuHotSpot.toInt();
    gpuHotSpot = gpuHotSpot.substring(0, gpuHotSpot.length() - 3);
    String gpuLoad = doc["Children"][0]["Children"][3]["Children"][2]["Children"][0]["Value"];//загруженность видеокарты
    String gpuLoad2 = gpuLoad.substring(0, gpuLoad.length() - 1);


    if (HotSpotToInt >= 10 && HotSpotToInt < 45) {
      tft.drawBitmap(3, 50, gpuBigBMP, 79, 36, ST77XX_GREEN);
    }
    if (HotSpotToInt >= 45 && HotSpotToInt < 65) {
      tft.drawBitmap(3, 50, gpuBigBMP, 79, 36, ST77XX_YELLOW);
    }
    if (HotSpotToInt >= 65 && HotSpotToInt < 72) {
      tft.drawBitmap(3, 50, gpuBigBMP, 79, 36, ST77XX_ORANGE);
    }
    if (HotSpotToInt >= 72) {
      tft.drawBitmap(3, 50, gpuBigBMP, 79, 36, ST77XX_RED);
    }

    tft.setCursor(8, 30);
    tft.setTextSize(1);
    tft.println(gpuName2);
    tft.setCursor(80, 5);
    tft.println("GPU");
    tft.setTextSize(2);
    tft.setCursor(85, 20);
    tft.println(gpuCoreClockPars);
    tft.setCursor(85, 60);
    tft.println(gpuHotSpot); tft.setCursor(135, 60); tft.print("C");
    tft.setCursor(85, 100);
    tft.println(gpuLoad2);   tft.setCursor(145, 100); tft.print("%");

    /*
      for(int i = 0; i < gpuBigBmp_width; i++) {
        for(int j = 0; j < gpuBigBmp_height; j++) {
            int pixelValue = pgm_read_byte(&gpuBigBMP[i + j * gpuBigBmp_width]); // Получение значения пикселя из PROGMEM массива

            uint16_t color = interpolateColor(pixelValue);

            tft.drawPixel(i + 1, j + 50, color); // Рисуем пиксель с плавным изменением цвета
        }
      }
    */
  }
  http.end();//закрываем http соединение
}

void getCpuInfo() {
  WiFiClient client; //инициализируем клиента для передачи данных по wifi
  HTTPClient http; //Инициализируем библиотеку httpclient
  http.useHTTP10(true); //устанвливаем версию http протокола 1.1
  http.begin(client, clientIP); //указываем библиотеке через какой интерфейс отправлять данные и на какой адрес
  //Здесь проверяем доступен ли воообще JSON по указанному нами адресу
  int httpCode = http.GET(); //отправляем запрос и получаем код ответа

  // httpCode равен -1 при ошибке
  if (httpCode > 0) {
    // HTTP запрос отправлен
    // файл найден на сервере
    if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
    }
  } else {
    Serial.printf("sensors http sync failed, error: %s\n", http.errorToString(httpCode).c_str()); //содержимое файла не может быть прочитано
    return;
  }

  if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) { //если файл получен начинаем обработку
    //фильтруем содержимое для экономии памяти микроконтроллера
    StaticJsonDocument<224> filter;//создаем вспомогательный Json документ для фильтрации получаемых значений

    JsonObject filter_Children_0_Children_0 = filter["Children"][0]["Children"].createNestedObject();
    filter_Children_0_Children_0["Text"] = true;

    JsonObject filter_Children_0_Children_0_Children_0_Children_0 = filter_Children_0_Children_0["Children"][0]["Children"].createNestedObject();
    filter_Children_0_Children_0_Children_0_Children_0["Text"] = true;
    filter_Children_0_Children_0_Children_0_Children_0["Value"] = true;
    filter_Children_0_Children_0_Children_0_Children_0["Children"][0]["Value"] = true;
    // непосредственно парсинг основоного JSON.
    DynamicJsonDocument doc(18000); //выделяем память
    deserializeJson(doc, http.getStream(), DeserializationOption::Filter(filter), DeserializationOption::NestingLimit(12));//читаем файл и применяем фильтр
    doc.shrinkToFit();//освобождаем неиспользованную память
    String cpuName = doc["Children"][0]["Children"][1]["Text"];//название процессора
    String cpuNamePars2 = cpuName.substring(-6, cpuName.length() - 8);
    String cpuTempPackage = doc["Children"][0]["Children"][0]["Children"][0]["Children"][1]["Children"][0]["Value"];//температура процессора
    int cpuTempPackageToInt = cpuTempPackage.toInt();
    cpuTempPackage = cpuTempPackage.substring(0, cpuTempPackage.length() - 3);
    String cpuLoad = doc["Children"][0]["Children"][1]["Children"][4]["Children"][0]["Value"];//загруженность цп %
    String cpuLoad2 = cpuLoad.substring(0, cpuLoad.length() - 1);
    String cpuClock = doc["Children"][0]["Children"][1]["Children"][2]["Children"][1]["Value"]; //частота 1ого ядра цп
    String cpuClockPars = cpuClock.substring(0, cpuClock.length() - 4);

    if (cpuTempPackageToInt >= 20 && cpuTempPackageToInt < 45) {
      tft.drawBitmap(1, 22, cpuBigBMP, 80, 80, ST77XX_GREEN);
    tft.setCursor(13, 56);
    tft.setTextSize(1);
    tft.setTextColor(ST77XX_GREEN);
    tft.println(cpuNamePars2);
    }
    if (cpuTempPackageToInt >= 45 && cpuTempPackageToInt < 65) {
      tft.drawBitmap(1, 22, cpuBigBMP, 80, 80, ST77XX_YELLOW);
    tft.setCursor(13, 56);
    tft.setTextSize(1);
    tft.setTextColor(ST77XX_YELLOW);
    tft.println(cpuNamePars2);
    }
    if (cpuTempPackageToInt >= 65 && cpuTempPackageToInt < 75) {
      tft.drawBitmap(1, 22, cpuBigBMP, 80, 80, ST77XX_ORANGE);
    tft.setCursor(13, 56);
    tft.setTextSize(1);
    tft.setTextColor(ST77XX_ORANGE);
    tft.println(cpuNamePars2);
    }
    if (cpuTempPackageToInt >= 75) {
      tft.drawBitmap(1, 22, cpuBigBMP, 80, 80, ST77XX_RED);
    tft.setCursor(13, 56);
    tft.setTextSize(1);
    tft.setTextColor(ST77XX_RED);
    tft.println(cpuNamePars2);
    }

    //tft.setCursor(13, 56);
    //tft.setTextSize(1);
    //tft.println(cpuNamePars2);
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(80, 5);
    tft.println("CPU");
    tft.setCursor(10, 15);
    tft.println("Intel Core");
    tft.setTextSize(2);
    tft.setCursor(85, 20);
    tft.println(cpuClockPars);
    tft.setCursor(85, 60);
    tft.println(cpuTempPackage); tft.setCursor(135, 60); tft.print("C");
    tft.setCursor(85, 100);
    tft.println(cpuLoad2);  tft.setCursor(145, 100); tft.print("%");
  }
  http.end();//закрываем http соединение
}

void getRamInfo() {
  WiFiClient client; //инициализируем клиента для передачи данных по wifi
  HTTPClient http; //Инициализируем библиотеку httpclient
  http.useHTTP10(true); //устанвливаем версию http протокола 1.1
  http.begin(client, clientIP); //указываем библиотеке через какой интерфейс отправлять данные и на какой адрес
  //Здесь проверяем доступен ли воообще JSON по указанному нами адресу
  int httpCode = http.GET(); //отправляем запрос и получаем код ответа

  // httpCode равен -1 при ошибке
  if (httpCode > 0) {
    // HTTP запрос отправлен
    // файл найден на сервере
    if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
    }
  } else {
    Serial.printf("sensors http sync failed, error: %s\n", http.errorToString(httpCode).c_str()); //содержимое файла не может быть прочитано
    return;
  }

  if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) { //если файл получен начинаем обработку
    //фильтруем содержимое для экономии памяти микроконтроллера
    StaticJsonDocument<224> filter;//создаем вспомогательный Json документ для фильтрации получаемых значений

    JsonObject filter_Children_0_Children_0 = filter["Children"][0]["Children"].createNestedObject();
    filter_Children_0_Children_0["Text"] = true;

    JsonObject filter_Children_0_Children_0_Children_0_Children_0 = filter_Children_0_Children_0["Children"][0]["Children"].createNestedObject();
    filter_Children_0_Children_0_Children_0_Children_0["Text"] = true;
    filter_Children_0_Children_0_Children_0_Children_0["Value"] = true;
    filter_Children_0_Children_0_Children_0_Children_0["Children"][0]["Value"] = true;
    // непосредственно парсинг основоного JSON.
    DynamicJsonDocument doc(18000); //выделяем память
    deserializeJson(doc, http.getStream(), DeserializationOption::Filter(filter), DeserializationOption::NestingLimit(12));//читаем файл и применяем фильтр
    doc.shrinkToFit();//освобождаем неиспользованную память
    String usedRAM = doc["Children"][0]["Children"][2]["Children"][1]["Children"][0]["Value"];//использованная память ОЗУ
    String usedRAM2 = usedRAM.substring(0, usedRAM.length() - 2);
    int usedRAMint = usedRAM2.toInt();
    String freeRAM = doc["Children"][0]["Children"][2]["Children"][1]["Children"][1]["Value"];//свободная память ОЗУ
    String freeRAM2 = freeRAM.substring(0, freeRAM.length() - 2);
    int freeRAMint = freeRAM2.toInt();
    String loadRAM = doc["Children"][0]["Children"][2]["Children"][0]["Children"][0]["Value"]; //загрузка озу в %
    String loadRAM2 = loadRAM.substring(0, loadRAM.length() - 1);
    int loadRAMint = loadRAM.toInt();
    int RAMvolume = freeRAMint + usedRAMint + 1;
    //tft.drawBitmap(3, 50, RamBigligo, 78, 36, ST77XX_WHITE);
    Serial.println(loadRAMint);
    
    if (loadRAMint >= 10 && loadRAMint < 50) {
    tft.drawBitmap(3, 50, RamBigligo, 78, 36, ST77XX_GREEN);
    }
    if (loadRAMint >= 50 && loadRAMint < 70) {
    tft.drawBitmap(3, 50, RamBigligo, 78, 36, ST77XX_YELLOW);
    }
    if (loadRAMint >= 70&& loadRAMint < 85) {
    tft.drawBitmap(3, 50, RamBigligo, 78, 36, ST77XX_ORANGE);
    }
    if (loadRAMint >= 85) {
    tft.drawBitmap(3, 50, RamBigligo, 78, 36, ST77XX_RED);
    }
    
    
    tft.setCursor(80, 5);
    tft.setTextSize(1);
    tft.println("RAM");
    tft.setCursor(10, 30);
    tft.println("no name");
    tft.setTextSize(2);
    tft.setCursor(85, 20);
    tft.println(RAMvolume); tft.setCursor(120, 20); tft.println("GB");
    tft.setCursor(85, 60);
    tft.println(usedRAM2); tft.setCursor(135, 60); tft.println("GB");
    tft.setCursor(85, 100);
    tft.println(loadRAM2); tft.setCursor(145, 100); tft.println("%");
  }
  http.end();//закрываем http соединение
}

void displayChange() {
  //tft.drawRect(1, 1, 159, 127, ST77XX_WHITE);
  getCpuInfo();
  delay(4000);
  tft.fillRect(85, 18, 74, 109, ST77XX_BLACK); 
  getCpuInfo();
  delay(4000);
  tft.fillRect(85, 18, 74, 109, ST77XX_BLACK);
  getCpuInfo();
  delay(4000);
  tft.fillScreen(ST77XX_BLACK);

  //tft.drawRect(1, 1, 159, 127, ST77XX_WHITE);
  getGpuInfo();
  delay(4000);
  tft.fillRect(85, 18, 74, 109, ST77XX_BLACK);
  getGpuInfo();
  delay(4000);
  tft.fillRect(85, 18, 74, 109, ST77XX_BLACK);
  getGpuInfo();
  delay(4000);
  tft.fillScreen(ST77XX_BLACK);

  //tft.drawRect(1, 1, 159, 127, ST77XX_WHITE);
  getRamInfo();
  delay(4000);
  tft.fillRect(85, 18, 74, 109, ST77XX_BLACK);
  getRamInfo();
  delay(4000);
  tft.fillRect(85, 18, 74, 109, ST77XX_BLACK);
  getRamInfo();
  delay(4000);
  tft.fillScreen(ST77XX_BLACK);
}

void splashScreenHorizontal() {

  //загрузочный экран
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(3);
  tft.setCursor(65, 30);
  tft.println("PC");
  tft.setCursor(40, 60);
  tft.println("STATS");
  tft.setTextSize(1);
  tft.setCursor(25, 90);
  tft.setTextColor(ST77XX_BLUE);
  tft.println("code version: ");
  tft.setCursor(105, 90);
  tft.println(CODE_VERS);
  tft.setTextColor(ST77XX_WHITE);
  delay(2500);

  tft.fillScreen(ST77XX_BLACK);
  tft.drawRect(1, 1, 159, 127, ST77XX_GREEN);
}
