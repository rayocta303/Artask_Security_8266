//PROJECT MAS ALFIEYAN
//LUNAS
#define USE_CLIENTSSL false
#include <Wire.h>
#include <AsyncTelegram2.h>
#include <SPI.h>
#include <MFRC522.h>
#include <SoftwareSerial.h>
// Timezone definition
#include <time.h>
#define MYTZ "CET-1CEST,M3.5.0,M10.5.0/3"

//RFID
#define RST_PIN D1
#define SDA_PIN D2
MFRC522 mfrc522(SDA_PIN, RST_PIN);
//WIRING PIN RFID
//3V    3v3
//RST   D1
//GND   GND
//MISO  D6
//MOSI  D7
//SCK   D5
//SDA   D2


//Dekllarasi PIN I/O
int alarm = D3;
int lock = D4;
int sensor = A0;

// Pengaturan Delay selenoid
//NOTE : JANGAN SETTING LEBIH DARI 10000
//Karena Dapat Menyebabkan Kerusakan Coil Pada Selenoid
int delay_buka = 5000;//5000ms -/+ 5 Detik

//Delay Panggilan
int delay_call = 17000;

#define GSM_RX D1 //RX D1 
#define GSM_TX D8 //TX D8
SoftwareSerial SIM800L(GSM_RX, GSM_TX);

bool armed = 1; // 1 = Sistem Keamanan Default ON || 0 = Default OFF
int bacaSensor = 0; //Nilai Default
bool s_rfid = 0; //Akses Default RFID 1=ON || 0=Off

#ifdef ESP8266
#include <ESP8266WiFi.h>
BearSSL::WiFiClientSecure client;
BearSSL::Session   session;
BearSSL::X509List  certificate(telegram_cert);

#elif defined(ESP32)
#include <WiFi.h>
#include <WiFiClient.h>
#if USE_CLIENTSSL
#include <SSLClient.h>
#include "tg_certificate.h"
WiFiClient base_client;
SSLClient client(base_client, TAs, (size_t)TAs_NUM, A0, 1, SSLClient::SSL_ERROR);
#else
#include <WiFiClientSecure.h>
WiFiClientSecure client;
#endif
#endif



AsyncTelegram2 myBot(client);
const char* ssid  =  "Artask.ID";     // SSID WiFi network
const char* pass  =  "";     // Password  WiFi network
const char* token =  "5569518833:AAH5ENxIEiGJQZjmmfwePREwRM2CqIqd2p0";  // Telegram token

void setup() {
  digitalWrite(alarm, HIGH);
  digitalWrite(lock, HIGH);
  SIM800L.begin(9600);
  SPI.begin();
  mfrc522.PCD_Init();
  pinMode(alarm, OUTPUT);
  pinMode(lock, OUTPUT);
  pinMode(sensor, INPUT);
  // initialize the Serial
  Serial.begin(115200);
  Serial.println("\nStarting Telegram BOT...");

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  delay(500);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(500);
  }

#ifdef ESP8266
  // Sync time with NTP, to check properly Telegram certificate
  configTime(MYTZ, "time.google.com", "time.windows.com", "pool.ntp.org");
  //Set certficate, session and some other base client properies
  client.setSession(&session);
  client.setTrustAnchors(&certificate);
  client.setBufferSizes(1024, 1024);
#elif defined(ESP32)
  // Sync time with NTP
  configTzTime(MYTZ, "time.google.com", "time.windows.com", "pool.ntp.org");
#if USE_CLIENTSSL == false
  client.setCACert(telegram_cert);
#endif
#endif
  // Set the Telegram bot properies
  myBot.setUpdateTime(1000);
  myBot.setTelegramToken(token);

  // Check if all things are ok
  Serial.print("\nTest Telegram connection... ");
  myBot.begin() ? Serial.println("OK") : Serial.println("NOK");

  Serial.print("Bot name: @");
  Serial.println(myBot.getBotName());

  Serial.println("Sistem Online");
  TBMessage msg;
  myBot.sendMessage(msg,  "Sistem Online\nKlik /Start Untuk Memunculkan Menu");

}

