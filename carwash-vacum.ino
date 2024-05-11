#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Arduino.h>

#define I2C_ADDR 0x27
#define LCD_ROWS 6
#define LCD_COLS 16
#define POWER_PIN 6
#define START_BUTTON_PIN 3
#define STOP_BUTTON_PIN 4
#define COIN_ACCEPTOR_PIN 2
#define SS_PIN 10
#define RST_PIN 5
// variable use to measure the intervals inbetween impulses
int i = 0;

// Flag to check if a coin has been inserted
bool coinInserted = false;

// Hier fügen wir die Definitionen für die Master-UIDs hinzu
byte masterCardUID1[] = {0xB3, 0xFC, 0x5C, 0xF7};
byte masterCardUID2[] = {0x53, 0xA4, 0x21, 0x25};

LiquidCrystal_I2C lcd(I2C_ADDR, LCD_COLS, LCD_ROWS);
MFRC522 rfid(SS_PIN, RST_PIN);

unsigned long previousMillis = 0;
long interval = 300000; // 5 Minuten in Millisekunden
bool timerStarted = false;
bool activated = false;
unsigned long remainingTime = interval; // Verbleibende Zeit initial auf Intervall setzen
int impulsCount = 0;
float total_amount = 0;

void setup() {
  lcd.init();
  lcd.backlight(); // Schalte die Hintergrundbeleuchtung ein
  lcd.setCursor(0, 0);
  lcd.print("Praona, ");
  lcd.setCursor(0, 1);
  lcd.print("je otvorena!");
  // Ändere die Helligkeit auf 50% (Beispielwert, anpassen nach Bedarf)
  lcd.setBacklight(128);

  pinMode(POWER_PIN, OUTPUT); // Konfiguriere Pin 2 als Ausgang
  pinMode(START_BUTTON_PIN, INPUT_PULLUP); // Konfiguriere Pin 3 als Eingang mit Pull-Up-Widerstand
  pinMode(STOP_BUTTON_PIN, INPUT_PULLUP); // Konfiguriere Pin 4 als Eingang mit Pull-Up-Widerstand
  pinMode(COIN_ACCEPTOR_PIN, INPUT_PULLUP); // Konfiguriere Pin 2 des Münzakzeptors als Eingang mit Pull-Up-Widerstand
  digitalWrite(POWER_PIN, HIGH); // Schalte den Pin 2 ein

   pinMode(COIN_ACCEPTOR_PIN, INPUT_PULLUP); // Configure coin acceptor pin
  Serial.begin(9600);
  attachInterrupt(digitalPinToInterrupt(COIN_ACCEPTOR_PIN), incomingImpuls, FALLING); // Attach interrupt to coin acceptor pin

  Serial.begin(9600);
  SPI.begin(); // init SPI bus
  rfid.PCD_Init(); // init MFRC522
  digitalWrite(POWER_PIN, LOW); 

  Serial.println("Tap RFID/NFC Tag on reader");
}


void incomingImpuls() {
  impulsCount = impulsCount + 1;
  coinInserted = true;
  i = 0;
}

void loop() {
  unsigned long currentMillis = millis();
  unsigned long elapsedTime = currentMillis - previousMillis;

  i = i + 1;

  if (coinInserted) { // Check if a coin has been inserted
    Serial.print("i=");
    Serial.print(i);
    Serial.print(" Impulses:");
    Serial.print(impulsCount);
    Serial.print(" Total:");
    Serial.println(total_amount);
   
    delay(1000);

    if (impulsCount == 2) {
      total_amount = total_amount + 10;
      impulsCount = 0;
    }
    if (impulsCount == 3) {
      total_amount = total_amount + 4;
      impulsCount = 0;
    }
    if (impulsCount == 4) {
      total_amount = total_amount + 2;
      impulsCount = 0;
    }
    if (impulsCount == 5) {
      total_amount = total_amount + 1;
      impulsCount = 0;
    }
    if (impulsCount == 6) {
      total_amount = total_amount + 0.4;
      impulsCount = 0;
    }

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Novac");
    lcd.setCursor(0, 1);
    lcd.print(total_amount/2); 

    Serial.print("Total amount: ");
    Serial.println(total_amount);

    interval = total_amount * 60000;
    coinInserted = false; // Reset the coinInserted flag
  }

  handleStartButton(currentMillis, elapsedTime);
  handleStopButton(currentMillis, elapsedTime);
  handleRFID(currentMillis, elapsedTime);

  if(timerStarted){
    updateTimer(currentMillis, elapsedTime);
  }
}

