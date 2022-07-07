//test_struct test;
/*********
  Rui Santos  
  Complete project details at https://RandomNerdTutorials.com/esp-now-one-to-many-esp32-esp8266/
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*********/
 
#include "esp_now.h"
#include "WiFi.h"

int latchPin = 1;
int clockPin = 2;
int dataPin = 3;
int en_din = 4;
int numOfRegisters = 4;//поставил 3 для своего. У ТЕБЯ БУДЕТ 4 РЕГИСТРА РАБОТАТЬ
byte* registerState;

int effect = -1;

long effectSpeed = 30;
 
//Пример структуры для приема данных
//ДОЛЖЕН СОВПАДАТЬ СО СТРУКТУРОЙ ПЛАТЫ-ОТПРАВИТЕЛЯ
typedef struct test_struct {
  int x;
  int y;
} test_struct;
 
//Создаем элемент struct_message с названием myData
test_struct myData;
 
//обратная функция, которая вызывается, когда сообщение получено
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&myData, incomingData, sizeof(myData));
  Serial.print("Bytes received: ");
  Serial.println(len);
  Serial.print("x: ");
  Serial.println(myData.x);
  Serial.print("y: ");
  Serial.println(myData.y);
  Serial.println();
  effect = myData.x;
  
}
 
void setup() {
  //Запускаем монитор порта
  Serial.begin(115200);
    //Initialize array
  registerState = new byte[numOfRegisters]; //4 регистра
  for (size_t i = 0; i < numOfRegisters; i++) {
    registerState[i] = 0;
  }

  //set pins to output so you can control the shift register
  pinMode(latchPin, OUTPUT); //МОЖНО НЕ ТРОГАТЬ
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  pinMode(en_din, OUTPUT);
  digitalWrite(en_din, LOW);
  
  //Выставляем режим клиента WiFi
  WiFi.mode(WIFI_STA);
 
  //Запускаем протокол ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  // После запуска протокола определяем функцию recv CB
  // чтобы получать информацию о приеме
  esp_now_register_recv_cb(OnDataRecv);
   Serial.println("я запустился");
}
 
void loop() {
 if(effect ==1)
  {
    effectP(effectSpeed);
     effect =-1;
    }
    if(effect ==2)
  {
     effectA(effectSpeed);
     effect =-1;
    }
        if(effect ==3)
  {
     effectD(effectSpeed);
     effect =-1;
    }
        if(effect ==4)
  {
     effectC(effectSpeed);
     effect =-1;
    }
        if(effect ==5)
  {
     effectB(effectSpeed);
     effect =-1;
    }
            if(effect ==6)
  {
     effectP(effectSpeed);
     effect =-1;
    }
}

void effectA(int speed){ //ПЕРВЫЙ ЭФФЕКТ Заполнение
  for (int i = 0; i < 32; i++){ //зависит от кол-ва диодов
    for (int k = i; k < 32; k++){
      regWrite(k, HIGH);
      delay(speed); //хадержка свечения
      regWrite(k, LOW);
    }
    regWrite(i, HIGH);
  }
}

void effectB(int speed){ //ВТОРОЙ ЭФФЕКТ Обратное заполнение
  for (int i = 32; i >= 0; i--){ 
    for (int k = 0; k < i; k++){
      regWrite(k, HIGH);
      delay(speed);
      regWrite(k, LOW);
    }
    regWrite(i, HIGH);
  }
}

void effectC(int speed){ //ТРЕТИЙ ЭФФЕКТ Одие Бегущий огонь
  int prevI = 0;
  for (int i = 0; i < 32; i++){
    regWrite(prevI, LOW);
    regWrite(i, HIGH);
    prevI = i;

    delay(speed);
  }

  for (int i = 32 ; i >= 0; i--){
    regWrite(prevI, LOW);
    regWrite(i, HIGH);
    prevI = i;

    delay(speed);
  }
}


void effectD(int speed){ //МОЙ ЭФФЕКТ Мигание
  for (int i = 0; i < 32; i++){ //зависит от кол-ва диодов
      regWrite(i, LOW); //ЗАЖЕГ все
    }
     delay(speed*3); //Задержка свечения умножил на 10 скорость
    for(int i = 0; i < 32; i++)
    {
      regWrite(i, HIGH); //все потушил
      }
    delay(speed*3); //Задержка свечения

  }

void effectP(int speed) { //Половинки
   for (int i = 0; i < 16; i++){ //зависит от кол-ва диодов
      regWrite(i, HIGH); //ЗАЖЕГ все
    }
      //Задержка свечения умножил на 10 скорость
    for(int i = 16; i < 32 ; i++)
    {
      regWrite(i, LOW); //все потушил
      }
    delay(speed*3); //Задержка свечения
       for (int i = 0; i < 16; i++){ //зависит от кол-ва диодов
      regWrite(i, LOW); //ЗАЖЕГ все
    }
      //Задержка свечения умножил на 10 скорость
    for(int i = 16; i < 32 ; i++)
    {
      regWrite(i, HIGH); //все потушил
      }
    delay(speed*3); //Задержка свечения
   
}

void effectE(int speed){ //ТРЕТИЙ ЭФФЕКТ Три бегущих огня
  int prevI = 0;
  for (int i = 2; i < 32; i++){
    regWrite(prevI-3, LOW);
    regWrite(i-1, HIGH);
    regWrite(i-2, HIGH);
    prevI = i;

    delay(speed);
  }

  for (int i = 32 ; i >= 0; i--){
    regWrite(prevI+3, LOW);
    regWrite(i+1, HIGH);
    regWrite(i+2, HIGH);
    prevI = i;

    delay(speed);
  }
}
void effectI(int speed) {
   for (int i = 0; i < 16; i++){ //зависит от кол-ва диодов
      regWrite(i, HIGH); //ЗАЖЕГ все
    }
      //Задержка свечения умножил на 10 скорость
    for(int i = 16; i < 32 ; i++)
    {
      regWrite(i, LOW); //все потушил
      }
    delay(speed*15); //Задержка свечения
 for (int i = 0; i < 16; i++){ //зависит от кол-ва диодов
      regWrite(i, LOW); //ЗАЖЕГ все
    }
      //Задержка свечения умножил на 10 скорость
    for(int i = 16; i < 32 ; i++)
    {
      regWrite(i, HIGH); //все потушил
      }
    delay(speed*15); //Задержка свечения
   
}


void regWrite(int pin, bool state){ //ЗАПИСЬ В регистры. МОЖНО НЕ ТРОГАТЬ
  //Determines register
  int reg = pin / 8;
  //Determines pin for actual register
  int actualPin = pin - (8 * reg);

  //Begin session
  digitalWrite(latchPin, LOW);

  for (int i = 0; i < numOfRegisters; i++){
    //Get actual states for register
    byte* states = &registerState[i];

    //Update state
    if (i == reg){
      bitWrite(*states, actualPin, state);
    }

    //Write
    shiftOut(dataPin, clockPin, MSBFIRST, *states);
  }

  //End session
  digitalWrite(latchPin, HIGH);
}
