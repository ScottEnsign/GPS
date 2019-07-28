// Use a Mayfly data logger and GPSbee to log position to SD card.

#include <TinyGPS.h>
#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include "Sodaq_DS3231.h"

#define DATA_HEADER "sampleNumber, dateTime, batteryVolts, loggertempC, sats, HDOP, lat, long, age"
#define SD_SS_PIN 12                  //Digital pin 12 is the MicroSD slave select pin on the Mayfly
#define FILE_NAME "datalog.txt"       //The data log file

int long sampleinterval = 10000;
int samplenum = 1;                        // declare the variable "samplenum" and start with 1
int batteryPin = A6;                      // on the Mayfly board, pin A6 is connected to a resistor divider on the battery input
int batterysenseValue = 0;                // variable to store the value coming from the analogRead function
float batteryvoltage;                     // the battery voltage as calculated by the formula below
const int TIME_ZONE = -4;                 // set the time zone to -4 for EDT or -5 for EST
static void smartdelay(unsigned long ms); //this has something to do with the GPS

String add02d(uint16_t val) {   
  if (val < 10)
    {return "0" + String(val);}
  else
    {return String(val);}
}

TinyGPS gps;

void setup()
{
  Serial.begin(9600);
  rtc.begin();
  setupLogFile();              //Initialise log file
  Serial.println(DATA_HEADER); //Echo the data header to the serial connection
  Serial1.begin(9600);
  pinMode(23,OUTPUT);
  digitalWrite(23,HIGH);
  pinMode(8,OUTPUT);
  digitalWrite(8,HIGH);
}

void loop(){
  String dataRec = createDataRecord();
  logData(dataRec);                     //Save the data record to the log file
  Serial.println(dataRec);              //Echo the data to the serial connection
  smartdelay(1000);
  delay(sampleinterval);
}

void setupLogFile() {
  if (!SD.begin(SD_SS_PIN)) { //Initialise the SD card
    Serial.println("Error: SD card failed to initialise or is missing.");
  }
  bool oldFile = SD.exists(FILE_NAME);           //Check if the file already exists
  File logFile = SD.open(FILE_NAME, FILE_WRITE); //Open the file in write mode
  if (!oldFile)                                  //Add header information if the file did not already exist
  {
    logFile.println(FILE_NAME);
    logFile.println(DATA_HEADER);
  }
  logFile.close(); //Close the file to save it
}

void logData(String rec) {
  File logFile = SD.open(FILE_NAME, FILE_WRITE); //Re-open the file
  logFile.println(rec);                          //Write the CSV data
  logFile.close();                               //Close the file to save it
}

String createDataRecord() {
  String data = "";            //Create a String type data record in csv format
  data += samplenum;           //creates a string called "data", put in the sample number
  data += ",";                 //adds a comma between values
  DateTime now = rtc.makeDateTime(rtc.now().getEpoch()+TIME_ZONE*3600); 
  data += now.year();
  data += "-";
  data += now.month();
  data += "-";
  data += now.date();
  data += " ";
  data += add02d(now.hour());
  data += ":";
  data += add02d(now.minute());
  data += ",";
  batterysenseValue = analogRead(batteryPin);                   // reads the analog voltage on the batteryPin, reported in bits
  batteryvoltage = (3.3/1023) * 4.7 * batterysenseValue;        // converts bits into volts (see batterytest sketch for more info)
  data += batteryvoltage;                                       //adds the battery voltage to the data string
  data += ",";
  rtc.convertTemperature();
  data += rtc.getTemperature();
  data += ",";
  data += gps.satellites();
  data += ",";
  data += gps.hdop();
  data += ",";
  long lat,lon;
  unsigned long fix_age;
  gps.get_position(&lat, &lon, &fix_age);
  data += lat;
  data += ",";
  data += lon;
  data += ",";
  data += fix_age;
    
  samplenum++;   //increment the sample number
  return data;
}

static void smartdelay(unsigned long ms) {
  unsigned long start = millis();
  do   {
    while (Serial1.available())
    gps.encode(Serial1.read());
  } 
  while (millis() - start < ms);
}
