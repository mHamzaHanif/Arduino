#include <Arduino.h>

#include <LiquidCrystal_I2C.h>

#include <SoftwareSerial.h>
#include <TinyGPS.h>

#include <SD.h>
#include <SPI.h>


LiquidCrystal_I2C lcd(0x27, 16, 2);
SoftwareSerial mySerial(3, 4); // RX, TX
TinyGPS gps;


void gpsdump(TinyGPS &gps);
void printFloat(double f, int digits = 2);


int CS_PIN = 10;
File file;

void setup() {
  Serial.begin(9600);
  mySerial.begin(9600);

  lcd.begin();
  lcd.backlight();
  lcd.clear();

  delay(1000);
  Serial.print("Testing TinyGPS library v. "); Serial.println(TinyGPS::library_version());
  Serial.println("by Muhammad Hamza Hanif");
  Serial.println();

  initializeSD();
  openFile("test.txt");
  writeToFile("=====================================");
  // closeFile();
}


void loop() { // run over and over
  bool newdata = false;
  unsigned long start = millis();
  
  // Every min we print an update
  while (millis() - start < 10000*6) {
    if (mySerial.available()) {
      char c = mySerial.read();
      writeToFile(c);
      Serial.print(c);  // uncomment to see raw GPS data
      if (gps.encode(c)) {
        newdata = true;
        break;  // uncomment to print new data immediately!
      }
    }
  }

  if (newdata) {
    Serial.println("Acquired Data");
    Serial.println("-------------");
    float GPS = gpsdump(gps);
    GPS;
    Serial.println("-------------");
    Serial.println();
  }

  else if(start == 1000*60){
    closeFile();
  }
//   float flat, flon;
//   printFloat(flat, 5); Serial.print(", "); printFloat(flon, 5);
}

//////////////////////////////////////////////////////////////////
/////////////////// *** External Functions ** ////////////////////
//////////////////////////////////////////////////////////////////

void gpsdump(TinyGPS &gps){
  long lat, lon;
  float flat, flon;
  unsigned long age, date, time, chars;
  int year;
  byte month, day, hour, minute, second, hundredths;
  unsigned short sentences, failed;

  gps.get_position(&lat, &lon, &age);
  Serial.print("Lat/Long(10^-5 deg): "); Serial.print(lat); Serial.print(", "); Serial.print(lon);
  Serial.print(" Fix age: "); Serial.print(age); Serial.println("ms.");

  // On Arduino, GPS characters may be lost during lengthy Serial.print()
  // On Teensy, Serial prints to USB, which has large output buffering and
  //   runs very fast, so it's not necessary to worry about missing 4800
  //   baud GPS characters.

  gps.f_get_position(&flat, &flon, &age);
  Serial.print("Lat/Long(float): "); printFloat(flat, 5); Serial.print(", "); printFloat(flon, 5);
  Serial.print(" Fix age: "); Serial.print(age); Serial.println("ms.");

  lcd.setCursor(0, 0);
  lcd.print("Lat ");
  lcd.print(flat, 5); //flat
  lcd.setCursor(0, 1);
  lcd.print("Long ");
  lcd.print(flon, 5);

  char MAX[20]; 
  ftoa(flat, result, 6); 

  writeToFile("Lat: "); writeToFile(floatToString(result)); writeToFile("\n");
  
  char MAX[20]; 
  ftoa(flon, result, 6); 
  writeToFile("Lon: "); writeToFile(floatToString(result)); writeToFile("\n");

  gps.get_datetime(&date, &time, &age);
  Serial.print("Date(ddmmyy): "); Serial.print(date); Serial.print(" Time(hhmmsscc): ");
  Serial.print(time);
  Serial.print(" Fix age: "); Serial.print(age); Serial.println("ms.");

  gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, &hundredths, &age);
  Serial.print("Date: "); Serial.print(static_cast<int>(month)); Serial.print("/");
  Serial.print(static_cast<int>(day)); Serial.print("/"); Serial.print(year);
  Serial.print("  Time: "); Serial.print(static_cast<int>(hour)); Serial.print(":");
  Serial.print(static_cast<int>(minute)); Serial.print(":"); Serial.print(static_cast<int>(second));
  Serial.print("."); Serial.print(static_cast<int>(hundredths));
  Serial.print("  Fix age: ");  Serial.print(age); Serial.println("ms.");
  /*
    lcd.setCursor(0, 0);
    lcd.print("Date: ");
    lcd.print(static_cast<int>(day));
    lcd.print("/");
    lcd.print(static_cast<int>(month));
    lcd.print("/");
    lcd.print(year);
    lcd.setCursor(0, 1);
    lcd.print("Time: ");
    lcd.print(static_cast<int>(hour));
    lcd.print(":");
    lcd.print(static_cast<int>(minute));
    lcd.print(":");
    lcd.print(static_cast<int>(second));
  */
  Serial.print("Alt(cm): "); Serial.print(gps.altitude()); Serial.print(" Course(10^-2 deg): ");
  Serial.print(gps.course()); Serial.print(" Speed(10^-2 knots): "); Serial.println(gps.speed());
  Serial.print("Alt(float): "); printFloat(gps.f_altitude()); Serial.print(" Course(float): ");
  printFloat(gps.f_course()); Serial.println();
  Serial.print("Speed(knots): "); printFloat(gps.f_speed_knots()); Serial.print(" (mph): ");
  printFloat(gps.f_speed_mph());
  Serial.print(" (mps): "); printFloat(gps.f_speed_mps()); Serial.print(" (kmph): ");
  printFloat(gps.f_speed_kmph()); Serial.println();

  gps.stats(&chars, &sentences, &failed);
  Serial.print("Stats: characters: "); Serial.print(chars); Serial.print(" sentences: ");
  Serial.print(sentences); Serial.print(" failed checksum: "); Serial.println(failed);
}

