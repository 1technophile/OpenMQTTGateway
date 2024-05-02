#include <LoRa.h>
#include <SPI.h>
#include <WiFi.h>
#include <Wire.h>
#include <stdio.h>

#include "SSD1306.h"
#include "rom/ets_sys.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/sens_reg.h"

#define SCK  5 // GPIO5  -- SX1278's SCK
#define MISO 19 // GPIO19 -- SX1278's MISnO
#define MOSI 27 // GPIO27 -- SX1278's MOSI
#define SS   18 // GPIO18 -- SX1278's CS
#define RST  23 // GPIO23 -- SX1278's RESET
#define DI0  26 // GPIO26 -- SX1278's IRQ(Interrupt Request)
#define BAND 433E6

//OLED pins
#define OLED_SDA 21
#define OLED_SCL 22 
//#define OLED_RST 16

#define LED      25

#define DHT_PIN  12
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

DHT dht(DHT_PIN, DHT_TYPE); //Inizializza oggetto chiamato "dht", parametri: pin a cui è connesso il sensore, tipo di dht 11/22

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

  delay(1500);
}

void loop() {
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);

  display.drawString(0, 0, "Sending packet: ");
  display.drawString(90, 0, String(cntpkt));

  String NodeId = WiFi.macAddress();
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  float battery = getBattery();
  // send packet
  LoRa.beginPacket();
  // Build json string to send
  //per test rimuovere

  String msg = "{\"model\":\"ESP32CNT\",\"id\":\"" + NodeId + "\",\"count\":\"" + String(counter) + "\",\"tempc\":\"" + String(temp) + "\",\"hum\":\"" + String(hum) + "\",\"batt\":\"" + String(battery) + "\"}";
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
  delay(60000); // wait for 60 seconds
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

