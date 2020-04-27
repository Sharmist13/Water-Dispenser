//  Automatic Water Dispenser
//  with MQTT logging
//  https://github.com/StorageB/Water-Dispenser

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>

#define valve_output   D1       // valve output pin
#define ir_input       D5       // ir sensor input piin
#define switch_input   D7       // pushbutton switch input pin
#define led_pin        D2       // NeoPixel strip signal pin
#define led_count      8        // number of LEDs in NeoPixel strip
#define led_brightness 125      // NeoPixel brightness (max = 255)
#define pwm_intervals  100      // number of intervals in the fade in/out for loops    
#define ir_input_delay 100      // how long to wait once an object is detected by IR sensor before opening valve
#define error_time     60000    // amount of time valve can be open before automatically turning off and displaying an error

int ir_state;                   // state of IR sensor - LOW if object detected, HIGH if no object detected
int switch_state;               // state of pushbutton switch - HIGH if pressed, LOW if not pressed
int light_on = 0;               // state of NeoPixel strip: 1 if on, 0 if off
int valve_open = 0;             // state of valve: 1 if open, 0 if closed
int data_published = 0;         // set to 0 if data has not published, 1 if it has
int error_status = 0;           // set to 0 if no errors, 1 if there is an error

unsigned long current_time = 0; // used to get current time
unsigned long timer_start = 0;  // used to start timer when valve is open
unsigned long run_time_lng = 0; // used to calculate how long the valve was open
float gal_per_sec = 0.0072;     // measured flow rate of water in gallons per second

float R = (pwm_intervals * log10(2))/(log10(255)); // used to calculate the 'R' value for fading LEDs

// Declare NeoPixel strip object:
Adafruit_NeoPixel strip(led_count, led_pin, NEO_GRB + NEO_KHZ800);

// Enter network credentials
#ifndef STASSID
#define STASSID "network"
#define STAPSK  "password"
#endif
const char* ssid = STASSID;
const char* password = STAPSK;

// Required for MQTT
const char *ID = "water-dispenser"; // Name of our device, must be unique for each device
const char *TOPIC = "water";        // Topic to subcribe to
IPAddress broker(192,168,1,XXX);    // IP address of your MQTT broker eg. 192.168.1.50
WiFiClient wclient;
PubSubClient client(wclient);       // Setup MQTT client
bool state=0;



void setup() {
  Serial.begin(9600);
  
  // ----- Project setup -----

  pinMode(LED_BUILTIN, OUTPUT);     // Initialize on board LED as output  
  pinMode(valve_output, OUTPUT);    // Initialize pin as digital output   (solenoid valve)
  pinMode(ir_input, INPUT);         // Initialize pin as digital input    (infrared sensor)
  pinMode(switch_input, INPUT);     // Initialize pin as digital input    (pushbutton switch)
  
  digitalWrite(LED_BUILTIN, HIGH);  // LED off
  digitalWrite(valve_output, LOW);  // valve closed

  strip.begin();                        // Initialize NeoPixel strip object (required)
  strip.show();                         // Turn OFF all pixels ASAP
  strip.setBrightness(led_brightness);  // Set BRIGHTNESS (max = 255)

  // ----- Required for OTA programming -----

  Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);
  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");
  // No authentication by default
  // ArduinoOTA.setPassword("admin");
  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  // Reqiured for MQTT publishing
  client.setServer(broker, 1883);

}


