#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>

#define D2 4
#define D3 0
#define D5 14
#define D7 13
#define D8 15

const int ledRedPin = D7;
const int ledGreenPin = D8;
const int ledControl = D5;
const int trigPin = D3;
const int echoPin = D2;

String slotSensor1;
String checkCar;

const char* ssid     = "DiamondNote10+";
const char* password = "diamond1280";
const char* server = "34.87.27.141";

long duration;
float distance;
int safetyDistance;
bool greenLed,redLed,blueLed,slotSensorValue;
char flo[] = "1";
HTTPClient http;


void setup() {
  pinMode(ledRedPin, OUTPUT);
  pinMode(ledGreenPin, OUTPUT);
  pinMode(ledControl, OUTPUT);
  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input
  Serial.begin(115200); // Starts the serial communication

  Serial.println();
  Serial.println();
  Serial.print("Connection to ");
  Serial.println(ssid);

  //connect Wifi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

}

void loop() {
  static float oldDist=0;
  // Clears the trigPin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echoPin, HIGH);
  
  distance= duration * 0.034 / 2;

//5d59955e00683931f0139837  //slot1
//5d59959700683931f0139838  //slot2
//5d5995bd00683931f0139839  //slot3

  if ((WiFi.status() == WL_CONNECTED)) {
    //get Data from Database
    HTTPClient httpGet;
    httpGet.begin("http://34.87.27.141:3030/parking/5d59917c00683931f0139832/5d5995bd00683931f0139839/getSlot");
    int httpCodeGet = httpGet.GET();
    String payloadGet = httpGet.getString();
    if (httpCodeGet > 0) {
        
      Serial.println(httpCodeGet);
      Serial.println(payloadGet);
      const size_t capacity = JSON_ARRAY_SIZE(1) + JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(8)+182;
      
      DynamicJsonDocument docGet(capacity);
      DeserializationError error = deserializeJson(docGet, payloadGet);
      if (error) {
        Serial.println("Error");
        Serial.println(capacity);
        return;
        }
      
       else{
          
          JsonObject root = docGet.as<JsonObject>();
          const char* id1 = root["_id"];
          String slotSenSorRo = root["slotSensor"];
          String ledGreen = root["slotBarrier"]["green"];
          String ledRed = root["slotBarrier"]["red"];
          String ledBlue = root["slotBarrier"]["blue"];
          
          Serial.println(F("Response:"));
          Serial.println(id1);
          Serial.println(slotSenSorRo);
          Serial.println("Green = "+ledGreen);
          Serial.println("Red = "+ledRed);
          Serial.println("Blue = "+ledBlue);
          Serial.println(checkCar);

          slotSensor1 = slotSenSorRo;
          checkCar = ledBlue;
          
          if(ledGreen.equals("true")) digitalWrite(ledGreenPin, HIGH);
          if(ledRed.equals("true"))  digitalWrite(ledRedPin, HIGH);
          if(ledBlue.equals("true")) digitalWrite(ledControl, HIGH);
          if(ledGreen.equals("false"))  digitalWrite(ledGreenPin, LOW);
          if(ledRed.equals("false"))  digitalWrite(ledRedPin, LOW);
          if(ledBlue.equals("false"))  digitalWrite(ledControl, LOW);
       }
    }
    else Serial.println("Error on HTTP request");
    
  }
  // Calculating the distance
  //distance= duration * 0.034 / 2;
  //safetyDistance = distance;
  Serial.print("test");
  Serial.print(fabs(distance-oldDist));
    safetyDistance = distance;
  if (slotSensor1.equals("true")) { // available
    delay(5000);
    if(fabs(distance-oldDist) > 2 && safetyDistance >= 5){
        greenLed = true;
        redLed = false;
        blueLed = false;
        slotSensorValue= false;
        pushToAPIEndPoint();
    }
  } 
  Serial.print("Distance: ");
  Serial.println(distance);
  oldDist = distance;

}

void pushToAPIEndPoint() {
  //update Json to Database
  DynamicJsonDocument docPut(1024);
  JsonObject slot= docPut.to<JsonObject>();
  slot["floor"] = flo;
  slot["slotSensor"] = slotSensorValue;
  JsonObject slotBarrier = slot.createNestedObject("slotBarrier");
  slotBarrier["green"] = greenLed;
  slotBarrier["red"] = redLed;
  slotBarrier["blue"] = blueLed;
  Serial.println("HI");
  char JSONMesBuf[300];
  http.begin("http://34.87.27.141:3030/parking/5d59917c00683931f0139832/5d5995bd00683931f0139839/updateSlotAvailable");
  http.addHeader("Content-Type", "application/json"); //Specify content-type header
  Serial.println("Connected to server");
  serializeJsonPretty(slot,Serial);
  serializeJsonPretty(slot,JSONMesBuf);
  int httpCodePut = http.sendRequest("PUT",JSONMesBuf);
  String payloadPut = http.getString();
  
  Serial.println("httpcode = ");
  Serial.println(httpCodePut);
  Serial.println(payloadPut);
  http.writeToStream(&Serial);
  http.end();
  //delay(1000);
  
}
