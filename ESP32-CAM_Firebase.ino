#include <FirebaseESP32.h>
#include <FirebaseESP32HTTPClient.h>
#include <FirebaseJson.h>

#include <Wire.h>
#include <ACROBOTIC_SSD1306.h>
#define I2C_SDA 14
#define I2C_SCL 15

const char* ssid = "Jesus";
const char* password = "Password";
String FIREBASE_HOST = "https://spyesp8266.firebaseio.com/";
String FIREBASE_AUTH = "GGN6PSry1QHQBw3JqyD5y1KHd8ZoC5lwW4sVxYl1";

FirebaseData firebaseData;
FirebaseJson json;

#include <WiFi.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "Base64.h"

#include "esp_camera.h"

// WARNING!!! Make sure that you have either selected ESP32 Wrover Module,
//            or another board which has PSRAM enabled

//CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22
#define Flash 16

void setup() { 
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  pinMode(Flash, OUTPUT);
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  //Serial.println();
  //Serial.println("ssid: " + (String)ssid);
  //Serial.println("password: " + (String)password);
  
  WiFi.begin(ssid, password);

  long int StartTime=millis();
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      if ((StartTime+10000) < millis()) break;
  } 

  if (WiFi.status() == WL_CONNECTED) {
    char* apssid = "Jesus";
    char* appassword = "Password";         //AP password require at least 8 characters.
    //Serial.println(""); 
    //Serial.print("Camera Ready! Use 'http://");
    //Serial.print(WiFi.localIP());
    //Serial.println("' to connect");
    WiFi.softAP((WiFi.localIP().toString()+"_"+(String)apssid).c_str(), appassword);            
  }
  else {
    //Serial.println("Connection failed");
    return;
  } 

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  //init with high specs to pre-allocate larger buffers
  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;  //0-63 lower number means higher quality
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;  //0-63 lower number means higher quality
    config.fb_count = 1;
  }
  
  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    //Serial.printf("Camera init failed with error 0x%x", err);
    delay(1000);
    ESP.restart();
  }

  //drop down frame size for higher initial frame rate
  sensor_t * s = esp_camera_sensor_get();
  s->set_framesize(s, FRAMESIZE_QQVGA);  // VGA|CIF|QVGA|HQVGA|QQVGA   ( UXGA? SXGA? XGA? SVGA? )
 
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);
  Firebase.setMaxRetry(firebaseData, 3);
  Firebase.setMaxErrorQueue(firebaseData, 30); 
  Firebase.enableClassicRequest(firebaseData, true);

  //----
  Wire.begin(I2C_SDA, I2C_SCL);  
  oled.init();                      // Initialze SSD1306 OLED display
  oled.clearDisplay();              // Clear screen
  /*
  oled.setTextXY(0,0);              // Set cursor position, start of line 0
  oled.putString("Jesus");
  oled.setTextXY(1,0);              // Set cursor position, start of line 1
  oled.putString(WiFi.localIP().toString());
  oled.setTextXY(2,0);              // Set cursor position, start of line 2
  oled.putString("Mamu");
  oled.setTextXY(2,10);             // Set cursor position, line 2 10th character
  oled.putString("Akku"); */
  oled.setTextXY(2,0);              // Set cursor position, start of line 2
  oled.putString((String)WiFi.status());
  Firebase.setString(firebaseData, "/addFingerPrint", "0");
}

 void sendImage(String photo){
  json.clear();
  json.add(photo, Photo2Base64());
  String photoPath = "/esp32-cam";
  Firebase.setJSON(firebaseData,photoPath,json);
 }
 void captureImage(String photo){
  if(Firebase.getString(firebaseData, "/captureButton")){
     String  captureButton = firebaseData.stringData();
     if(captureButton == "clicked"){
        json.clear();
        digitalWrite(Flash , HIGH);
        json.add(photo, Photo2Base64());
        digitalWrite(Flash , LOW);
        String photoPath = "/esp32-capture";
        Firebase.setJSON(firebaseData,photoPath,json);
        Firebase.setString(firebaseData, "/captureButton", "unclicked");
        oled.setTextXY(1,0);
        oled.putString("Image Captured!!!");
     }
   }
 }
