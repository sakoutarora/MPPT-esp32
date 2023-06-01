#include "freertos/FreeRTOS.h"
#include <HTTPClient.h>
#include <WiFi.h>        // Include the Wi-Fi library
#include <ArduinoJson.h>
#include <HardwareSerial.h>
#define configCHECK_FOR_STACK_OVERFLOW 2


const char* ssid     = "Saksham";         // The SSID (name) of the Wi-Fi network you want to connect to
const char* password = "8800919033";     // The password of the Wi-Fi network
const char* serverUrl = "https://pv-cell-monitoring-api.onrender.com/api/readings/"; // HTTPS endpoint 
const char* drendpoint = "https://pv-cell-monitoring-api.onrender.com/dutyratio";    // HTTP end-point 

int completeflag = 0;
const int len = 256;
int counter = 0;
float v_oc = 24.56;                    // V_oc variable 
float i_sc = 1.23;                     // i_sc variable 
char buffer[len]; 
HardwareSerial Serialport(2);
SemaphoreHandle_t mutex; 
DynamicJsonDocument doc(200);           // Creating Dynamic json object  
HTTPClient http;                        // Creating HTTPS client
HTTPClient httpget;
int httpCode;
String jsonString; 
int duty_cycle = 45; 
int s_duty_cycle = 45;
String getresponseString;
int updated = 0;
int sendcomplete = 0;
int temp = 25;
int validTemp(float number){
  return ((number >= 25) && (number <= 45));
}

int validvoc(float number)
{
  return ((number > 0) && (number < 23));
}

int validisc(float number)
{
  return ((number >= 0) && (number < 2.4));
}


void UARTRx(void *tparam){
  Serial.println("Inside UARTx");
  while (1)
  {
    if (Serialport.available())
    {
      char ch = Serialport.read();   
      buffer[counter] = ch; 
      counter++;
      if (ch == '\n'){
        buffer[counter-1] = '\0';
        Serial.print("Buffer is:");
        Serial.println(buffer);
        char* ref = buffer;
        char *voc = strtok(ref, ",");
        char *isc = strtok(NULL, ",");
        char *t = strtok(NULL, ",");
        if ((t != NULL) && (voc != NULL ) && (isc != NULL))
        {
          if (validTemp(std::atof(t)) && validvoc(std::atof(voc)) && validisc(std::atof(isc)))
          {
            temp = std::atof(t);
            v_oc = std::atof(voc);
            i_sc = std::atof(isc);
            updated = 1;
            Serial.print("VOC UPDATED:");
            Serial.println(v_oc);
            Serial.print("ISC UPDATED:");
            Serial.println(i_sc);
            Serial.print("TEMP UPDATED:");
            Serial.println(temp);
          }
        }
        counter = 0;
        vTaskDelay(50 / portTICK_RATE_MS);
      }
      if (counter >= len){
        buffer[len-1] = '\0';        
        counter = 0;
        vTaskDelay(50 / portTICK_RATE_MS);
      }
    }
    else{
      vTaskDelay(50 / portTICK_RATE_MS);
    }
    
  }
  vTaskDelete(NULL);
}

void post(void *tparam){
  Serial.println("Inside POST TASK ");
  http.addHeader("Content-Type", "auto");
  vTaskDelay(100/portTICK_RATE_MS);
  while (1){
    // xSemaphoreTake(mutex, portMAX_DELAY);
    if (updated)
    {
      http.begin(serverUrl);                  // Connecting to https endpoint 
      http.addHeader("Content-Type", "application/json");
      Serial.println("HTPP object active ");
      doc["v_oc"] = int(v_oc*100)/100.0;
      doc["i_sc"] = int(i_sc*100)/100.0;
      jsonString = "";
      serializeJson(doc, jsonString);
      Serial.print("Json Strin generated : "); 
      Serial.println(jsonString);
      Serial.println("POSTING DATA TO END POINT");
      int response = http.POST(jsonString);
      

      Serial.print("RESPONSE FROM SERVER: "); 
      Serial.println(response);

      String responseString = http.getString();
      Serial.print("Response String ");
      Serial.println(responseString);
      http.end();
      completeflag = 0;
      // counter = 0;
      updated = 0;   
      sendcomplete = 1;   
    }

    vTaskDelay(50/portTICK_RATE_MS);
  }
  vTaskDelete(NULL);
}

int isValidDuty(){
  return ((s_duty_cycle >= 0) && (s_duty_cycle <= 100));
}


void get(void *tparam){
  Serial.println("Inside GET TASK ");
  vTaskDelay(100/portTICK_RATE_MS);
  // httpget.addHeader("Content-Type", "auto");
  // Serial.println("After Delay");
  while (1){
    if (sendcomplete)
    {
      httpget.begin(drendpoint);                  // Connecting to https endpoint 
      httpCode = httpget.GET();
      Serial.print("RESPONSE FROM SERVER-GET: "); 
      Serial.println(httpCode);

      getresponseString = httpget.getString();
      Serial.print("Response String-GET : ");
      Serial.println(getresponseString);
      httpget.end();
      s_duty_cycle = int((getresponseString.toFloat())*100);
      sendcomplete = 0;
    }
    

    vTaskDelay(50/portTICK_RATE_MS);
  }
  vTaskDelete(NULL);

}

void setup() {
  // put your setup code here, to run once:

  Serialport.begin(115200, SERIAL_8N1, 16, 17);
  Serial.begin(115200);         // Start the Serial communication to send messages to the computer
  delay(10);
  Serial.println("Serail read ....");
  
  WiFi.begin(ssid, password);             // Connect to the network
  Serial.print("Connecting to ");
  Serial.print(ssid);

  while (WiFi.status() != WL_CONNECTED) { // Wait for the Wi-Fi to connect
    delay(500);
    Serial.print('.');
  }

  // Serial.println("\n");
  Serial.println("Connection established!");  
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());         // Send the IP address of the ESP8266 to the computer
  
   
  

  delay(100);
  xTaskCreatePinnedToCore(UARTRx, "UARTRx", 1024, NULL, 2, NULL, 0);
  xTaskCreatePinnedToCore(post, "httpreq", 10000, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(get, "GET", 10000, NULL, 1, NULL, 0);
  // mutex = xSemaphoreCreateMutex();
  vTaskStartScheduler();

}

void loop() {
  // put your main code here, to run repeatedly:
  // Serial.println("Inside loop");
  if (isValidDuty()){
    duty_cycle = s_duty_cycle;
    int offset = 100;
    Serialport.println(int(50+offset));
  }
  else{
    Serial.print("Invalid duty cycle :");
    Serial.println(duty_cycle);
  }
  vTaskDelay(50/ portTICK_RATE_MS);

}
