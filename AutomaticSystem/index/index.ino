#include <Arduino.h>
#include <string.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <SocketIOClient.h>
#include <ArduinoJson.h>
#include "SimpleTimer.h"
#include <DHT.h>

//Socket
ESP8266WiFiMulti WiFiMulti;
SocketIOClient webSocket;

extern String RID;
extern String Rname;
extern String Rcontent;

//Wifi and sensor temp & hum
#define HOST "automaticsystem.herokuapp.com"
#define WIFI_SSID "Room HKT"              //Ten wifi
#define WIFI_PASSWORD "hoilamgi6"         //Pass wifi
#define MOTORPIN 16     //MOTOR phun suong
#define MOTORPIN_2 12   //MOTOR nho giot
#define DHTPIN 4             // Pin ket noi voi DHT
#define DHTTYPE DHT11        // Su dung cam bien DHT11
#define t_zero 0             //zero
#define t_short 15           //short
#define t_medium 30          //medium
#define t_long 45            //long
#define t_verylong 60        //verylong
#define t_repeat 60000          // lap moi 60s

DHT dht(DHTPIN, DHTTYPE);    // Cau hinh chan DHT 

//Variable thuat toan mo
float x = 0;    //Nhiet do
float y = 0;    //Do am
float z = 0;    //Thoi gian bom

//Variable
SimpleTimer timer;        // Su dung timer
float hum = 0;            //Nhiet do
float temp = 0;           //Do am
bool flagAuto = false;    //Che do tu dong
bool flagMotor = false;   //Trang thai motor 1
bool flagMotor_2 = false; //Trang thai motor 2
int num = 0;              //So lan lap o che do Auto
unsigned long current = 0;//Thoi gian tinh tu luc chay chuong trinh
unsigned long lastrun = 0;//Thoi gian tinh tu luc lan cuoi cung chay Auto
int n = 0;                //So lan lap doc cam bien nhiet do
int timerId = 0;

void setup() {
  Serial.begin(115200);                 // Mo Serial
  pinMode(MOTORPIN, OUTPUT);
  digitalWrite(MOTORPIN, HIGH);
  pinMode(MOTORPIN_2, OUTPUT);
  digitalWrite(MOTORPIN_2, HIGH);
  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH);
  dht.begin();                          // Khoi tao DHT

  //Connect to wifi.
  WiFiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("connecting");
  while (WiFiMulti.run() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("connected: ");
  Serial.println(WiFi.localIP());

  //Socket
  if (!webSocket.connect(HOST, 80)) {
    Serial.println("connection failed");
    return;
  }
  if (webSocket.connected())  {
    Serial.println("connection success");
  }

  Serial.println(min(-12, 1));

  //Timer
  timer.setTimeout(3000L, login);         // login
  timer.setTimeout(3000L, sendStatus);    //Gui lai trang thai khi khoi dong lai
  timer.setInterval(5000L, printTempHum); //1s doc cam bien 1 lan
  timer.setInterval(5000L, sendTempHum);  //Gui temp, hum den server
  //timer.setInterval(10000L, test);
}

bool flag = false;

void test(){
  flag = !flag;
  if (flag){
    digitalWrite(MOTORPIN, HIGH);
  } else {
    digitalWrite(MOTORPIN, LOW);
  }
}

void loop() {
  
  timer.run();
  
  if (webSocket.monitor()){
    //Serial.println(RID);
    if (RID == "serversendmodel_1"){
      //Bat-tat motor
      if (Rcontent == "on"){
        onMotor();
      } else {
        offMotor();
      }
      Serial.print("\n\nMotor Status: ");
      Serial.println(Rcontent);
    } else if (RID == "serversendmodel_2"){
      if (Rcontent == "on"){
        flagAuto = true;
      } else {
        flagAuto = false;
        timer.disable(timerId);
        offMotor();
      }
      num = 0;
      Serial.print("\n\nAuto Status: ");
      Serial.println(Rcontent);
    } else if (RID == "serversendmodel_3"){
      //Test logic mo
      int index = Rcontent.indexOf('-');
      int len = Rcontent.length();
      int t = Rcontent.substring(0, index).toInt();
      int h = Rcontent.substring(index+1).toInt();
      float result = progressTime(t, h);
      Serial.println("\nTime: " + String(result, 1));
    } else if (RID == "serversendmodel_4"){
      //On/off motor 2
      if (Rcontent == "on"){
        onMotor_2();
      } else {
        offMotor_2();
      }
      Serial.print("\n\nMotor_2 Status: ");
      Serial.println(Rcontent);
    }
  }

  if (flagAuto){
    //Ngay khi nguoi dung nhan nut Auto
    if (num == 0){
      runAuto();
    } else {
      if (millis() - lastrun >= t_repeat){
        runAuto();
      }
    }
  }
}

void runAuto(){
  num++;
  lastrun = millis();
  z = progressTime(temp, hum);
  Serial.println("Run Auto: " + String(z, 0));
  if (z>0){
    timer.setTimeout(1000L, onMotor);
    timerId = timer.setTimeout((z+1)*1000L, offMotor);
    sendMessageAuto();
  }
}

void sendMessageAuto(){
  StaticJsonBuffer<200> jsonBuffer;
  String JSON;
  JsonObject& root = jsonBuffer.createObject();
  root["time"] = String(z, 0);
  root["num"] = String(num, 0);
  root.printTo(JSON);
  webSocket.sendJSON("modelsenddata_4", JSON);
}

void onMotor(){
  flagMotor = true;
  digitalWrite(MOTORPIN, LOW);
  sendStatus();
}

