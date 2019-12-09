//This sketch is for the feeding system that delivers liquid food to the automated marine aquatic system.  This is a final revised working version.
//The latest change was to move the stir cycle out of the feeding loop and only have it mix at 5 min before the hour. This turns out to be critical,
//as before there was too much air generated in the food being pumped and this caused pump faliures due to the the check valves staying open and 
//pump failures. Currently the timing of feedings is set to once every two hours for 24 mins. In addition, the time for pumping is set to 30 secs
//which is enough for the pump to complete its cycle.  This may need to be changed for larger volumes. Possible improvements include some way to 
//continue showing the red fault LED once feeding cycle starts or stops (now if will change to blue or green momentarily until the sensor again 
//detects the low food level). Also have added a way to check that food entered the line, and if not, it sends a text, and to send a text if food
//never leaves the line. Also once a low food alert is sent, this triggers the warning LED warning light to turn RED, but if the sensor no longer
//sees a low level the red status light will remain lit until the next feeding cycle begins or ends, or is reset at 55 mins past the hour for a stir
//cylce.

//Uses date and time functions from a DS1307 RTC connected via I2C, with battery backup.The program controls feeding cycles of the automated aquarium
//system for Crepidula snails (controls the aquarium bypass valve, stir plate, syringe pump and air pump). Stir plate and aquarium pump are controlled
//via relays for 120 volts AC in the remote outlet box. The New Era NE-1010 syringe pump has faster speeds compared to the NE-1000 and can deliver
//higher pressures when using gas tight glass syringes. The controller also monitors food levels in the reservoir using an NPN capacitance sensor, and 
//sends a text message if there is a problem (e.g., food supply is Low). The script also incorporates alarm test to avoid rapidly resending low food text
//alerts. The alarm status is reset only at 55 mons past the hour so texts should only be repeated once an hour. System, also monitors food passage through
//the feeding tube using a 38KHz pulsed IR LED and sensor. The Ethernet sheild uses pins 10 (used to deselect W5100 to disable it, set as output and set HIGH,
//but we are using it so no need to do that here), 11, 12, 13, as well as pin 4, which is used two deselect SD card, set as output and set to HIGH, which is
//needed here, as we are not using the SD card). (Thus, we have Ethernet W5100 enabled and disable the SD card). If using W5200 or later, one would need a 
//different, updated ethernet library. Below we also use pins 2, 3, 5, 6, 7, 8, 9. Analog pins A4 and A5 are used for the RTC for the I2C SDA and SCL pins
//Analog pins A0, A1, and A2 are used to control the power switch's three LEDs (Green, Blue, Red) to show system status alerts. Also incorporates alarm test
//to avoid resending low food text alerts. Define them as OUTPUT pins, setting them to ground each LED to light them To write to the LCD display we currently
//use the TX pin 1, but this creates problems if one uploads code while display is connected. This can corrupt the display's eprom settings requiring that it 
//be reset using Afrtuit's LCD script. It would be better to use Software Serial and redefine the TX pin (to analog pin A3, as it is the only other available,
//free pin). First created by J. Henry on July 30, 2018, continuously updated until Dec. 9, 2019. Thgis porject funded by and NSF EDGE grant (no. 1827533).
//to J.J. Henry.

#include "Arduino.h"
#include <SPI.h>
#include <RTClib.h>//Realtime clock library
//#include <EthernetV2_0.h>//needed to run the ethernet shield, V2 for W5200 chip, need to set that up inside the library
#include <Ethernet.h>//needed to run the ethernet shield, needed for W5100 chip
#include <Wire.h>// also included here as the RTClib depends on it
#include <LiquidCrystal.h>//needed for LCD display
//#include <SoftwareSerial.h>//needed to redefine the rx and tx serial pins to run the lcd display

#include <IRremote.h>

