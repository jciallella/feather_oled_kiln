/* ===========================================================================================*/
/*                                                                                            */
/*      Arduino Kiln by Jack D. Ciallella                                                     */
/*      Adafruit TFT Feather Wing & Adafruit Feather Adalogger                                */
/*      :: Alpha Version ::                                                                   */
/*      Last Updated 6/2017                                                                   */
/*                                                                                            */
/* ===========================================================================================*/

/* Includes */
#include <SD.h>
#include <SPI.h>
#include <Wire.h>
#include <EEPROM.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "Adafruit_HTU21DF.h"

/* Hardware */
Adafruit_SSD1306 display = Adafruit_SSD1306();
Adafruit_HTU21DF htu = Adafruit_HTU21DF();

/* Pin Mapping */

#define scrnButtonC    5        // C. Screen Power
#define hitsButtonB    6        // B. Resets Hits       [ 6 = Pin A7]
#define tempButtonA    9        // A. Sets Temperature  [ 9 = Pin A9]
#define mosfetPin      10       // Output to Mosfet
#define rdboardLED     13       // On-board LED
#define thermoPin      A2       // TMP36 Temp Sensor
#define intakeFan               // Fresh Air Fan
#define internalFans            // Kiln Air Circulation Fans

#define extLedPin 11

/* Enable / Disable */
#define VBAT_ENABLED   0        // Enable / Disable integrated batt mgmt

/* Debounce / Button Hold */
#define debounce       300       // prevent button noise

/* Temperature Variables   (A) */
int tButtonCount =  1;           // press counter
int tButtonState =  0;           // current state
int tButtonPrev =   0;           // previous state
int firePower =     0;           // power output to mosfet

/* Hit Counter Variables   (B) */
int hButtonCount =  0;
int hButtonState =  0;
int hButtonPrev =   0;

/* Screen Variables        (C) */
int sButtonCount =  0;
int sButtonState =  0;
int sButtonPrev =   0;
int screenOff =     0;
boolean screenFlip = true;           // Screen Inversion State (Light or Dark)

/* Fire Button Variables   (E) */
int fButtonCount =  0;
int fButtonState =  0;
int fButtonPrev =   0;
int preHeatStage =  0;             // 0, 1, 2, 3 :: OFF, Countdown, Animation, Static
int preHeatOn =     0;             // 0, 1 :: OFF, ON

/* Temperature Variables */
float temperatureF;                 // Temperature Reading
int avgTempRead =   0;              // Average Temperature
const int numReadings = 85;         // smooths voltage readings
const float calcVolt = 3.3;         // Tested Board Voltage [3.228]

/* LED Variables */
int epAddress =     0;              // Current EEPROM Address / byte

/* Held Button Variables */
int current;                         // Current state of button (LOW = Pressed for pullup resistors)
long millis_held;                    // How long the button was held (milliseconds)
long secs_held;                      // How long the button was held (seconds)
long prev_secs_held;                 // How long the button was held in the previous check
byte previous = HIGH;
unsigned long firstTime;             // How long since the button was first pressed

int iCount = 0;

/*Newest*/
int realHumid;
float realTempC;
int realTempF;
unsigned long milliTime;
long previousMillis = 0;
long loginterval = 30;                // In Seconds
int humidHigh = 0;
int humidLow = 150;
int tempHigh = 0;
int tempLow = 150;
bool ledState = true;
bool ventFanStatus = true;

/* Read Write Data */
File myFile;
File myFileTwo;
File plotFile;

/* ===========================================================================================*/
/*                                                                                            */
/*     SETUP PROGRAM                                                                          */
/*                                                                                            */
/* ===========================================================================================*/

void setup()
{
  /* Set Pin Input/Output & Pullups */
  pinMode(hitsButtonB,   INPUT_PULLUP);
  pinMode(scrnButtonC,   INPUT_PULLUP);
  pinMode(tempButtonA,   INPUT);
  pinMode(thermoPin,     INPUT);
  pinMode(rdboardLED,    OUTPUT);
  pinMode(mosfetPin,     OUTPUT);

  /* Setup Display */
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);     // Initialize I2C addr 0x3C (for 128x32)
  display.setRotation(2);                        // Rotate Display: 0, 90, 180 or 270* (0,1,2,3)
  // display.invertDisplay(true);                // White screen logo
  display.display();                             // Display splashscreen
  delay(500);                                    // Time to display splash screen
  display.clearDisplay();                        // Clear buffer

  /* Setup Timer */
  // Add Clock here if you want
  //SD.begin();                                    // Starts SD Card

  /* Read from Memory */
  //chButtonCount = EEPROM.read(epAddress);      // Change to set for heat setting save


  Serial.print("Initializing SD card...");

  if (!SD.begin(4)) {
    Serial.println("initialization failed!");
    return;
  }
  Serial.println("initialization done.");



}