// Connect MQTT
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(ID)) {
      Serial.println("connected");
      Serial.print("Publishing to: ");
      Serial.println(TOPIC);
      Serial.println('\n');
    } else {
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

// Fade LEDs on
void fade_in(String fade_color, int wait) {
  int brightness = 0;
  for(int i = 0; i <= pwm_intervals; i++){
    brightness = pow (2, (i / R)) - 1;
    for(int j = 0; j < strip.numPixels(); j++) {
      if(fade_color == "blue") {
        strip.setPixelColor(j,0,0,brightness);
      }
      if (fade_color == "red") {
        strip.setPixelColor(j,brightness,0,0);
      }
    }
    strip.show();
    delay(wait);
  }
}

// Fade LEDs off
void fade_out(String fade_color, int wait) {
  int brightness = 255;
  for(int i = pwm_intervals; i >= 0; i--){
    brightness = pow (2, (i / R)) - 1;
    for(int j = 0; j < strip.numPixels(); j++) {
      if(fade_color == "blue") {
        strip.setPixelColor(j,0,0,brightness);
      }
      if (fade_color == "red") {
        strip.setPixelColor(j,brightness,0,0);
      }
    }
    strip.show();
    delay(wait);
  }
  strip.clear();
  strip.show();
}

// Turn valve off and display error on LEDs if an error is detected
void error() {
  digitalWrite(valve_output, LOW); // close valve
  Serial.println("error status 1");

  // infinite loop - board must be manually reset if there is an error
  while (error_status == 1)
  {
    // Flash onboard LED and red NeoPixels
    fade_in("red", 3);
    digitalWrite(LED_BUILTIN, LOW);
    fade_out("red", 3);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(50);
  }
}

// Open valve and turn on NeoPixels
void turn_on() {
  if (valve_open == 0) {
    digitalWrite(LED_BUILTIN, LOW);   // LED on
    digitalWrite(valve_output, HIGH); // valve open
    timer_start = millis();           // start timer
    valve_open = 1;
    Serial.print("valve open at ");
    Serial.println(timer_start);
  }
  if (light_on == 0) {
    fade_in("blue", 5); 
    light_on = 1;
  }
  // Report error if valve has been open for longer than the specified error_time
  if (valve_open == 1) {
    current_time = millis();
    if (current_time - timer_start > error_time) {
      error_status = 1;
      error();
    }
  }
  delay(10);
}

// Close valve and turn off NeoPixels
void turn_off() {
  if (valve_open == 1) {
    digitalWrite(LED_BUILTIN, HIGH); // LED off
    digitalWrite(valve_output, LOW); // valve closed
    current_time = millis();         // get current time
    valve_open = 0;
    data_published = 0;
    Serial.print("valve closed at ");
    Serial.println(current_time);
  }
  if (light_on == 1) {
    fade_out("blue", 5);
    light_on = 0;
  }
  delay(10);
}


void loop() {
  ArduinoOTA.handle();

  // Initially connect MQTT and reconnect if connection is lost
  if (!client.connected())  
  {
    // *** uncomment next line to enable MQTT ***
    //reconnect();
  }
  client.loop();

  // Publish MQTT data
  if (data_published == 0) {
    
    // Report gallons used
    /*
    float gallons_flt = (gal_per_sec * (current_time - timer_start)) / 1000;
    String gallons_str = String(gallons_flt, 4);
    char gallons_char[50];
    Serial.print("water used: ");
    Serial.print(gallons_str);
    Serial.println(" gallons");
    gallons_str.toCharArray(gallons_char, gallons_str.length() + 1);
    client.publish(TOPIC, gallons_char);
    */

    // Report valve open time
    run_time_lng = (current_time - timer_start);
    String run_time_str = String(run_time_lng);
    char run_time_char[50];
    run_time_str.toCharArray(run_time_char, run_time_str.length() + 1);
    Serial.print("valve was open for ");
    Serial.print(run_time_str);
    Serial.println(" ms");
    client.publish(TOPIC, run_time_char);

    Serial.println("----------------------------");
    data_published = 1;
  }

  ir_state = digitalRead(ir_input);
  switch_state = digitalRead(switch_input);
  
  // button pressed
  while (switch_state == HIGH) {
    turn_on();
    switch_state = digitalRead(switch_input);
    if (switch_state == LOW) {
      turn_off();
    }
  }

  // object detected
  while (ir_state == LOW) {
    delay(ir_input_delay);
    ir_state = digitalRead(ir_input);
    if (ir_state == LOW) {
      turn_on();
    }
    else if (ir_state == HIGH) {
      turn_off();
    }
  }
  
  delay(50);
}

