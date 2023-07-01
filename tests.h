int strcmp(const char* str1, const char* str2);

char *trimString(char *str, const char* symbols, int len);
int sendActivityState(const char* activity_name);
char* prepareToken(char* jwt_json);
char* getActivityToken(const char* name);
int registerActivity(const char* name);


void registerActivity_test(){
  int resp = registerActivity(activity_name);
  if (resp == -1){
    Serial.println("registerActivity_test FAILED");
  }
  else{
    Serial.printf("registerActivity_test PASSED; Status: %d\n", resp);
  }
}


void getActivityToken_test(){
  char* json_token_resp = getActivityToken(activity_name);
  if (json_token_resp == NULL){
    Serial.println("getActivityToken_test FAILED");
  }
  else{
    Serial.println("getActivityToken_test PASSED");
  }
}


void trimString_test(){
  char line1[20] = "{\"json_field\"}";
  char* res1 = trimString(line1, "{}", 2);
  res1 = trimString(res1, "\"", 1);
  char expected1[20] = "json_field";
  if (strcmp(res1, expected1) == 0){
    Serial.println("trimString_test PASSED");
  }
  else {
    Serial.println("trimString_test FAILED");
  }
}


void sendActivityState_test(){
  int resp_code = sendActivityState(activity_name);
  if (resp_code == -1){
    Serial.println("sendActivityState_test FAILED");
  }
  else{
    Serial.printf("sendActivityState_test PASSED; Status: %d\n", resp_code);
  }
}


void prepareToken_test(){
  char token_json[300] = "{\"token\":\"eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9." \
  "eyJFbnRpdHlOYW1lIjoidmVsb2VyZyMxMjM0NSIsImV4cCI6IjIwMjMtMDctMjlUMjI6NTg6Mzku" \
  "ODYyNzk5OCswMzowMCIsImlhdCI6IjIwMjMtMDYtMjlUMjI6NTg6MzkuODYyNzk5OCswMzowMCJ9.0JNcc4ewuP434_tsprxw6-Z35zdul9ecUYN7tnqLlsU\"}";

  char wanted_token[300] = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9." \
  "eyJFbnRpdHlOYW1lIjoidmVsb2VyZyMxMjM0NSIsImV4cCI6IjIwMjMtMDctMjlUMjI" \
  "6NTg6MzkuODYyNzk5OCswMzowMCIsImlhdCI6IjIwMjMtMDYtMjlUMjI6NTg6MzkuODYyNzk5OCswMzowMCJ9.0JNcc4ewuP434_tsprxw6-Z35zdul9ecUYN7tnqLlsU";

  char* raw_token = prepareToken(token_json);
  
  if (strcmp(raw_token, wanted_token) == 0){
    Serial.println("prepareToken_test PASSED");
  }
  else {
    Serial.println("prepareToken_test FAILED");
  }

  free(raw_token);
}


void runTests(){
  registerActivity_test();
  trimString_test();
  prepareToken_test();
  getActivityToken_test();
  sendActivityState_test();
}