void addFingerPrint(){
  if(Firebase.getString(firebaseData, "/addFingerPrint")){
     String  fireStatus = firebaseData.stringData();                  
      if (fireStatus == "1") {                                      
          Serial.print("CMD2");
          //oled.setTextXY(1,0);
            //oled.putString("CMD2");
          //delay(3000);
          Firebase.setString(firebaseData, "/addFingerPrint", "0");
          if(Serial.read() == 1){
            oled.setTextXY(1,0);
            oled.putString("FingerPrint Added!!!");
            Firebase.setString(firebaseData, "/addFingerPrint", "0");
                   if(Firebase.getInt(firebaseData, "/totalFingerPrint")){
                   int  fingerPrintCount = firebaseData.intData();
                   Firebase.setInt(firebaseData, "/totalFingerPrint", fingerPrintCount++);}
          }
          else if(Serial.read() == 2){
            oled.setTextXY(1,0);
            oled.putString("Failed!!!");
            Firebase.setString(firebaseData, "/addFingerPrint", "0");
          }
          else if(Serial.read() == 3){
            oled.setTextXY(1,0);
            oled.putString("Library Full!!!");
            Firebase.setString(firebaseData, "/addFingerPrint", "0");
          }
          //else{oled.setTextXY(1,0);
          //  oled.putString("Error");}
         }
      
  }
}
void deleteFingerPrint(){
  if(Firebase.getString(firebaseData, "/deleteFingerPrint")){
     String  fireStatus = firebaseData.stringData();                    
      if (fireStatus == "1") { 
          if(Firebase.getString(firebaseData, "/deleteUserNum")){
            String  userNum = firebaseData.stringData();            
          Serial.print("CDMJ"+userNum);                                                       
         }
         if(Serial.read() == 1){
            oled.setTextXY(1,0);
            oled.putString("FingerPrint Deleted!!!");
            Firebase.setString(firebaseData, "/deleteFingerPrint", "0");
            Firebase.setString(firebaseData, "/deleteUserNum", "0");       
          }
         
  }
}}
/*
void matchFingerPrint(){
                    
     // if (PushButton == HIGH) {             
          Serial.print("CMD3");                                                       
         }
         if(Serial.read() == 1){
            oled.setTextXY(1,0);
            oled.putString("Matching successful !");       
          }
         else if(Serial.read() == 2){
            oled.setTextXY(1,0);
            oled.putString("Not found !");       
          }
         else if(Serial.read() == 3){
            oled.setTextXY(1,0);
            oled.putString("Time Out !");       
          }
         else if(Serial.read() == 3){
            oled.setTextXY(1,0);
            oled.putString("Failed !");       
          }
         
     // }
*/
void loop() { 
  
  captureImage("capture");

  if(Firebase.getString(firebaseData, "/led")){
     String  ledFlash = firebaseData.stringData();

     if(ledFlash == "1"){digitalWrite(Flash , HIGH);
     oled.setTextXY(0,0);
     oled.putString("FLASH ON");}
     else{
      digitalWrite(Flash , LOW);
      oled.setTextXY(0,0);
      oled.clearDisplay();}

  }

  if(Firebase.getString(firebaseData, "/liveview")){
     String  liveview = firebaseData.stringData();

     if(liveview == "ON") {sendImage("photo");
     oled.setTextXY(0,0);
     oled.putString("LIVE VIEW");}
     else{oled.setTextXY(0,0);
     oled.clearDisplay();}
  }
  
  //addFingerPrint();
  //oled.setTextXY(0,0);
  //oled.putString((String)Serial.read());


}

String Photo2Base64() {
    camera_fb_t * fb = NULL;
    fb = esp_camera_fb_get();  
    if(!fb) {
      Serial.println("Camera capture failed");
      return "";
    }
  
    String imageFile = "data:image/jpeg;base64,";
    char *input = (char *)fb->buf;
    char output[base64_enc_len(3)];
    for (int i=0;i<fb->len;i++) {
      base64_encode(output, (input++), 3);
      if (i%3==0) imageFile += urlencode(String(output));
    }

    esp_camera_fb_return(fb);
    
    return imageFile;
}

String urlencode(String str)
{
    String encodedString="";
    char c;
    char code0;
    char code1;
    char code2;
    for (int i =0; i < str.length(); i++){
      c=str.charAt(i);
      if (c == ' '){
        encodedString+= '+';
      } else if (isalnum(c)){
        encodedString+=c;
      } else{
        code1=(c & 0xf)+'0';
        if ((c & 0xf) >9){
            code1=(c & 0xf) - 10 + 'A';
        }
        c=(c>>4)&0xf;
        code0=c+'0';
        if (c > 9){
            code0=c - 10 + 'A';
        }
        code2='\0';
        encodedString+='%';
        encodedString+=code0;
        encodedString+=code1;
        //encodedString+=code2;
      }
      yield();
    }
    return encodedString;
}