// Rest des Codes bleibt unverändert...



// Funktion zur Überprüfung, ob die UID mit einer der Master-UIDs übereinstimmt
bool checkMasterCard(byte uid[]) {
  if (memcmp(uid, masterCardUID1, sizeof(masterCardUID1)) == 0 || memcmp(uid, masterCardUID2, sizeof(masterCardUID2)) == 0) {
    return true; // UID stimmt mit einer der Master-UIDs überein
  }
  return false; // UID stimmt mit keiner der Master-UIDs überein
}

// Funktion zur Aktualisierung des LCD-Displays mit der verbleibenden Zeit
void updateDisplay(unsigned long remainingTime) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Preostalo");
  lcd.setCursor(0, 1);
  lcd.print(remainingTime / 60000); // Minuten
  lcd.print(" min ");
  lcd.print((remainingTime / 1000) % 60); // Sekunden
  lcd.print(" sec");
}

void handleStartButton(unsigned long currentMillis, unsigned long elapsedTime) {
  if ((digitalRead(START_BUTTON_PIN) == LOW && activated) || (digitalRead(START_BUTTON_PIN) == LOW && total_amount > 0)) {
    if (!timerStarted) {
      // Falls Timer gestoppt war, setze die verbleibende Zeit entsprechend fort
      remainingTime = interval - elapsedTime;
      previousMillis = currentMillis - (interval - remainingTime);
      timerStarted = true;
      digitalWrite(POWER_PIN, HIGH); // Schalte das Relais ein
    }
  }
}



void handleStopButton(unsigned long currentMillis, unsigned long elapsedTime) {
  if (digitalRead(STOP_BUTTON_PIN) == LOW && activated || (digitalRead(STOP_BUTTON_PIN) == LOW && total_amount > 0)) {
    timerStarted = false;
    digitalWrite(POWER_PIN, LOW); 
    // Speichere die verbleibende Zeit beim Stoppen des Timers
    remainingTime = interval - elapsedTime;
    if (remainingTime < 0) {
      remainingTime = 0; // Stellen Sie sicher, dass die verbleibende Zeit nicht negativ ist
    }
  }
}


void handleRFID(unsigned long currentMillis, unsigned long elapsedTime) {
  if (rfid.PICC_IsNewCardPresent()) { // new tag is available
    if (rfid.PICC_ReadCardSerial()) { // NUID has been readed
      if (checkMasterCard(rfid.uid.uidByte)) { // Überprüfe, ob die UID einem der Master-Tags entspricht
        interval = 300000;
        previousMillis = currentMillis; // Starte den Timer
        timerStarted = true; // Markiere den Timer als gestartet
        digitalWrite(POWER_PIN, HIGH); 
        activated = true;

        MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
        //Serial.print("RFID/NFC Tag Type: ");
        //Serial.println(rfid.PICC_GetTypeName(piccType));

        // print NUID in Serial Monitor in the hex format
        Serial.print("UID:");
        for (int i = 0; i < rfid.uid.size; i++) {
          Serial.print(rfid.uid.uidByte[i] < 0x10 ? " 0" : " ");
          Serial.print(rfid.uid.uidByte[i], HEX);
        }
        Serial.println();

        rfid.PICC_HaltA(); // halt PICC
        rfid.PCD_StopCrypto1(); // stop encryption on PCD
      }
    }
  }
}

void updateTimer(unsigned long currentMillis, unsigned long elapsedTime) {
  unsigned long remainingTime = interval - (currentMillis - previousMillis);
  updateDisplay(remainingTime);
  if (elapsedTime > interval) {
    // Timer abgelaufen
    activated = false;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Isteklo vrijeme!");
    digitalWrite(POWER_PIN, LOW); // Schalte den Pin 2 aus
    timerStarted = false; // Markiere den Timer als nicht mehr gestartet
    setup();
    total_amount = 0;
  }
  else {
    digitalWrite(POWER_PIN, HIGH); 
  }
}


// Funktion zur Berechnung des Wertes der eingeworfenen Münze basierend auf der Impulsanzahl
float getCoinValue(int impulsCount) {
  switch (impulsCount) {
    case 2:
      return 300;
    case 3:
      return 120;
    case 4:
      return 60;
    case 5:
      return 30;
    case 6:
      return 18;
    default:
      return 0;
  }
}
