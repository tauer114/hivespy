
/*
    Hivespy - Network
    === Hive Node ===
    Libraries:
    nRF24/RF24, https://github.com/nRF24/RF24
    nRF24/RF24Network, https://github.com/nRF24/RF24Network
    nrf24/RF24Mesh, https://github.com/nRF24/RF24Mesh/releases
    Low-Power, https://github.com/rocketscream/Low-Power

//  pins hard coded
//  ---------------
  D9 10 11 12 13  nrf24 CE,CSN,MOSI,MISO,SKD
  D5              DHTxx [out,in]
  D3              DS18B20 one-wire array 
  D8              Powerpin
  A0 A1           Dour,SCK,Power - HX711 
  A3              battery voltage 
*/


#include <SPI.h>
#include <RF24Network.h>
#include <RF24Mesh.h>
#include <LowPower.h>

//----------------------------------------------------------------
// Config
//----------------------------------------------------------------
uint32_t SpeicherIntervall = 300; // Speicherintervall / Versand an Master

byte Anzahl_Sensoren_DS18B20 = 0; // Mögliche Werte: '0','1','2'

byte Anzahl_Sensoren_DHT = 1; // Mögliche Werte: '0','1','2'

byte Anzahl_Sensoren_Gewicht = 1; // Mögliche Werte: '0','1'

byte DHT_Typ = 2; // Mögliche Werte: '1' = DHT21, '2' = DHT22

long Taragewicht = 101672;  // Hier ist der Wert aus der Kalibrierung einzutragen
float Skalierung = 24.52;  // Hier ist der Wert aus der Kalibrierung einzutragen

float Kalibriertemperatur = 0;       // Temperatur zum Zeitpunkt der Kalibrierung
float KorrekturwertGrammproGrad = 0; // Korrekturwert zur Temperaturkompensation - '0' für Deaktivierung

byte ClientNummer = 6; // Mögliche Werte: 1-6

const uint8_t nodeID = 6;
const String nodeName = "Meine Beute 6";
const String nodeLocation = "daheim";
const uint16_t nodeSleepTime = 60;

// Konfiguration nRF24L01 - Funk

#include <RF24.h> 
RF24 radio(9,10);
RF24Network network(radio);
RF24Mesh mesh(radio, network);

static const uint64_t pipes[6] = {0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL, 0xF0F0F0F0C3LL, 0xF0F0F0F0B4LL, 0xF0F0F0F0A5LL, 0xF0F0F0F096LL};

// Konfiguration DS18B20 - Temperatur
#include <OneWire.h> 
#include <DallasTemperature.h> 
 
#define ONE_WIRE_BUS 3
#define Sensor_Aufloesung 12
DeviceAddress Sensor_Adressen;
 
OneWire oneWire(ONE_WIRE_BUS); 
DallasTemperature sensors(&oneWire); 

// Konfiguration DHT21 / DHT22 - Temperatur und Luftfeuchte
#include <dht.h> 
dht DHT;

byte DHT_Sensor_Pin[2] = {5,6};

// Konfiguration Gewicht

#include <HX711.h>                         
HX711 scale(A1, A0); 
//------------------------------------
// Variable

float TempIn = 999.99;
float TempOut = 999.99;
float FeuchteIn = 999.99;
float FeuchteOut = 999.99;
long Gewicht = 999999;
long LetztesGewicht = 0;


float Temperatur[2] = {999.99,999.99};
float Luftfeuchte[2] = {999.99,999.99};

uint32_t LetztesIntervall = 0;
uint32_t LogDelayZeit;
//--------------------------------------
void setup() {

// Setup nRF24L01 - Funk
 radio.begin();
delay(20);
radio.setChannel(1);                // Funkkanal - Mögliche Werte: 0 - 127
radio.setAutoAck(0);
radio.setRetries(15,15);      
radio.setPALevel(RF24_PA_HIGH);     // Sendestärke darf die gesetzlichen Vorgaben des jeweiligen Landes nicht überschreiten! 
                                    // RF24_PA_MIN=-18dBm, RF24_PA_LOW=-12dBm, RF24_PA_MED=-6dBM, and RF24_PA_HIGH=0dBm
                                    
radio.openWritingPipe(pipes[ClientNummer-1]);
radio.openReadingPipe(1,pipes[0]); 
 
radio.startListening();
delay(20);	   

// Setup DS18B20 - Temperatur
if ((Anzahl_Sensoren_DS18B20 > 0) and (Anzahl_Sensoren_DS18B20 < 3)) {
  sensors.begin();
 
  for(byte i=0 ;i < sensors.getDeviceCount(); i++) {
    if(sensors.getAddress(Sensor_Adressen, i)) {
      sensors.setResolution(Sensor_Adressen, Sensor_Aufloesung);
    }
  }
}

// Setup DHT21 / DHT22 - Temperatur und Luftfeuchte

// Setup Gewicht

if (Anzahl_Sensoren_Gewicht == 1) {
  scale.set_offset(Taragewicht);  
  scale.set_scale(Skalierung);
}
  
}