void offMotor(){
    flagMotor = false;
    digitalWrite(MOTORPIN, HIGH);
    sendStatus();
}

void onMotor_2(){
  flagMotor_2 = true;
  digitalWrite(MOTORPIN_2, LOW);
  sendStatus();
}

void offMotor_2(){
    flagMotor_2 = false;
    digitalWrite(MOTORPIN_2, HIGH);
    sendStatus();
}

void login(){
  StaticJsonBuffer<200> jsonBuffer;
  String JSON;
  JsonObject& root = jsonBuffer.createObject();
  root["status"] = "on";
  root.printTo(JSON);
  webSocket.sendJSON("modelsenddata_2", JSON);
  Serial.println("Has Login");
}

void sendTempHum(){
  StaticJsonBuffer<200> jsonBuffer;
  String JSON;
  JsonObject& root = jsonBuffer.createObject();
  root["temp"] = String(temp, 0);
  root["hum"] = String(hum, 0);
  root.printTo(JSON);
  webSocket.sendJSON("modelsenddata_1", JSON);
}

void sendStatus(){
  StaticJsonBuffer<200> jsonBuffer;
  String JSON;
  JsonObject& root = jsonBuffer.createObject();
  root["motor_2"] = flagMotor_2? "on" : "off";
  root["motor"] = flagMotor? "on" : "off";
  root["auto"] = flagAuto? "on" : "off";
  root.printTo(JSON);
  webSocket.sendJSON("modelsenddata_3", JSON);
}

void printTempHum(){
  getTemp();
  getHum();
  if (isnan(hum) || isnan(temp)) {
    temp = 0;
    hum = 0;
    Serial.println("Failed to read from DHT sensor!");
    delay(100);
  } else {
    n++;
//    Serial.print("\nn = ");
//    Serial.println(n);
//    Serial.print("temp: ");
//    Serial.println(temp);
//    Serial.print("hum: ");
//    Serial.println(hum);
    Serial.println();
    delay(2000);
  }
}

float getTemp(){
  float tam = dht.readTemperature();
  if (!isnan(tam)){
    temp = tam;
  } else {
    Serial.println("Failed to read from DHT sensor!");
    delay(100);
  }
  return temp;
}

float getHum(){
  float tam = dht.readHumidity();
  if (!isnan(tam)){
    hum = tam;
  } else {
    Serial.println("Failed to read from DHT sensor!");
    delay(100);
  }
  return hum;
}

float progressTime(float t, float h){
  float result = 0;
  float x = t;
  float y = h;

  Serial.println("\nX: " + String(x, 1));
  Serial.println("Y: " + String(y, 1));
  
  float DCold = (x >= 0) && (x <= 50) ? 1 - x/50 : 0;
  float DMedium = (x >= 0) && (x <= 50) ? x/50 : 2 - x/50;
  float DHot = (x >= 0) && (x <= 50) ? 0 : x/50 -1;

  float KMoist = (y >= 0) && (y <= 50) ? 1 - y/50 : 0;
  float KMedium = (y >= 0) && (y <= 50) ? y/50 : 2 - y/50;
  float KDry = (y >= 0) && (y <= 50) ? 0 : y/50 -1;

//  float W1 = min(DHot, KDry)*t_verylong;
//  float W2 = min(DMedium, KDry)*t_long;
//  float W3 = min(DCold, KDry)*t_medium;
//  float W4 = min(DHot, KMedium)*t_long;
//  float W5 = min(DMedium, KMedium)*t_medium;
//  float W6 = min(DCold, KMedium)*t_short;
//  float W7 = min(DHot, KMoist)*t_medium;
//  float W8 = min(DMedium, KMoist)*t_short;
//  float W9 = min(DCold, KMoist)*t_zero;

  float W1 = min(DHot, KDry);
  float W2 = min(DMedium, KDry);
  float W3 = min(DCold, KDry);
  float W4 = min(DHot, KMedium);
  float W5 = min(DMedium, KMedium);
  float W6 = min(DCold, KMedium);
  float W7 = min(DHot, KMoist);
  float W8 = min(DMedium, KMoist);
  float W9 = min(DCold, KMoist);

  Serial.println("DCold: " + String(DCold, 1));
  Serial.println("DMedium: " + String(DMedium, 1));
  Serial.println("DHot: " + String(DHot, 1));

  Serial.println("KMoist: " + String(KMoist, 1));
  Serial.println("KMedium: " + String(KMedium, 1));
  Serial.println("KDry: " + String(KDry, 1));

  Serial.println("\nW1: " + String(W1, 1));
  Serial.println("W2: " + String(W2, 1));
  Serial.println("W3: " + String(W3, 1));
  Serial.println("W4: " + String(W4, 1));
  Serial.println("W5: " + String(W5, 1));
  Serial.println("W6: " + String(W6, 1));
  Serial.println("W7: " + String(W7, 1));
  Serial.println("W8: " + String(W8, 1));
  Serial.println("W9: " + String(W9, 1));

  float uC = W1*t_verylong 
           + W2*t_long 
           + W3*t_medium 
           + W4*t_long 
           + W5*t_medium
           + W6*t_short 
           + W7*t_medium 
           + W8*t_short 
           + W9*t_zero;

  Serial.println("uC: " + String(uC, 1));
  Serial.println("Sum: " + String(W1 + W2 + W3 + W4 + W5 + W6 + W7 + W8 + W9, 1));
  
  result = uC/(W1 + W2 + W3 + W4 + W5 + W6 + W7 + W8 + W9);
  return result;
}