/* ===========================================================================================*/
/*                                                                                            */
/*    END SETUP                                                                                */
/*                                                                                            */
/* ===========================================================================================*/
/*                                                                                            */
/*    BEGIN LOOP                                                                            */
/*                                                                                            */
/* ===========================================================================================*/

void loop()
{
  /* Start PC Communications */
  Serial.begin(9600);

  /* Test Readings */
  // Serial.print("Temp: "); Serial.print(htu.readTemperature());
  // Serial.print("\t\tHum: "); Serial.println(htu.readHumidity());

  /* ===========================================================================================*/
  /*                                                                                            */
  /*    Execute Functions                                                                       */
  /*                                                                                            */
  /* ===========================================================================================*/

  /* Internal */  // Need to add data logging function

  RunSensors();                                 // Runs HTU21D-F Humidity / Temp Sensor
  LogData();                                    // Saves Data to SD Card

  ReadTemp();                                   // Reads Ambient Temp
  Smooth();                                     // Averages Sensor Readings

  /* Buttons */
  ButtonReader();                               // Check button presses
  ResetCount();                                 // Reset button counters
  HoldButton();                                 // De-bounces buttons
  LightsOn();
  ExchangeAir();


  /* Memory */
  //WriteEEPROM();                                // Saves Information Last


  /* ===========================================================================================*/
  /*                                                                                            */
  /*    Display Menus                                                                           */
  /*                                                                                            */
  /* ===========================================================================================*/

  /* Conditionally Display Menus */
  if (sButtonCount <= 1 && hButtonCount == 0) {
    MainMenu();                               // Render screen icons & text
  }
  if (sButtonCount == 2 && hButtonCount == 0) {
    //SecondMenu();
  }
  if (hButtonCount == 1) {
    HumidHistoryMenu();
  }
  if (hButtonCount == 2) {
    TempHistoryMenu();
  }
  if (hButtonCount == 3) {
    display.invertDisplay(true);
    DateTimeMenu();
  }
} /* Close Program */

/* ===========================================================================================*/
/*                                                                                            */
/*    END LOOP                                                                                */
/*                                                                                            */
/* ===========================================================================================*/
/*                                                                                            */
/*    Main Menu Functions   [ OLED ]                                                          */
/*                                                                                            */
/* ===========================================================================================*/


/* Creates Main Interface  */
void MainMenu()
{
  display.clearDisplay();
  display.invertDisplay(false);

  // Humidity  #
  display.setTextColor(WHITE);
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.print(realHumid);
  display.println("%");

  // Humid Text
  display.setTextSize(1);
  display.setCursor(3, 24);
  display.println("HUMID");
  display.drawFastHLine(0, 19, 36, WHITE);

  // Temp  #
  display.setTextColor(WHITE);
  display.setTextSize(2);
  if (realTempF > 100) {
    display.setCursor(46, 0);
  }
  else {
    display.setCursor(52, 0);
  }
  display.print(realTempF);
  display.write(0xf7);
  display.drawFastHLine(46, 19, 45, WHITE);

  // Temp Text
  display.setTextSize(1);
  display.setCursor(56, 24);
  display.println("TEMP");

  // Dry Text
  display.setTextSize(1);
  display.setCursor(103, 25);
  display.println("DRY");
  display.drawFastHLine(100, 19, 23, WHITE);

  /* Adjusts Screen OF/HI/MD/LO (via Button A) */
  display.setTextColor(WHITE);
  display.setTextSize(2);
  display.setCursor(101, 0);

  switch (tButtonCount) {
    case 0:
      display.println("NO");
      break;
    case 1:
      display.println("LO");
      break;
    case 2:
      display.println("MD");
      break;
    case 3:
      display.println("HI");
      break;
  }
  delay(1);
  display.display();
}


