#include <SPI.h>
#include "ds3234.h"
#include <avr/sleep.h>
#include <SdFat.h>
#include <avr/power.h>
#include <SoftwareSerial.h> 
#define rx 7                                         //define what pin rx is going to be
#define tx 8                                         //define what pin tx is going to be

SoftwareSerial myserial(rx, tx);
 
const int CS = 10; //
int ss = 9;
int AlarmPin = 2;
int Pgnd=5;
int Cgnd=3;
int T_power=4;
int C_power=6;
//Create objects

SdFat sd;
SdFile file;


unsigned char wakeup_min;

struct ts t;
uint8_t sleep_period = 2;       // the sleep interval in minutes between 2 consecutive alarms
char newfile[] = "multiProbe091217_2.dat";
float C=0;

String inputstring = "";                              
String sensorstring = "";                             
boolean input_string_complete = false;                
boolean sensor_string_complete = false;               
float DO;

void set_alarm(void)
{
    // calculate the minute when the next alarm will be triggered
    wakeup_min = (t.min / sleep_period + 1) * sleep_period;
    if (wakeup_min > 59) {
        wakeup_min -= 60;
    }

    // flags define what calendar component to be checked against the current time in order
    // to trigger the alarm - see datasheet
    // A1M1 (seconds) (0 to enable, 1 to disable)
    // A1M2 (minutes) (0 to enable, 1 to disable)
    // A1M3 (hour)    (0 to enable, 1 to disable)
    // A1M4 (day)     (0 to enable, 1 to disable)
    // DY/DT          (dayofweek == 1/dayofmonth == 0)
    uint8_t flags[4] = { 0, 1, 1, 1};

    // set Alarm1
    DS3234_set_a2(ss, wakeup_min, 0, 0, flags);

    // activate Alarm1
    DS3234_set_creg(ss, DS3234_INTCN | DS3234_A2IE);
}

void setup() {
  Serial.begin(9600);
  myserial.begin(9600);                              

  SPI.beginTransaction(SPISettings(6000000, MSBFIRST, SPI_MODE3));
  DS3234_init(ss, DS3234_INTCN);
  delay(10);
  DS3234_get(ss, &t);
  delay(10);
  set_alarm();
  DS3234_clear_a2f(ss);
  delay(10);

  pinMode (AlarmPin, INPUT);
  pinMode (Pgnd, OUTPUT);
  pinMode (Cgnd, OUTPUT);
  pinMode (T_power, OUTPUT);
  pinMode (C_power, OUTPUT);
  
  attachInterrupt(0, alarm, FALLING); // setting the alarm interrupt


 while (!sd.begin(CS, SPI_HALF_SPEED)) {

  }

//Serial.println("Card Initialised”); //would print to the Arduino screen if connected

  // open the file for write at end like the Native SD library
  file.open(newfile, O_WRITE | O_CREAT | O_APPEND);
  file.close();

 delay(1000);

  inputstring.reserve(10);                           
  sensorstring.reserve(30);                           
}


void loop() {

  gotoSleep();
  delay(5);

  digitalWrite(Pgnd,HIGH);
  delay(500);
  int P_DN = analogRead(A1);
  digitalWrite(Pgnd,LOW);
  
  digitalWrite(Cgnd,HIGH);
  digitalWrite(C_power,HIGH);
  delay(5);
    int C_DN = analogRead(A3);

  digitalWrite(C_power,LOW);
  digitalWrite(Cgnd,LOW); 
  
  digitalWrite(T_power,HIGH);
  int T_DN = analogRead(A0);
  digitalWrite(T_power,LOW);


  float C_V = C_DN*3.3/1024.0;
  float T_V = T_DN*3.3/1024.0;
  float P_V = P_DN*3.3/1024.0;
  float T_R = 10.0/T_V*(3.3-T_V);
  float T = 1/(2.772539*pow(10,-3) + 2.50767*pow(10,-4)*log(T_R) + 3.37884*pow(10,-7)*pow(log(T_R),3))-273.15;

  float P = (P_V-1.58)*10000.0;

   myserial.print('r');                      //send that string to the Atlas Scientific product
   myserial.print('\r');                             //add a <CR> to the end of the string 
   delay(1800);
   myserial.print('r');                      //send that string to the Atlas Scientific product
   myserial.print('\r');                             //add a <CR> to the end of the string 
    delay(1800);
    
  DO_reading();
  
  delay(50);
  
  SPI.beginTransaction(SPISettings(6000000, MSBFIRST, SPI_MODE3));
  delay(20);
    DS3234_get(ss, &t);
  set_alarm();
  DS3234_clear_a2f(ss);


      String dataString = "";
      dataString += t.year;
      dataString += ",";
      dataString += t.mon;
       dataString += ",";
      dataString += t.mday;
      dataString += ",";
      dataString += t.hour;
       dataString += ",";
      dataString += t.min;
       dataString += ",";
      dataString += C_V;
      dataString += ",";
      dataString += P_V;
      dataString += ",";
      dataString += T;
      dataString += ",";
      dataString += DO;

       file.open(newfile, O_WRITE | O_APPEND); //Opens the file
       delay(5);
       file.println(dataString); //prints data string to the file
       delay(5);
       file.close(); //closes the file
       delay(5);
       Serial.println(dataString);
       delay(100);


  attachInterrupt(0, alarm, FALLING);

}
void gotoSleep(void)
{
   byte adcsra = ADCSRA;          //save the ADC Control and Status Register A
   ADCSRA = 0;  //disable the ADC
   sleep_enable();
   power_spi_disable();
   set_sleep_mode(SLEEP_MODE_PWR_DOWN);
   cli();
   sleep_bod_disable();
   sei();
   sleep_cpu();
   /* wake up here */
   sleep_disable();
   power_spi_enable();
   ADCSRA = adcsra;               //restore ADCSRA
}
void alarm()
{
    detachInterrupt(0);

}

void DO_reading()
{
    myserial.print('R');                      //send that string to the Atlas Scientific product
    myserial.print('\r');                             //add a <CR> to the end of the string 
//   delay(1800);

//  while (!myserial.available()){}

  while (myserial.available()) {                     //if we see that the Atlas Scientific product has sent a character
    char inchar = (char)myserial.read();   
//delay(10);
    sensorstring += inchar;                           //add the char to the var called sensorstring
    if (inchar == '\r') {                             //if the incoming character is a <CR>
      sensor_string_complete = true;                  //set the flag
//      Serial.println(sensorstring);
    }
  }


  if (sensor_string_complete== true) {                //if a string from the Atlas Scientific product has been received in its entirety
    if (isdigit(sensorstring[0]))  DO = sensorstring.toFloat();              //if the first character in the string is a digit
                                                    //convert the string to a floating point number so it can be evaluated by the Arduino

 sensorstring = "";                                //clear the string
    sensor_string_complete = false;                   //reset the flag used to tell if we have received a completed string from the Atlas Scientific product

    myserial.print("Sleep");   
    myserial.print('\r');
    delay(1800); 
  }
}
    
  

