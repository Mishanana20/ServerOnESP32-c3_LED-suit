#include <WiFiClient.h>
#include <WiFi.h>
#include "esp_now.h"
#include <ESPmDNS.h>
#include <WebServer.h>
#include <SPIFFS.h>   // Include the  library


int latchPin = 1;
int clockPin = 2;
int dataPin = 3;
int en_din = 4;
int numOfRegisters = 4;//поставил 3 для своего. У ТЕБЯ БУДЕТ 4 РЕГИСТРА РАБОТАТЬ
byte* registerState;


// ЗАМЕНИТЕ MAC-АДРЕСАМИ ПЛАТ-ПРИЕМНИКОВ
uint8_t broadcastAddress1[] = {0x7C, 0xDF, 0xA1, 0xB1, 0x06, 0x48};
uint8_t broadcastAddress2[] = {0x7C, 0xDF, 0xA1, 0xB0, 0x4C, 0x8C};

String RedComand = "";
String BlueComand = "";
String YellowComand = "";
int DelayCommand = 0;

int effect = -1;

long effectSpeed = 30;

typedef struct test_struct {
  int x;
  int y;
} test_struct;
 
test_struct test;

// сообщение, если данные отправлены
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  char macStr[18];
  Serial.print("Packet to: ");
  // Записывает МАС-адрес в строку
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.print(macStr);
  Serial.print(" send status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}



bool startEffect = false;

IPAddress local_ip(192,168,0,10);  //статический IP
IPAddress gateway(192,168,0,1);
IPAddress subnet(255,255,255,0);



WebServer server(80);    // Create a webserver object that listens for HTTP request on port 80

File fsUploadFile;              // a File object to temporarily store the received file

String getContentType(String filename); // convert the file extension to the MIME type
bool handleFileRead(String path);       // send the right file to the client (if it exists)
void handleFileUpload();                // upload a new file to the SPIFFS


void setup() {
   delay(1000);
  Serial.begin(9600);         // Start the Serial communication to send messages to the computer
  delay(100);

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
  



 Serial.print("Setting soft-AP configuration ... ");
Serial.println(WiFi.softAPConfig(local_ip, gateway, subnet) ? "Ready" : "Failed!");

Serial.print("Setting soft-AP ... ");
Serial.println(WiFi.softAP("LED") ? "Ready" : "Failed!");

IPAddress IP = WiFi.softAPIP();
Serial.print("AP IP address: ");
Serial.println(IP);

  SPIFFS.begin();                           // Start the SPI Flash Files System



if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  esp_now_register_send_cb(OnDataSent);
   
  // регистрируем платы в сети
  esp_now_peer_info_t peerInfo;
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  // первая плата  
  memcpy(peerInfo.peer_addr, broadcastAddress1, 6);
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
  
//   вторая плата  
  memcpy(peerInfo.peer_addr, broadcastAddress2, 6);
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }







  

  server.on("/upload", HTTP_GET, []() {                 // if the client requests the upload page
    if (!handleFileRead("/upload.html"))                // send it if it exists
      server.send(404, "text/plain", "404: Not Found"); // otherwise, respond with a 404 (Not Found) error
  });

  server.on("/upload", HTTP_POST,                       // if the client posts to the upload page
    [](){},
    handleFileUpload                                    // Receive and save the file
  );

server.on("/index", HTTP_GET, []() {                 // if the client requests the upload page
    if (!handleFileRead("/index.html"))                // send it if it exists
      server.send(404, "text/plain", "404: Not Found"); // otherwise, respond with a 404 (Not Found) error
  });

  server.on("/index", HTTP_POST,                       // if the client posts to the upload page
    [](){},
    handleFileUpload                                    // Receive and save the file
  );




///часть кода обработки других кнопок

  server.on("/StartEffect", HTTP_GET, []() {                 // если клиент зашел на страничку старта эффекта
StopEffect();
if (!handleFileRead("/StartEffect.html")) 
server.send(404, "text/plain", "404: Not Found");
//StartEffect();
});
  server.on("/StopEffect", HTTP_GET, []() {                 // если клиент зашел на страничку старта эффекта
    StartEffect();
if (!handleFileRead("/StopEffect.html")) 
server.send(404, "text/plain", "404: Not Found");
//StartEffect();
});

 server.on("/StartEffect", HTTP_POST, []() {},                 // если клиент зашел на страничку старта эффекта
StartEffect
);
 server.on("/StopEffect", HTTP_POST,                       // if the client posts to the upload page
    [](){},
    StartEffect                                // Receive and save the file
  );

  server.onNotFound([]() {                              // If the client requests any URI
    if (!handleFileRead(server.uri()))                  // send it if it exists
      server.send(404, "text/plain", "404: Not Found"); // otherwise, respond with a 404 (Not Found) error
  });

  server.begin();                           // Actually start the server
  Serial.println("HTTP server started");
}