/*  Millis Time Menu */
void DateTimeMenu()
{
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(3, 4);
  display.println("MILLIS TIME");
  display.setCursor(3, 12);
  display.setTextSize(2);
  display.println(milliTime);
  display.display();
}

/* High & Low Humidity Menu */
void HumidHistoryMenu()
{

  if (realHumid > humidHigh) {
    humidHigh = realHumid;
  }

  if (realHumid < humidLow) {
    humidLow = realHumid;
  }

  display.setTextColor(WHITE);
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.print(humidLow);
  display.print("% / ");
  display.print(humidHigh);
  display.print("%");

  // Humid Text
  display.setTextSize(1);
  display.setCursor(0, 22);
  display.println("48HR MIN/MAX HUMIDITY");

  // Create Graph
  /* Just Some rough program
      Swap from 24H to 72H using C Button
      B Button Should be LED Lights
     Get data from EEPROM for X
     For X time, do the drawing
     Create single dot on point x,y
     But draw line down from that dot to bottom plane
     Stat with x at static place
     substitute Y with humidity number
     map humidity 1-100 with the screen size 32px
     exclude far top and bottom pixels by 1 from top, 6 from bottom px ea
  */

  /*
    //                                                                                                         *****************BM
    for (int dataPoints = 0; dataPoints < 47; dataPoints++) {

      int startX = 78;

      // Reading Data
      plotFile = SD.open("tempplot.txt");                 // re-open
      int tempRead = Serial.write(plotFile.read());       // Read it
      int plotY = map(tempRead, 1, 200, 1, 24);
      display.drawFastVLine(startX + dataPoints, 0, plotY, 1);
    }

  */

  display.display();
  display.clearDisplay();
  myFile.close();                                 // close the file:
}

/* High & Low Temperature Menu */
void TempHistoryMenu()
{
  if (realTempF > tempHigh) {
    tempHigh = realTempF;
  }

  if (realTempF < tempLow) {
    tempLow = realTempF;
  }

  display.setTextColor(WHITE);
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.print(tempLow);
  display.write(0xf7);                    // degrees
  display.print(" / ");
  display.print(tempHigh);
  display.write(0xf7);

  // Humid Text
  display.setTextSize(1);
  display.setCursor(0, 22);
  display.println("48HR MIN/MAX TEMP");

  display.display();
  display.clearDisplay();

}

/* ===========================================================================================*/
/*                                                                                            */
/*    Buttons - Read & Count & Reset                                                          */
/*                                                                                            */
/* ===========================================================================================*/

/* Turns on Lights via A Button */
void LightsOn()
{
  if (sButtonCount == 2) {
    ledState = true;
  }
  else {
    ledState = false;
  }

  // Turns on Relay
  if (ledState = true) {
    digitalWrite(extLedPin, HIGH);    // Use an empty pin on micro-controller, do not use relay
  }
}

void ExchangeAir()
{
  if (tButtonCount == 2 && hButtonCount >= 1) {
    ventFanStatus = true;
  }
  else {
    ventFanStatus = false;
  }
}

/* Read Temperature Button + Haptic Feedback (Add to All buttons later) */
void ButtonReader()
{
  tButtonState = analogRead(tempButtonA);
  hButtonState = digitalRead(hitsButtonB);
  sButtonState = digitalRead(scrnButtonC);

  if (tButtonState == LOW && sButtonCount <= 2)  {
    tButtonCount++;
  }
  if (hButtonState == LOW && sButtonCount != 3)  {
    hButtonCount++;
  }
  if (sButtonState == LOW)  {
    sButtonCount++;
  }
  delay(1);
}

/* Reset Button Counts */
void ResetCount()
{
  if (tButtonState == LOW && tButtonCount >= 3)  {
    tButtonCount = 0;
  }
  if (hButtonState == LOW && hButtonCount >= 4)  {
    hButtonCount = 0;
    sButtonCount = 0;
  }
  if (sButtonState == LOW && sButtonCount >= 3)  {
    sButtonCount = 0;
  }
}