//---------------------------------------------------------------------
void loop() {
  if ((millis() - LetztesIntervall) >= (SpeicherIntervall*1000)) {
    Sensor_DS18B20();
    Sensor_DHT();
    Sensor_Gewicht();
    Senden_nRF24L01();  
    LetztesIntervall = millis(); 
  }
}

// Funktion nRF24L01 - Funk
void Senden_nRF24L01() {
  long message[8] = {long(TempIn*100),long(TempOut*100),long(FeuchteIn*100),long(FeuchteOut*100),Gewicht}; 
  radio.stopListening(); 
  radio.write(&message, sizeof(message));
  radio.startListening();
  delay(20); 
}

// Funktion DS18B20 - Temperatur
void Sensor_DS18B20() {
  if ((Anzahl_Sensoren_DS18B20 > 0) and (Anzahl_Sensoren_DS18B20 < 3)) {
    sensors.requestTemperatures();
  
    for(byte i=0 ;i < Anzahl_Sensoren_DS18B20; i++) {
      if (i < sensors.getDeviceCount()) {
        for(byte j=0 ;j < 3; j++) { 
          Temperatur[i] = sensors.getTempCByIndex(i);
          if ((Temperatur[i] < 60) and (Temperatur[i] > -40)) j=10; // Werte für Fehlererkennung
          else {
            Temperatur[i] = 999.99;
            LogDelay(1000);
          }
        } 
      }
    }  
    TempIn = Temperatur[0];                                     // Hier kann die Zuordnung der Sensoren geändert werden
    if (Anzahl_Sensoren_DS18B20 == 2) TempOut = Temperatur[1];  // Hier kann die Zuordnung der Sensoren geändert werden
  }
}

// Funktion DHT21 / DHT22 - Temperatur und Luftfeuchte

void Sensor_DHT() {
  if ((Anzahl_Sensoren_DHT > 0) and (Anzahl_Sensoren_DHT < 3)) {
    for(byte i=0 ;i < Anzahl_Sensoren_DHT; i++) {
      int check;
      
      for(byte j=0 ;j < 3; j++) {
        if (j > 0) LogDelay(2000);
        
        if (DHT_Typ == 1) check = DHT.read21(DHT_Sensor_Pin[i]); 
        else check = DHT.read22(DHT_Sensor_Pin[i]); 
        
        Temperatur[i] = 999.99;
        Luftfeuchte[i] = 999.99;
        
        switch (check) {
          case DHTLIB_OK:
            Luftfeuchte[i] = DHT.humidity; 
            Temperatur[i] = DHT.temperature;
            j=10;
          break;
        }  
      }
      TempOut = Temperatur[0];         // Hier kann die Zuordnung der Sensoren geändert werden
      FeuchteOut = Luftfeuchte[0];     // Hier kann die Zuordnung der Sensoren geändert werden
      if (Anzahl_Sensoren_DHT == 2) {
        TempIn = Temperatur[1];        // Hier kann die Zuordnung der Sensoren geändert werden
        FeuchteIn = Luftfeuchte[1];    // Hier kann die Zuordnung der Sensoren geändert werden
      }
    }
  }
}  

// Funktion Gewicht
void Sensor_Gewicht() {
  if (Anzahl_Sensoren_Gewicht == 1) {
    delay(100);
    for(byte j=0 ;j < 3; j++) { // Anzahl der Widerholungen, wenn Abweichung zum letzten Gewicht zu hoch
      Gewicht= scale.get_units(10);
      if ((Gewicht-LetztesGewicht < 500) and (Gewicht-LetztesGewicht > -500)) j=10; // Abweichung für Fehlererkennung
      if (j < 3) {
        LogDelay(3000);  // Wartezeit zwischen Wiederholungen      
      }
    } 
    // Temperaturkompensation
    if ((TempOut != 999.99)){
      if (TempOut > Kalibriertemperatur) Gewicht = Gewicht-(fabs(TempOut-Kalibriertemperatur)*KorrekturwertGrammproGrad); 
      if (TempOut < Kalibriertemperatur) Gewicht = Gewicht+(fabs(TempOut-Kalibriertemperatur)*KorrekturwertGrammproGrad);
    } 
    
    LetztesGewicht = Gewicht;
  }
}

// Funktion LogDelay

void LogDelay(long Zeit) {
  LogDelayZeit = millis(); 
  
  while ((millis() - LogDelayZeit) < (Zeit));
}  