#define PIN_IR 3//38khz pulsed output pin for IR LED
#define PIN_DETECT 7//detection pin for IR sensor
//#define lcd Serial1
//#if defined(ARDUINO_ARCH_SAMD) || defined(__SAM3X8E__)
// use pin 18 with Due, pin 1 with Zero or M0 Pro
//#define lcd Serial1
//#else
//#include <SoftwareSerial.h>
// Create a software serial port!
//SoftwareSerial lcd = SoftwareSerial(0,A3);//RX, TX use this to create a new tx pin
//#endif

RTC_DS1307 rtc;
//IR sensor is on the tank feeding line and is attached to Pin 3
//note NPN will source a negative, whle PNP will source a positive signal. So if using an NPN sensor
//the sensor it will go LOW when it senses liquid, so use pull up resistor on these pins
#define supplyPin 2//capacitance liquid sensor output monitors the food supply line and is attached to Pin 2

int checksum = 1;//begin as a non-feeding cycle = 1
int alarm = 0;
int count = 0;//part of air pump alert

//ethernet setup
byte mac[] = { XX, XX, XX, XX, XX, XX };// user definable, unique mac address for this device need to change this!!
// change following network settings to yours
IPAddress ip( ##, ##, ##, ## );       // unique ip address for this device need to change this!!
IPAddress gateway( ##, ##, ##, ## );    // change to internet access via router, not always needed, unique address, need to change this!!
IPAddress subnet( ##, ##, ##, ## );    // change to subnet mask, not always needed, unique address, need to change this!!
IPAddress myserver( ##, ##, ##, ## );   // for email alarm, unique address device need to change this!!
//EthernetServer server(8080);           // server port : remember to set port forwarding on the DSL router, if needed, we dont need this for this sketch
EthernetClient client;

IRsend irsend;
void setup()
{

  pinMode(PIN_DETECT, INPUT);// make this pin an input pin
  irsend.enableIROut(38);//pulses IR LED at 38Khz, for selectivity IR sensor can only detect IR light at 38K hz
  irsend.mark(0);//

  pinMode(A0, OUTPUT);//redefine analog pin as output to control system status LED, nominal, green
  digitalWrite(A0, LOW);//set the pin to LOW, negative TO TURN ON THE GREEN LED
  pinMode(A1, OUTPUT);//redefine analog pin as output to control status LERD, feeding, blue
  digitalWrite(A1, HIGH);//set the pin to HIGH, positive, TO TURTN OFF THE BLUE LED
  pinMode(A2, OUTPUT);//redefine analog pin as output to control system status LED, alert, red
  digitalWrite(A2, HIGH);//set the pin to HIGH, positive, TO TURN OFF THE RED LED
  pinMode(A3, OUTPUT);//redefine analog pin as output to control system status LEDs as a positive +5VDC source
  digitalWrite(A3, HIGH);//set the pin to HIGH, positive, +5VDC source
  //note that analogs pins A4 and A5 are used for the I2C SDA and SCL pins, respectively and no need to define these here
  pinMode(2, INPUT);//from capacitance sensor
  digitalWrite (2, LOW);// For OPNO sensor; Reverse if using NPN sensor (HIGH) to activate pullup resistor.
  pinMode(5, OUTPUT);//to relay board, relay coil is not energized when input is HIGH
  digitalWrite(5, HIGH);
  pinMode(6, OUTPUT);//to relay board, relay coil is not energized when input is HIGH
  digitalWrite(6, HIGH);
  pinMode(8, OUTPUT);//ttl output for aquarium system bypass valve
  digitalWrite(8, LOW);
  pinMode(9, OUTPUT);//ttl output for syringe pump
  digitalWrite(9, LOW);
  //while (!Serial); // for Leonardo/Micro/Zero

  Serial.begin(9600); //baud rate for communications
  pinMode(4, OUTPUT); // needed for the ethernet sheild to inactivate the SD card
  digitalWrite(4, HIGH);//inactivate the SD card
  Ethernet.begin(mac, ip, gateway, gateway, subnet);
  delay(2000);


  if (! rtc.begin()) {
    Serial.println("Can't find RTC");
    while (1);


  }

  if (! rtc.isrunning()) {
    Serial.println("RTC not running");
    delay(1000);
    // the following last line sets the RTC to the date & time this sketch was computer's time, only needed once or for a DST change
    // This next line sets the RTC with an explicit date & time, for example to set January 21, 2014 at 3am you would call:
    //rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
    //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));//This phrase uses computer time to set the RTC. But note a bug can't get time to reset on my new Mac!?

  }

}

void loop () {//the main loop

  //Serial.write(0xFE);//turn off lcd display backlight
  //Serial.write(0x46);
  //delay(10);
  //not sure we need this next bit so will inactivate it, believe this is in eprom, non-volatile memory
  //Serial.write(0xFE);//turn autoscroll on, new text will appear on first line only
  //Serial.write(0x51);
  //delay(10);

  DateTime now = rtc.now();//queries the RTC
  delay(1000);
  //now let's print out the current set date and time
  //Serial.print(now.year(), DEC);
  //Serial.print('/');
  //Serial.print(now.month(), DEC);
  //Serial.print('/');
  //Serial.print(now.day(), DEC);
  //Serial.print(' ');
  Serial.print("Time");
  Serial.print(' ');
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println();
  delay(1500);

  //lcd.write(0xFE);
  //lcd.write(0x58);
  //delay(10);
  {

//checks food reservoir every cycle for low food level
    if (digitalRead(supplyPin) == 1) { //check to see if there is food in reservoir, PNP capacitance sensor. Reverse if using NPN sensor (0)
      Serial.println("Reservoir FULL");
      delay(500);
    }
    else if (alarm == 0){//checks to make sure an email has not already been sent
      digitalWrite(A0, HIGH);//turn off other status lights
      digitalWrite(A1, HIGH);//turn off oither status lights
      digitalWrite(A2, LOW);//Send LOW ttl signal to aquarium system status LED to red alert
 
      Serial.println("Reservoir LOW");
      delay(500);
      alarm = 1;//set alarm to 1 so that emails will not be sent repeatedly
      Serial.println("Refill Food");
      
      //Serial.write(0xFE);//turn on lcd display backlight
      //Serial.write(0x42);
      //delay(10);

      delay(500);
      if (sendEmail()) {
        Serial.println("Email sent");
        delay(500);
      } else {
        Serial.println("Email failed");
        alarm = 0;// reset alarm to 0 so that program will try to resend a sucessful email (text)
      }
      delay (1000);//wait 1 sec
    }
    else{
      Serial.println("Reservoir LOW");
      delay(500);
      }
  }
  {
     //below triggers a feeding cycle every 2 hours or 12 times each day
      if ((now.hour() == 1 || now.hour() == 3 || now.hour() == 5 || now.hour() == 7 || now.hour() == 9 || now.hour() == 11 || now.hour() == 13 || now.hour() == 15 || now.hour() == 17 || now.hour() == 19 || now.hour() == 21 || now.hour() == 23) && (now.minute() == 00)) { //activate a feeding cycle every 2 hours. btw || is OR
     //below triggers a feeding cycle every 3 hours or 8 times each day
      //if ((now.hour() == 1 || now.hour() == 4 || now.hour() == 7 || now.hour() == 10 || now.hour() == 13 || now.hour() == 16 || now.hour() == 19 || now.hour() == 22) && (now.minute() == 00)) { //activate a feeding cycle every 3 hours. btw || is OR
     //below triggers a feeding cycle every 4 hours or 6 times each day
      //if ((now.hour() == 1 || now.hour() == 5 || now.hour() == 9 || now.hour() == 13 || now.hour() == 17 || now.hour() == 21) && (now.minute() == 00)) { //activate a feeding cycle every 3 hours. btw || is OR

      alarm = 0;//to reset alarm once food is refilled
   
      digitalWrite(A0, HIGH);//turn off other status lights
      digitalWrite(A2, HIGH);//turn off other status lights
      digitalWrite(A1, LOW);//set LOW to light the FEEDING BLUE LED
      
      Serial.println("Feeding Cycle");
      delay(200);
      checksum = 2;// set this value, which is used to tell other parts of the program it is a feeding cycle

      digitalWrite(8, HIGH);//Send high ttl signal to aquarium system to activate drain shunt to the return sump and bypass the filter circuit
      Serial.println("Bypass Filters");
      delay(3000);//wait three seconds for this to occur and to see text on LCD, may need to be longer
      
      //commented out the stirring below, as I changed the regular stir cycle time to occur just before each hour.
      //digitalWrite(5, LOW);//Send signal to relay board to activate the stir plate prior to feeding to resuspend food
      //Serial.println("Stirring ON");
      //delay(20000);//run stirplate for 20 sec, this may need to be longer to get stirplate up and running

      //digitalWrite(5, HIGH);//End signal to relay to stop the stir plate prior to dispensing food
      //Serial.println("Stirring OFF");
      //delay (500);

      digitalWrite(9, HIGH);//Send 200msec ttl pulsed signal to syringe pump to begin preprogrammed feeding cycle program
      //Serial.println("Syringe ON");
      delay(200);//wait, shoud be at least 200msec nomonally, /pump set to trigger on falling edge
      digitalWrite(9, LOW);//complete the ttl signal pulse to syringe pump
      Serial.println("Syringe ON");
      delay(500);
      delay(30000);//wait 30 seconds for feeding cycle to be acomplished, adjust length, as needed for syringe pump program (depends on pump volume and speed/rate)
      Serial.println("Syringe OFF");
      //delay(500);
        //**add additional code here to see if any food entered the line, if there is none, send a text, for feeding failure

//here we add code to send email if there is no food loaded into the feeding line
{//added this to avoid erroneous email
if (digitalRead(PIN_DETECT) == 1) { //should be set to 1 to see if there is food in the feeding line, 0 for testing without emails
 Serial.println("Food loaded");
 delay(500);
}
  else{//there is no food in the feeding line;
    Serial.println("No food loaded");  

      digitalWrite(A0, HIGH);//turn off other status lights
      digitalWrite(A1, HIGH);//turn off other status lights
      digitalWrite(A2, LOW);//Send LOW ttl signal to aquarium system status LED to red alert
      
delay(500);
      if (sendEmail2()) {
        Serial.println("Email sent");
        delay(500);
      } else {
        Serial.println("Email failed");
      }
  }
}
      //count = 1; part of air pump alert to set counter to first run of 30 secs duration
      digitalWrite(6, LOW);//Signal to aquarium air pump = Turn on the air pump to flush the line
      Serial.println("Air ON");
      delay(30000);//run air pump for thirty seconds before checking the line for residual food, formerly this was set to six seconds
      //but it was not enough for the rather viscous food and volumes used.
      while (digitalRead(PIN_DETECT) == 1) { //see if there is still food in the feeding line
        //while (senseValue == HIGH);//Once line is empty, shut off the air pump relay
        digitalWrite(6, LOW);//Signal to aquarium air pump = Continue to turn on the air pump as long as food may be present
        Serial.println("Air still ON");
        //count = (count +1);//part of air pump alert, increment the counter
        delay(200);//could lengthen this, especially if one activates this air pump alert code below, if not
        //the program could be stuck in a permanant while loop when the airpump fails or the air line comes loose
        //if (count == 4){      // send email alert and bail out after 3 more tries of duration set with above delay, part of air pump alert
        //count = 0;//part of air pump alert, resets the counter
        //if (sendEmail3()) {//part of air pump alert, code is getting pretty big so could simply use sendEmail2() subroutine instead
        //Serial.println("Email sent");//part of air pump alert
        //delay(500);//part of air pump alert
      //} else {//part of air pump alert
       // Serial.println("Email failed");//part of air pump alert
      //}//part of air pump alert
       //break;//part of air pump alert
        //}  //part of air pump alert
        //above adds additional code to time the air purge step, if it takes too long and food is not cleared send a text for air pump failure
        //adds a counter to give it three more tries and then gives up and sends the email alert that the air pump failed
        //
      }
      //else {//dont need this line at all
      digitalWrite(6, HIGH);//Signal to aquarium air pump = Turn off the air
      Serial.println("Air OFF");
      delay(1000);// a little extra delay to wait long enough so as to not trigger another feeding cycle, as now minute will no longer be 0

    }
      
      if ((now.hour() == 1 || now.hour() == 3 || now.hour() == 5 || now.hour() == 7 || now.hour() == 9 || now.hour() == 11 || now.hour() == 13 || now.hour() == 15 || now.hour() == 17 || now.hour() == 19 || now.hour() == 21 || now.hour() == 23) && (now.minute() == 24)) { //activate a non-feeding cycle 23 mins after feeding begins. btw || is OR
    //if ((now.hour() == 2 || now.hour() == 5 || now.hour() == 8 || now.hour() == 11 || now.hour() == 14 || now.hour() == 17 || now.hour() == 20 || now.hour() == 23) && (now.minute() == 00)) { //activate a non-feeding cycle one hour after feeding begins, btw || is OR
    //if ((now.hour() == 1 || now.hour() == 3 || now.hour() == 5 || now.hour() == 7 || now.hour() == 9 || now.hour() == 11 || now.hour() == 13 || now.hour() == 15 || now.hour() == 17 || now.hour() == 19 || now.hour() == 21 || now.hour() == 23) && (now.minute() == 45)) { //activate a non-feeding cycle 45 mins after feeding begins, btw || is OR
    //if ((now.hour() == 1 || now.hour() == 5 || now.hour() == 9 || now.hour() == 13 || now.hour() == 17 || now.hour() == 21) && (now.minute() == 45)) { //activate a non-feeding cycle 45 mins after feeding begins, btw || is OR

      delay(200);
      digitalWrite(A1, HIGH);//turn off other status lights
      digitalWrite(A2, HIGH);//turn off other status lights
      digitalWrite(A0, LOW);//Send LOW ttl signal to aquarium system status LED to NON-FEEDING GREEN alert
      
      checksum = 1;// value used to tell other parts of the program it is a non-feeding cycle
      digitalWrite(8, LOW);//Send signal to aquarium system to deactivate bypass valve and reactivate the filter circuit
      Serial.println("Regular cycle");
      delay(60000);//wait long enough so as not to retrigger another non-feeding cylce
      //}//took this out to make it compile with extra stir cycle below

    }
  }//start of mixing cycle

  if ((now.minute() == 55)) { //activate a stir cycle once at 55 min past the hour to resuspend the pre-diluted food in the reservoir.
    //This was changed so that the food will be resuspended just before a feeding cycle, and this allows a 5 min time delay for any air bubbles 
    //to rise out of solution and not be passed to the syringe pump. Air was causing failures of the pump valves, when food was mixed the instant before
    //being pumped.

    alarm = 0;//to reset alarm on each 55 min past the hour
    delay(200);
    digitalWrite(5, LOW);//Send signal to start stir plate
    Serial.println("Stir cycle");
    delay(20000);//stir for 20 secs
    digitalWrite(5, HIGH);//Send signal to stop stir plate
    delay(41000);//wait long enough so as not to retrigger another stir cycle

  }//end of mixing cycle

    if (checksum == 2) {
      Serial.println("Feeding Cycle");
      delay (1000);
    }
    else if (checksum == 1) {
      Serial.println("Regular Cycle");
      delay (1000);
    }
  
  
  Serial.println("End of Loop");

}//end of the loop

//subroutine to send Email or Text message
byte sendEmail()
{
  byte thisByte = 0;
  byte respCode;


  if (client.connect(myserver, 25)) {
    Serial.println("connected");
  } else {
    Serial.println("connection fail");
    return 0;
  } 

  if (!eRcv()) return 0;
  Serial.println("Sending helo");

  // change to your public ip
  client.println("helo 1.2.3.4");

  if (!eRcv()) return 0;
  Serial.println("Sending From");

  // change to your email address (sender)// need to fix this
   client.println("MAIL From: <#######.com>");//add your own information and phone number to send the text message, check wioth your wireless phone service provider 

  if (!eRcv()) return 0;

  // change to recipient address
  Serial.println("Sending To");
    client.println("RCPT To: <#######.com>");//add your own information and phone number to send the text message, check wioth your wireless phone service provider 

  if (!eRcv()) return 0;

  Serial.println("Sending DATA");
  client.println("DATA");

  if (!eRcv()) return 0;

  Serial.println("Sending email");

  // change to recipient address
   client.println("To: You <#######.com>");//add your own information and phone number to send the text message, check wioth your wireless phone service provider 

  // change to your address
  client.println("From: Me <#######.com>");//add your own information and phone number to send the text message, check wioth your wireless phone service provider 


  client.println("Subject: Snail Aquarium");

  client.println("Food is Low!");

  
  client.println(".");

  if (!eRcv()) return 0;

  Serial.println("Sending QUIT");
  client.println("QUIT");

  if (!eRcv()) return 0;

  client.stop();

  Serial.println("disconnected");

  return 1;
}

//subroutine to send Email or Text message
byte sendEmail2()
{
  byte thisByte = 0;
  byte respCode;


  if (client.connect(myserver, 25)) {
    Serial.println("connected");
  } else {
    Serial.println("connection fail");
    return 0;
  }

  if (!eRcv()) return 0;
  Serial.println("Sending helo");

  // change to your public ip
  client.println("helo 1.2.3.4");

  if (!eRcv()) return 0;
  Serial.println("Sending From");

  // change to your email address (sender)// need to fix this
   client.println("MAIL From: <#######.com>"); //add your own information and phone number to send the text message, check wioth your wireless phone service provider 

  if (!eRcv()) return 0;

  // change to recipient address
  Serial.println("Sending To");
    client.println("RCPT To: <#######.com>"); //add your own information and phone number to send the text message, check wioth your wireless phone service provider

  if (!eRcv()) return 0;

  Serial.println("Sending DATA");
  client.println("DATA");

  if (!eRcv()) return 0;

  Serial.println("Sending email");

  // change to recipient address
   client.println("To: You <#######.com>"); //add your own information and phone number to send the text message, check wioth your wireless phone service provider


  // change to your address
  client.println("From: Me <#######.com>"); //add your own information and phone number to send the text message, check wioth your wireless phone service provider


  client.println("Subject: Snail Aquarium");

  client.println("Feeding Pump Failure!");

  
  client.println(".");

  if (!eRcv()) return 0;

  Serial.println("Sending QUIT");
  client.println("QUIT");

  if (!eRcv()) return 0;

  client.stop();

  Serial.println("disconnected");

  return 1;
}

byte eRcv()
{
  byte respCode;
  byte thisByte;

  //EthernetClient client;

  while (!client.available()) delay(1);

  respCode = client.peek();

  while (client.available())
  {
    thisByte = client.read();
    Serial.write(thisByte);
  }

  if (respCode >= '4')
  {
    efail();
    return 0;
  }

  return 1;
}


void efail()
{
  byte thisByte = 0;

  // EthernetClient client;

  client.println("QUIT");

  while (!client.available()) delay(1);

  while (client.available())
  {
    thisByte = client.read();
    Serial.write(thisByte);
  }

  client.stop();

  Serial.println("disconnected");

}

