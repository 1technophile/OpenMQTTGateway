#include <LoRa.h>
#include <SPI.h>
#include <WiFi.h>
#include <Wire.h>
#include <stdio.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

#include "SSD1306.h"

//LoRa pins
#define SCK   5 // GPIO5  -- SX1278's SCK
#define MISO 19 // GPIO19 -- SX1278's MISnO
#define MOSI 27 // GPIO27 -- SX1278's MOSI
#define SS   18 // GPIO18 -- SX1278's CS
#define RST  23 // GPIO14 -- SX1278's RESET
#define DI0  26 // GPIO26 -- SX1278's IRQ(Interrupt Request)
#define BAND 433E6

//OLED pins
#define OLED_SDA 21
#define OLED_SCL 22 
//#define OLED_RST 16

#define LED      25

#define DHT_PIN  14
#define DHT_TYPE DHT22

#define CNT_PIN  34

#define BAT_PIN  35

const int MAX_ANALOG_VAL = 4095;
const float MAX_BATTERY_VOLTAGE = 4.2;

int cntpkt = 0;
unsigned long counter = 0;
unsigned long last_interrupt_time = 0;
int pulse_seen = 0;                                                   // pulse detected by interrupt function
int bounce_delay_ms = 100;

SSD1306 display(0x3c, OLED_SDA, OLED_SCL);
String rssi = "RSSI --";
String packSize = "--";
String packet;

DHT_Unified dht(DHT_PIN, DHT_TYPE); //Inizializza oggetto chiamato "dht", parametri: pin a cui è connesso il sensore, tipo di dht 11/22

void ICACHE_RAM_ATTR bounceCheck ();

void setup() {
  pinMode(LED, OUTPUT);

  //pinMode(OLED_RST, OUTPUT);
  //digitalWrite(OLED_RST, LOW); // set GPIO16 low to reset OLED
  //delay(50);
  //digitalWrite(OLED_RST, HIGH); // while OLED is running, must set GPIO16 in high

  Serial.begin(115200);
  while (!Serial);
  Serial.println();
  Serial.println("LoRa Sender Test");

  SPI.begin(SCK, MISO, MOSI, SS);
  LoRa.setPins(SS, RST, DI0);
  if (!LoRa.begin(BAND)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }

  Serial.println("init ok");
  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);

  pinMode(BAT_PIN, INPUT); 
  pinMode(CNT_PIN, INPUT_PULLUP);
  
  attachInterrupt (CNT_PIN, bounceCheck, RISING);

  dht.begin();
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  Serial.println(F("------------------------------------"));
  Serial.println(F("Temperature Sensor"));
  Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
  Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
  Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
  Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("°C"));
  Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("°C"));
  Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("°C"));
  Serial.println(F("------------------------------------"));
  // Print humidity sensor details.
  dht.humidity().getSensor(&sensor);
  Serial.println(F("Humidity Sensor"));
  Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
  Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
  Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
  Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("%"));
  Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("%"));
  Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("%"));
  Serial.println(F("------------------------------------"));

  delay(1500);
}

void loop() {
  sensors_event_t event1, event2;

  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);

  display.drawString(0, 0, "Sending packet: ");
  display.drawString(90, 0, String(cntpkt));

  String NodeId = WiFi.macAddress();
 // float temp = dht.readTemperature();
 // float hum = dht.readHumidity();
  float battery = getBattery();

  dht.temperature().getEvent(&event1);
  if (isnan(event1.temperature)) {
    Serial.println(F("Error reading temperature!"));
  }
  else {
    Serial.print(F("Temperature: "));
    Serial.print(event1.temperature);
    Serial.println(F("°C"));
  }
  // Get humidity event and print its value.
  dht.humidity().getEvent(&event2);
  if (isnan(event2.relative_humidity)) {
    Serial.println(F("Error reading humidity!"));
  }
  else {
    Serial.print(F("Humidity: "));
    Serial.print(event2.relative_humidity);
    Serial.println(F("%"));
  }

  // send packet
  LoRa.beginPacket();

  String msg = "{\"model\":\"ESP32CNT\",\"id\":\"" + NodeId + "\",\"count\":\"" + String(counter) + "\",\"tempc\":\"" + String(event1.temperature) + "\",\"hum\":\"" + String(event2.relative_humidity) + "\",\"batt\":\"" + String(battery) + "\"}";
  // Send json string
  LoRa.print(msg);
  LoRa.endPacket();

  Serial.println(msg);
  
  display.drawString(0, 15, String(NodeId));
  display.drawString(0, 30, "count: " + String(counter));
  display.drawString(0, 45, "battery: " + String(battery)+ " %");
  display.display();

  delay(5000);

  cntpkt++;

  digitalWrite(LED, HIGH); // turn the LED on (HIGH is the voltage level)
  delay(1000); // wait for a second
  digitalWrite(LED, LOW); // turn the LED off by making the voltage LOW
  //delay(60000); // wait for 60 seconds
  delay(6000);
}

void ICACHE_RAM_ATTR bounceCheck (){
   unsigned long interrupt_time = millis();
   if (interrupt_time - last_interrupt_time > bounce_delay_ms) counter++;    // void loop() then notes pulse == 1 and takes action      
   last_interrupt_time = interrupt_time;
}

float getBattery(void) {
  int rawValue = analogRead(BAT_PIN);

  float voltageLevel = (rawValue / 4095.0) * 2 * 1.1 * 3.3; // calculate voltage level
  float perc = voltageLevel / MAX_BATTERY_VOLTAGE * 100;

  return(perc);
}