void loop() {

  // local variable to store telegram message data
  TBMessage msg;

  //Baca Nilai sensor
  bacaSensor = analogRead(sensor);
  float nilaiSensor = bacaSensor * (5.0 / 1023.0);

  // if there is an incoming message...
  if (myBot.getNewMessage(msg)) {
    String msgText = msg.text;

    if (msgText.equals("Kamu Siapa?")) {
      myBot.sendMessage(msg, "Saya adalah Sistem Keamanan Pintar berbasis ESP-8266 yang di buat oleh Artask Media. \nSistem keamanan ini ter-integrasi dengan BOT Telegram melalui Rest API menggunakan metode HTTPS Request");       // notify the sender
    }
    else if (msgText.equals("/buka")) {
      Serial.println("Selenoid ON ");
      myBot.sendMessage(msg,  "Pintu di buka via telegram \nSistem Keamanan Di Matikan");
      digitalWrite(alarm, HIGH);
      digitalWrite(lock, LOW);
      delay(delay_buka);
      digitalWrite(lock, HIGH);
      Serial.println("Selenoid OFF");
    }
    else if (msgText.equals("/armed")) {
      Serial.println("Sistem Keamanan Di Aktifkan");
      myBot.sendMessage(msg, "Sistem Keamanan Di Aktifkan");
      armed = 1;
    }
    else if (msgText.equals("/disarm")) {
      Serial.println("Sistem Keamanan Di Matikan");
      myBot.sendMessage(msg, "Sistem Keamanan Di Matikan");
      armed = 0;
    }
    else if (msgText.equals("/rfid_on")) {
      Serial.println("Akses RFID Di Aktifkan");
      myBot.sendMessage(msg, "Akses RFID Di Aktifkan");
      s_rfid = 1;
    }
    else if (msgText.equals("/rfid_off")) {
      Serial.println("Akses RFID Di Matikan");
      myBot.sendMessage(msg, "Akses RFID Di Matikan");
      s_rfid = 0;
    }
    else if (msgText.equals("/alarm_off")) {
      Serial.println("Alarm Dimatikan");
      digitalWrite(alarm, HIGH);
      myBot.sendMessage(msg, "Alarm Di Matikan");

    }
    else if (msgText.equals("/call")) {
      Serial.println("Miscall Nomor Satu dimulai");
      myBot.sendMessage(msg, "Miscall Dimulai");
      delay(1000);
      SIM800L.write("ATD08124190xxxx;\r\n");//masukan nomor tujuan
      delay(delay_call); //delay panggilan
      SIM800L.write("ATH\r\n");//hangup panggilan sesuai delay
      Serial.println("Miscall Satu Selesai");
      myBot.sendMessage(msg, "Miscall Selesai");

    }
    else {
      // generate the message for the sender
      String welcome = "Selamat Datang .\n\n";
      welcome += "Daftar Perintah : \n";
      welcome += "/armed    : Mengaktifkan sistem keamanan\n";
      welcome += "/disarm   : Mematikan sistem keamanan\n";
      welcome += "/rfid_on  : Mengaktifkan akses RFID\n";
      welcome += "/rfid_off : Mematikan akses RFID\n";
      welcome += "/alarm_off: Mematikan Alarm \n";
      welcome += "/buka     : Akses selama 5 detik\n";
      welcome += "/call     : Miscall No Darurat -/+ 15-20 detik\n";
      myBot.sendMessage(msg, welcome);                    // and send it
    }
  }
  if (armed) {
    //    //Print Nilai Sensor
    //    Serial.print("\n Nilai Sensor = ");
    //    Serial.print(nilaiSensor);
    if (nilaiSensor > 3 ) {
      digitalWrite(alarm, LOW);//ON
      myBot.sendMessage(msg, "Pintu Dibuka Paksa!");
      myBot.sendMessage(msg, "Status Alarm Hidup \nKlik /alarm_off Untuk Mematikan Alarm");
      Serial.println("Miscall Nomor Satu dimulai");
      myBot.sendMessage(msg, "Miscall Dimulai");
      delay(1000);
      SIM800L.write("ATD08124190xxxx;\r\n");//masukan nomor tujuan
      delay(delay_call); //delay panggilan
      SIM800L.write("ATH\r\n");//hangup panggilan sesuai delay
      Serial.println("Miscall Satu Selesai");
      myBot.sendMessage(msg, "Miscall Selesai");
    }
  }
  // Look for new cards
  if ( ! mfrc522.PICC_IsNewCardPresent())
  {
    return;
  }
  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial())
  {
    return;
  }

  //Show UID on serial monitor
  Serial.print("UID tag :");

  String content = "";
  byte letter;
  for (byte i = 0; i < mfrc522.uid.size; i++)
  {
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
    content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  Serial.println();
  Serial.print("Message : ");
  content.toUpperCase();
  if (s_rfid) {
    if (content.substring(1) == "A9 12 CF 50") // UID KARTU 1
    {
      armed = 0;
      digitalWrite(alarm, HIGH);//OFF
      Serial.println("Akses Diterima");
      Serial.println();
      //Send notification
      myBot.sendMessage(msg, "Pintu Dibuka dengan RFID 1 \nSistem Keamanan Dimatikan");
      delay(500);
      digitalWrite(lock, LOW);//ON
      delay(delay_buka);
      digitalWrite(lock, HIGH);//OFF
      delay(100);
    }
    else if (content.substring(1) == "6B 7F 65 B4") //UID KARTU 2
    {
      armed = 0;
      digitalWrite(alarm, HIGH);//OFF
      Serial.println("Akses Diterima");
      Serial.println();
      //Send notification
      myBot.sendMessage(msg, "Pintu Dibuka dengan RFID 2 \nSistem Keamanan Dimatikan");
      delay(500);
      digitalWrite(lock, LOW);//ON
      delay(delay_buka);
      digitalWrite(lock, HIGH);//OFF
      delay(100);
    } else {
      digitalWrite(lock, HIGH);//OFF
      digitalWrite(alarm, LOW);//ON
      Serial.println(" Access Ditolak");
      //Send notification
      myBot.sendMessage(msg, "WARNING!!!\nAda Percobaan Akses RFID\nUID: " + String(content.substring(1)));
      Serial.println(content.substring(1));

      myBot.sendMessage(msg, "Status Alarm Hidup \nKlik /alarm_off Untuk Mematikan Alarm");
      Serial.println("Miscall Nomor Satu dimulai");
      myBot.sendMessage(msg, "Miscall Dimulai");
      delay(1000);
      SIM800L.write("ATD08124190xxxx;\r\n");//masukan nomor tujuan
      delay(delay_call); //delay panggilan
      SIM800L.write("ATH\r\n");//hangup panggilan sesuai delay
      Serial.println("Miscall Satu Selesai");
      myBot.sendMessage(msg, "Miscall Selesai");

    }
  }
}
