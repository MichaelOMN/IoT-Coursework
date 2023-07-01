#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

const char* activity_name = "veloerg#12345679";

// testing functions
#include "tests.h"

//library for managing wifi connections of ESP8266 via http in web-browser interface
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>

char *trimString(char *str, const char* symbols, int len)
{
    char *end;
    for(int i = 0; i < len; i++){
      while((unsigned char)*str == symbols[i]) str++;
      if(*str == 0)
          return str;
      end = str + strlen(str) - 1;
      while(end > str && (unsigned char)*end == symbols[i]) end--;
      end[1] = '\0';
    }
    return str;
}

const char* reg_host = "http://[your_host]:80/auth/activity/sign-up";
const char* get_token_host = "http://[your_host]:80/auth/activity/sign-in";
const char* activity_state_host = "http://[your_host]:80/api/activity_state/";
const char* ping_host = "http://[your_host]:80/ping";

char jwt_token[300];
time_t last_got_time = 0;


WiFiClient wifiClient;
Adafruit_MPU6050 mpu;

void setup_mpu6050(){
  while (!Serial)
    delay(10);

  Serial.println("Adafruit MPU6050 test!");

  // Try to initialize!
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }
  Serial.println("MPU6050 Found!");

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  Serial.print("Accelerometer range set to: ");
  switch (mpu.getAccelerometerRange()) {
  case MPU6050_RANGE_2_G:
    Serial.println("+-2G");
    break;
  case MPU6050_RANGE_4_G:
    Serial.println("+-4G");
    break;
  case MPU6050_RANGE_8_G:
    Serial.println("+-8G");
    break;
  case MPU6050_RANGE_16_G:
    Serial.println("+-16G");
    break;
  }
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  Serial.print("Gyro range set to: ");
  switch (mpu.getGyroRange()) {
  case MPU6050_RANGE_250_DEG:
    Serial.println("+- 250 deg/s");
    break;
  case MPU6050_RANGE_500_DEG:
    Serial.println("+- 500 deg/s");
    break;
  case MPU6050_RANGE_1000_DEG:
    Serial.println("+- 1000 deg/s");
    break;
  case MPU6050_RANGE_2000_DEG:
    Serial.println("+- 2000 deg/s");
    break;
  }

  mpu.setFilterBandwidth(MPU6050_BAND_5_HZ);
  Serial.print("Filter bandwidth set to: ");
  switch (mpu.getFilterBandwidth()) {
  case MPU6050_BAND_260_HZ:
    Serial.println("260 Hz");
    break;
  case MPU6050_BAND_184_HZ:
    Serial.println("184 Hz");
    break;
  case MPU6050_BAND_94_HZ:
    Serial.println("94 Hz");
    break;
  case MPU6050_BAND_44_HZ:
    Serial.println("44 Hz");
    break;
  case MPU6050_BAND_21_HZ:
    Serial.println("21 Hz");
    break;
  case MPU6050_BAND_10_HZ:
    Serial.println("10 Hz");
    break;
  case MPU6050_BAND_5_HZ:
    Serial.println("5 Hz");
    break;
  }

  Serial.println("");
  delay(100);
}


boolean pressed_once = false;


void setup () {
  Serial.begin(115200);
  setup_mpu6050();

  pressed_once = false;

  setupWifi();

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".. ");
  }
  Serial.println("Connection ready!");

  pinMode(LED_BUILTIN, OUTPUT);

  pinMode(12, INPUT_PULLUP);

  registerActivity(activity_name);

  //runTests();
}


// each N milliseconds, we will send the total average rotation speed in angles per second
float calculateRotationSpeedAxisZ(unsigned int N){
  sensors_event_t a, g, temp;
  const int millis_delay = 500;

  float avg = 0;

  for(int i = 0; i < N / millis_delay; i++){
    mpu.getEvent(&a, &g, &temp);
    float gyro_radians = g.gyro.z;
    avg += gyro_radians * 57,2958;
    delay(millis_delay);
  }

  avg /= (N / millis_delay);
  return avg;
}


void setAuthorizationHeader(HTTPClient& http, const char* activity_name){
  char bearer_buffer[300];
  char* token;
  strcpy(bearer_buffer, "Bearer ");

  // update activity's token every 30 minutes
  
  if ( (millis() - last_got_time > 30 * 60 * 1000) || last_got_time == 0){
    token = getActivityToken(activity_name);
    if (token){
      strcat(bearer_buffer, token);
      http.addHeader("Authorization", bearer_buffer);
      last_got_time = millis();
      memset(jwt_token, 0, 300);
      strcpy(jwt_token, token);
      free(token);
    }
  }
  else{
    strcat(bearer_buffer, jwt_token);
    http.addHeader("Authorization", bearer_buffer);
  }
}