/* Checks if Buttons are Held */
void HoldButton()
{
  int minHoldTime = 50;

  /* For Hits Button */
  if (hButtonState == LOW) {
    current = digitalRead(hitsButtonB);

    if (current == LOW && previous == HIGH && (millis() - firstTime) > 200) {
      firstTime = millis();
    }

    millis_held = (millis() - firstTime);
    secs_held = millis_held / 1000;

    if (millis_held > minHoldTime) {

      if (current == HIGH && previous == LOW) {

        if (millis_held >= 1000) {
          fButtonCount = 0;
          Serial.println("**************** B WAS HELD FOR TIME ********************");                    // **** BOOKMARK *** //
          // This function needs work above. Not sure why its hard to achieve.
        }
      }
    }
  }

  previous = current;
  prev_secs_held = secs_held;
}


/* ===========================================================================================*/
/*                                                                                            */
/*    Fire Vape Coil & LED's                                                                  */
/*                                                                                            */
/* ===========================================================================================*/

/* Check Fire Button and Turns on Heat */
void FireCoil()
{
  if (fButtonState == LOW)  {
    firePower = map(tButtonCount, 1, 3, 100, 255);
    analogWrite(mosfetPin, firePower);

    if (sButtonCount != 4) {
      display.clearDisplay();
      display.invertDisplay(true);
      display.setTextSize(2);
      display.setTextColor(WHITE);
      display.setCursor(18, 9);
      display.println("FIRE UP!");
      display.display();
    }
  }

  else  {
    analogWrite(mosfetPin, 0);
    display.clearDisplay();
  }
  delay(1);
}

/* ===========================================================================================*/
/*                                                                                            */
/*    Internal Functions                                                                      */
/*                                                                                            */
/* ===========================================================================================*/

/* Log Data to SD Card */
void LogData() {

  milliTime = (millis() / 1000);

  if (milliTime - previousMillis > loginterval) {
    previousMillis = milliTime;

    /* Open & Write to First File, then Close */
    myFile = SD.open("kilndata.txt", FILE_WRITE);     // open

    myFile.print("Time: ");                           // write
    myFile.print(milliTime);
    myFile.println("s");
    myFile.print("Humidity: ");
    myFile.print(realHumid);
    myFile.println(" %");
    myFile.print("Temperature: ");
    myFile.print(realTempF);
    myFile.println(" °");
    myFile.println('\n');

    myFile.close();                                   // close

    /* Do Second File */
    myFileTwo = SD.open("tempplot.txt", FILE_WRITE);  // open

    String dataString = "";

    dataString += String(realTempF);                  // write
    dataString += ",";
    myFileTwo.print(dataString);

    myFileTwo.close();                                // close
  }
}


/* Open & Read File, then Close */
void DisplayData() {

  for (int dataPoints = 0; dataPoints < 47; dataPoints++) {
    /* Reading Data */
    myFile = SD.open("kilndata.txt");                 // re-open
    Serial.write(myFile.read());                      // Read it

  }
  myFile.close();                                 // close the file:
}


/* Smooth Readings */
void Smooth()
{
  for (int i = 0; i < 175; i++)  {
    avgTempRead = avgTempRead + (temperatureF - avgTempRead) / numReadings;
  }

  delay(1);
}

/* Reads Temp Sensor & Shuts Down Functions if Too Hot*/
void ReadTemp()
{
  int reading = analogRead(thermoPin);

  float voltage = reading * calcVolt;                  // converting that reading to voltage
  voltage /= 1024.0;

  //converting from 10 mv per degree wit 500 mV offset to degrees ((voltage - 500mV) times 100)
  float temperatureC = (voltage - 0.5) * 100;

  temperatureF = (temperatureC * 9.0 / 5.0) + 32.0;    // Change to F (Line431)

  avgTempRead = (int) temperatureF;                    // Truncate digits after decimal

  delay(1);

  /* Run Fans */
  if (avgTempRead >= 150) {
    digitalWrite(mosfetPin, 255);
    //delay(3500);
  }
}

/* Writes settings to static ram */
void WriteEEPROM()
{
  // int val = analogRead(0) / 4;            // Divide by 4 to convert to analog [if digital]
  EEPROM.write(epAddress, avgTempRead);    // Write value to the appropriate byte of the EEPROM

  // Advance to the next address, when at the end restart at the beginning *For Compatibility*
  epAddress = epAddress + 1;
  if (epAddress == EEPROM.length()) {
    epAddress = 0;
  }
  delay(1);
}

void RunSensors()
{
  realHumid = htu.readHumidity();
  realTempC = htu.readTemperature();
  realTempF = (realTempC * 9.0 / 5.0) + 32.0;

  if (realHumid >= 100) {
    realHumid = 99;
  }
}

