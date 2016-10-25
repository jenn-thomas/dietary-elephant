// included libraries
#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>
#include <SoftwareSerial.h>
#include "Adafruit_Soundboard.h"

// pins for SPI
#define RST_PIN         5           
#define SS_PIN          53    
// software serial pins
#define SFX_TX 12
#define SFX_RX 11
// Connect to the RST pin on the Sound Board
#define SFX_RST 13
// pins for attachInterrupt on Mega 2, 3, 18, 19, 20, 21
#define pin0            18     
#define pin1            19 
#define pin2            20  
#define pin3            21

// sound files
//0  name: NO2     WAV size: 820320
//1 name: PEANUT  WAV size: 1502432
//2 name: VEGETA~1WAV size: 1524720
//3 name: YES0    WAV size: 713324
//4 name: YES1    WAV size: 664284
//5 name: DAIRY   WAV size: 1582680
//6 name: GLUTEN  WAV size: 1453392
//7 name: NO0     WAV size: 691032

// sound file yes and no location
int no[] = {0,7};
int yes[] = {3,4};

// RFID block allergy mode locations
// 62 - peanuts
// 61 - milk
// 60 - vegetarian
// 54 - gluten

// values that may change from mode change on an attach interrupt
volatile int blockNumber = 62; 
volatile int modeChange = 0;

// Adafruit Soundboard
SoftwareSerial ss = SoftwareSerial(SFX_TX, SFX_RX);
Adafruit_Soundboard sfx = Adafruit_Soundboard(&ss, NULL, SFX_RST);

MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.
MFRC522::MIFARE_Key key;
MFRC522::StatusCode status;
// initialize servos
Servo head;
Servo trunk;

void setup() {
    Serial.begin(115200); // Initialize serial communications with the PC
    while (!Serial);    // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
    SPI.begin();        // Init SPI bus
    mfrc522.PCD_Init(); // Init MFRC522 card
    ss.begin(9600); // soundboard
    if (!sfx.reset()) {
       Serial.println("Not found");
     } else {
     Serial.println("SFX board found");
     }
    // Init buttons for change of allergy mode
    pinMode(pin0, OUTPUT);
    attachInterrupt(digitalPinToInterrupt(pin0), pin0Change, RISING);
    pinMode(pin1, OUTPUT);
    attachInterrupt(digitalPinToInterrupt(pin1), pin1Change, RISING);
    pinMode(pin2, OUTPUT);
    attachInterrupt(digitalPinToInterrupt(pin2), pin2Change, RISING);
    pinMode(pin3, OUTPUT);
    attachInterrupt(digitalPinToInterrupt(pin3), pin3Change, RISING);
    // init servos
    head.attach(9);
    head.writeMicroseconds(1500);
    trunk.attach(10);
    trunk.writeMicroseconds(1500);

    // Prepare the key (used both as key A and as key B)
    // using FFFFFFFFFFFFh which is the default at chip delivery from the factory
    for (byte i = 0; i < 6; i++) {
        key.keyByte[i] = 0xFF;
    }
    // tell us what allergy you have Peanut!
    playAllergyType();
}

void loop() {
    int mode;
    byte arrayAddress[18];
    // for the most part, we want the servos to stay put
    mode = 10;
    movePeanut(mode);
    //If the mode changes, tell us his allergy
    if (modeChange) {
      sfx.stop();
      Serial.println("Mode changed! Mode: ");
      Serial.println(blockNumber);
      playAllergyType();
      modeChange = 0;
    }
    // Look for new cards
    if ( ! mfrc522.PICC_IsNewCardPresent())
        return;

    // Select one of the cards
    if ( ! mfrc522.PICC_ReadCardSerial())
        return;

          MFRC522::StatusCode status;
          
          int largestModulo4Number=blockNumber/4*4;
           int trailerBlock=largestModulo4Number+3;//determine trailer block for the sector

  // authentication of the desired block for access
  status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
         Serial.print("PCD_Authenticate() failed (read): ");
         Serial.println(mfrc522.GetStatusCodeName(status));
         return;
  }

  // read a block
  byte buffersize = 18;//we need to define a variable with the read buffer size, since the MIFARE_Read method below needs a pointer to the variable that contains the size... 
  status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blockNumber, arrayAddress, &buffersize);//&buffersize is a pointer to the buffersize variable; MIFARE_Read requires a pointer instead of just a number
  if (status != MFRC522::STATUS_OK) {
          Serial.print("MIFARE_read() failed: ");
          Serial.println(mfrc522.GetStatusCodeName(status));
  }
  Serial.println("block was read: ");
  dump_byte_array(arrayAddress,1);

  // Halt PICC
  mfrc522.PICC_HaltA();
  // Stop encryption on PCD
  mfrc522.PCD_StopCrypto1();
}

void dump_byte_array(byte *buffer, byte bufferSize) {
    int mode;
    for (byte i = 0; i < bufferSize; i++) {
        Serial.print(buffer[i] < 0x10 ? " 0" : " ");
        Serial.print(buffer[i]);
        if (buffer[i] == 48){
          mode = 0;
          movePeanut(mode);
        }
        else if (buffer[i] == 49) {
          mode = 1;
          movePeanut(mode);
        }
    }
}

void movePeanut(int mode){
  // if he can't have the food
  if (mode == 0){
      sfx.playTrack(no[rand()%2]); 
    for (int i=0; i <=2; i++){
      head.writeMicroseconds(1675);
      delay(700);
      head.writeMicroseconds(1325);
      delay(700);
    }
    head.writeMicroseconds(1500);
  }
  // if he can have the food
  else if (mode == 1){
      sfx.playTrack(yes[rand()%2]); 
    for (int i=0; i <=2; i++){
      trunk.writeMicroseconds(1575);
      delay(500);
      trunk.writeMicroseconds(1425);
      delay(500);
    }
    trunk.writeMicroseconds(1500);
  }
  // else just stay put
  else {
    head.writeMicroseconds(1500);
    trunk.writeMicroseconds(1500);
  }
}

// switch to peanut allergy
void pin0Change(){
  if (blockNumber == 61){
    modeChange = 0;
  }
  else{
    modeChange = 1;
  }
  blockNumber = 61; 
 }
// switch to lactose intolerant
void pin1Change(){
  if (blockNumber == 54){
    modeChange = 0;
  }
  else{
    modeChange = 1;
  }
  blockNumber = 54; 
}
// switch to vegetarian
void pin2Change(){
  if (blockNumber == 62){
    modeChange = 0;
  }
  else{
    modeChange = 1;
  }
  blockNumber = 62; 
}
// switch to gluten intolerance
void pin3Change(){
  if (blockNumber == 60){
    modeChange = 0;
  }
  else{
    modeChange = 1;
  }
  blockNumber = 60; 
}

// if Peanut changes modes
void playAllergyType() {
    if (blockNumber == 62){
        sfx.playTrack(1);
        if (! sfx.playTrack(1) ) {
           sfx.playTrack(1);
           Serial.println("Failed to play track?");
         }
      }
      else if (blockNumber == 61){
        sfx.playTrack(5);
        if (! sfx.playTrack(5) ) {
          Serial.println("Failed to play track?");
      }
      }
      else if (blockNumber == 60){
        sfx.playTrack(2);
        if (! sfx.playTrack(2) ) {
          Serial.println("Failed to play track?");
        }
      }
      else if (blockNumber == 54){
        sfx.playTrack(6);
         if (! sfx.playTrack(6) ) {
          Serial.println("Failed to play track?");
        }
     }

}