int sendActivityState(const char* activity_name){
  HTTPClient http;

  http.begin(wifiClient, activity_state_host);
  http.addHeader("Content-Type", "application/json");

  setAuthorizationHeader(http, activity_name);

  digitalWrite(LED_BUILTIN, LOW);
  float value = calculateRotationSpeedAxisZ(5000);
  digitalWrite(LED_BUILTIN, HIGH);

  const char* tmp_sign = (value < 0 ? "-" : "");
  int npart1 = value;
  float fract = value - npart1;
  int npart2 = trunc(fract * 100);

  char request_buffer[100];

  int written = sprintf(request_buffer, "{\"unit_amount\": %s%d.%d, \"state_type_id\": 2, \"secs\": 5}", tmp_sign, npart1, npart2);
  if (written <= 0) {
    return -1;
  }

  int resp_code = http.POST(request_buffer);
  String resp_json = http.getString();
  Serial.println(resp_json);

  // resp_code should be 200
  if ( !(resp_code <= 300 && resp_code >= 200)) {
    return -1;
  }

  http.end();
  return resp_code;
}


// alloc return_buffer

char* prepareToken(char* jwt_json){
  // removing braces of json
  jwt_json = trimString(jwt_json, "{}", 2);
  // calling strtok 2 times to receive quoted token
  char* tok_part = strtok(jwt_json, ":");
  tok_part = strtok(NULL, ":");
  // removing quotes
  tok_part = trimString(tok_part, "\"", 1);

  char* return_buffer = (char*)malloc(300);
  strcpy(return_buffer, tok_part);

  return return_buffer;
}


char* getActivityToken(const char* name) {
  HTTPClient http;

  http.begin(wifiClient, get_token_host);
  http.addHeader("Content-Type", "application/json");

// signing in
  char signin_buffer[100];

  int written = sprintf(signin_buffer, "{\"name\": \"%s\"}", activity_name);
  if (written <= 0) {
    return NULL;
  }

  int resp_code = http.POST(signin_buffer);
  String data_json = http.getString();

  // resp_code should be 200
  if ( !(resp_code <= 300 && resp_code >= 200)) {
    return NULL;
  }

  // String response to char buffer
  char response_buffer[300];
  strcpy(response_buffer, data_json.c_str());

  char* token_ready = prepareToken(response_buffer);

  http.end();
  return token_ready;
}


int registerActivity(const char* name) {
  HTTPClient http;

  http.begin(wifiClient, reg_host);
  
  http.addHeader("Content-Type", "application/json");

  char named_activity_json[100] = "";
  int written = sprintf(named_activity_json, "{\"name\":\"%s\", \"description\":\"A simple Veloergometer\", \"club_id\": 1}", name);
  if (written <= 0){
    return -1;
  }

  int resp_code = http.POST(named_activity_json);

  // resp_code should be 200
  if ( !(resp_code <= 300 && resp_code >= 200)) {
    return -1;
  }

  Serial.println(resp_code);
  String payload = http.getString();
  Serial.println(payload);  
    
  http.end();

  return resp_code;
}


void pingServer(){
  HTTPClient http;

  http.begin(wifiClient, ping_host);
  int resp_code = http.GET();
  Serial.print("Code: ");
  Serial.println(resp_code);
  Serial.print("Message: ");
  Serial.println(http.getString());
}


void resetToFactoryDefaults() {
  Serial.println("Going to deep sleep!");
  WiFi.disconnect();
  delay(3000);
  hardwareReset();
}


void hardwareReset() {
  ESP.deepSleep(3e6); // 3 seconds delay before turning board on
  //ESP.restart();
}


void setupWifi() {
  WiFiManager wifiManager;

  if (!wifiManager.autoConnect("ESP8266Configuration", "d178c67ee7b21e9d14af805467ed3ae10eec634f")) {
    Serial.println("failed to connect, we should reset as see if it connects");
    hardwareReset();
  }
}


unsigned long long last_millis = 0;

void loop(){
  boolean state = !digitalRead(12);
  if (state && !pressed_once){
    pressed_once = true;
    resetToFactoryDefaults();
  }

  if (millis() - last_millis > 5000){
    sendActivityState(activity_name);
    //pingServer();
    last_millis = millis();
  }
}