void loop() {
  if(!startEffect){
  server.handleClient();
  }
  if(startEffect)
  {
    String line = "";
    // Создаем объект класса «File» для манипуляции с файлом:
    File myFile;
    const char* myFilePath = "/LED.txt";
    if(SPIFFS.exists("/LED.txt"))
{
  Serial.println("Файл существует");
}
  File file = SPIFFS.open("/LED.txt", "r+");
  
  if(file){ // иначе 
    WiFi.mode(WIFI_STA);
    // мы можем открыть файл
    while(file.available()) {
      Serial.println("Читаю строку");
      //читаем строку за строкой в файле
      line = file.readStringUntil('\n');
      Serial.println(line);
    
      //file.close();
      //SPIFFS.remove("/RTC.txt"); //удаляем файл
      if(line.length() != 0) //если строка не пустая
     {
        String part01 = getValue(line,' ',0); 
        String part02 = getValue(line,' ',1);   
        String part03 = getValue(line,' ',2); 

        String part04 = getValue(line,' ',3); 
        String part05 = getValue(line,' ',4); 
        String part06 = getValue(line,' ',5); 
        String part07 = getValue(line,' ',6); 

        DelayCommand = part07.toInt();
        RedComand ="";
        BlueComand ="";
        YellowComand ="";
        if(part01 == "Красный")
        {
          RedComand = part02;
          DelayCommand = part03.toInt();
          }
        else if(part01 == "Синий")
        {
          BlueComand = part02;
          DelayCommand = part03.toInt();
          }
        else if(part01 == "Желтый")
        {
          YellowComand = part02;
          DelayCommand = part03.toInt();
          }
          else {Serial.println("Неверный формат команды");}
        if(part03 == "Красный")
        {
          RedComand = part04;
          DelayCommand = part05.toInt();
          }
        else if(part03 == "Синий")
        {
          BlueComand = part04;
          DelayCommand = part05.toInt();
          }
        else if(part03 == "Желтый")
        {
          YellowComand = part04;
          DelayCommand = part05.toInt();
          }
          else { }//DelayCommand = part03.toInt();} //иначе это задержка
        if(part05 == "Красный")
        {
           RedComand = part06;
           DelayCommand = part07.toInt();
          }
        else if(part05 == "Синий")
        {
          BlueComand = part06;
          DelayCommand = part07.toInt();
          }
        else if(part05 == "Желтый")
        {
          YellowComand = part06;
          DelayCommand = part07.toInt();
          }
          else{}//DelayCommand = part05.toInt();}
        Serial.println("RedComand:" );
       Serial.println(RedComand);
       Serial.println("BlueComand:" );
       Serial.println( BlueComand);
       Serial.println("YellowComand:" );
       Serial.println( YellowComand);
       Serial.println("DelayCommand:" );
       Serial.println(DelayCommand);
       test.y = DelayCommand;

       if(RedComand == "Половинки")
       {test.x = 1;}
       if(RedComand == "Заполнение")
       {test.x = 2;}
       if(RedComand == "Мигание")
       {test.x = 3;}
       if(RedComand == "ОдинБегущийОгонь")
       {test.x = 4;}
       if(RedComand == "ТриБегущихОгня")
       {test.x = 5;}
       if(RedComand == "ОбратноеЗаполнение")
       {test.x = 6;}
       if(RedComand == "ВсеГорят")
       {test.x = 7;}
       esp_err_t result1 = esp_now_send(broadcastAddress1, (uint8_t *) &test, sizeof(test_struct));
   
  if (result1 == ESP_OK) {
    Serial.println("Sent with success");
  }
  else {
    Serial.println("Error sending the data");
  }
if(BlueComand == "Половинки")
       {test.x = 1;}
       if(BlueComand == "Заполнение")
       {test.x = 2;}
       if(BlueComand == "Мигание")
       {test.x = 3;}
       if(BlueComand == "ОдинБегущийОгонь")
       {test.x = 4;}
       if(BlueComand== "ТриБегущихОгня")
       {test.x = 5;}
       if(BlueComand == "ОбратноеЗаполнение")
       {test.x = 6;}
       if(BlueComand == "ВсеГорят")
       {test.x = 7;}
esp_err_t result2 = esp_now_send(broadcastAddress2, (uint8_t *) &test, sizeof(test_struct));
   
  if (result2 == ESP_OK) {
    Serial.println("Sent with success");
  }
  else {
    Serial.println("Error sending the data");
  }

if(YellowComand == "Половинки")
       {test.x = 1;}
       if(YellowComand == "Заполнение")
       {test.x = 2;}
       if(YellowComand == "Мигание")
       {test.x = 3;}
       if(YellowComand == "ОдинБегущийОгонь")
       {test.x = 4;}
       if(YellowComand== "ТриБегущихОгня")
       {test.x = 5;}
       if(YellowComand == "ОбратноеЗаполнение")
       {test.x = 6;}
       if(YellowComand == "ВсеГорят")
       {test.x = 7;}
effect = test.x;

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

       test.x = 0;
       test.y = 0;
       delay(DelayCommand);


      }
     }
    }
    ESP.restart();
  }   
}