void printFloat(double number, int digits){
  // Handle negative numbers
  if (number < 0.0) {
    Serial.print('-');
    number = -number;
  }

  // Round correctly so that print(1.999, 2) prints as "2.00"
  double rounding = 0.5;
  for (uint8_t i = 0; i < digits; ++i)
    rounding /= 10.0;

  number += rounding;

  // Extract the integer part of the number and print it
  unsigned long int_part = (unsigned long)number;
  double remainder = number - (double)int_part;
  Serial.print(int_part);

  // Print the decimal point, but only if there are digits beyond
  if (digits > 0)
    Serial.print(".");

  // Extract digits from the remainder one at a time
  while (digits-- > 0) {
    remainder *= 10.0;
    int toPrint = int(remainder);
    Serial.print(toPrint);
    remainder -= toPrint;
  }
}

void initializeSD(){
  Serial.println("Initializing SD card...");
  pinMode(CS_PIN, OUTPUT);

  if (SD.begin())
  {
    Serial.println("SD card is ready to use.");
  } else
  {
    Serial.println("SD card initialization failed");
    return;
  }
}


int createFile(char filename[]){
  file = SD.open(filename, FILE_WRITE);

  if (file){
    Serial.println("File created successfully.");
    return 1;
  } 
  else{
    Serial.println("Error while creating file.");
    return 0;
  }
}

int writeToFile(char text[]){
  if (file){
    file.println(text);
    Serial.println("Writing to file: ");
    Serial.println(text);
    return 1;
  } 
  else{
    Serial.println("Couldn't write to file");
    return 0;
  }
}

void closeFile(){
  if (file){
    file.close();
    Serial.println("File closed");
  }
}

int openFile(char filename[]){
  file = SD.open(filename);
  if (file){
    Serial.println("File opened with success!");
    return 1;
  } 
  else{
    Serial.println("Error opening file...");
    return 0;
  }
}



void ftoa(float n, char* res, int afterpoint) 
{ 
    // Extract integer part 
    int ipart = (int)n; 
  
    // Extract floating part 
    float fpart = n - (float)ipart; 
  
    // convert integer part to string 
    int i = intToStr(ipart, res, 0); 
  
    // check for display option after point 
    if (afterpoint != 0) { 
        res[i] = '.'; // add dot 
  
        // Get the value of fraction part upto given no. 
        // of points after dot. The third parameter  
        // is needed to handle cases like 233.007 
        fpart = fpart * pow(10, afterpoint); 
  
        intToStr((int)fpart, res + i + 1, afterpoint); 
    } 
} 
  