String getContentType(String filename) { // convert the file extension to the MIME type
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}

bool handleFileRead(String path) { // send the right file to the client (if it exists)
  Serial.println("handleFileRead: " + path);
  if (path.endsWith("/")) path += "start.html";          // If a folder is requested, send the index file
  String contentType = getContentType(path);             // Get the MIME type
  String pathWithGz = path + ".gz";
  if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) { // If the file exists, either as a compressed archive, or normal
    if (SPIFFS.exists(pathWithGz))                          // If there's a compressed version available
      path = pathWithGz;                                      // Use the compressed verion
    File file = SPIFFS.open(path, "r");                    // Open the file
    size_t sent = server.streamFile(file, contentType);    // Send it to the client
    file.close();                                          // Close the file again
    Serial.println(String("\tSent file: ") + path);
    return true;
  }
  Serial.println(String("\tFile Not Found: ") + path);   // If the file doesn't exist, return false
  return false;
}

void handleFileUpload(){ // upload a new file to the SPIFFS
  HTTPUpload& upload = server.upload();
  if(upload.status == UPLOAD_FILE_START){
    String filename = upload.filename;
    if(!filename.startsWith("/")) filename = "/"+filename;
    Serial.print("handleFileUpload Name: "); Serial.println(filename);
    fsUploadFile = SPIFFS.open(filename, "w");            // Open the file for writing in SPIFFS (create if it doesn't exist)
  } else if(upload.status == UPLOAD_FILE_WRITE){
    if(fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize); // Write the received bytes to the file
  } else if(upload.status == UPLOAD_FILE_END){
    if(fsUploadFile) {                                    // If the file was successfully created
      fsUploadFile.close();                               // Close the file again
      Serial.print("handleFileUpload Size: "); Serial.println(upload.totalSize);
      server.sendHeader("Location","/success.html");      // Redirect the client to the success page
      server.send(303);
    } else {
      Serial.println("File upload failed");
      server.send(500, "text/plain", "500: couldn't create file");
    }
  }
}


void startWeb()
{
  SPIFFS.begin();
  File f = SPIFFS.open("/myTXT.txt", "r");
  server.sendHeader("Location","/download.html"); 
  SPIFFS.end();
  }

void handleRoot() {
  SPIFFS.begin();  
  File f = SPIFFS.open("/myTXT.txt", "a+");
  server.sendHeader("Location","/success.html");
   server.send(303);
  SPIFFS.end();
}


int count = 0; //счетчик измерений


void StartEffect() //функция при кнопке старт меняет переменные и запускает эффекты
{
  startEffect = true; //инвертирует значение булева, которая при запуске равна false
  Serial.println(startEffect);
   Serial.println("я же меняюсь, да?...кажись да");
  }
void StopEffect() //функция при кнопке старт меняет переменные и запускает эффекты
{
  startEffect = false; //инвертирует значение булева, которая при запуске равна false
  Serial.println(startEffect);
   Serial.println("я же меняюсь, да?...кажись да");
  }
  
void ReadFile()
{
  File f = SPIFFS.open("/start.html", "r");
  
  if (!f) {
    Serial.println("file open failed");
  }
  else
  {
      Serial.println("Reading Data from File:");
      //Data from file
      for(int i=0;i<f.size();i++) //Read upto complete file size
      {
        Serial.print((char)f.read());
      }
      f.close();  //Close file
      Serial.println("File Closed");
  }
  delay(5000);
  }


  String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;

  for(int i=0; i<=maxIndex && found<=index; i++){
    if(data.charAt(i)==separator || i==maxIndex){
        found++;
        strIndex[0] = strIndex[1]+1;
        strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }

  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
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
