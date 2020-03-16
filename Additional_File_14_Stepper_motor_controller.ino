//Peristaltic Pump Controller. This sketch works nicely.
//This fully working code uses a joystick to increment or decrement 2 values to be stored in eeprom memory
//This sketch has been combined with the peristalitic pump program.
//THius sketch no longer incorporates an interrupt on digital pin 2 (Interrupt 0) to recieve the signal from the feeding 
//station microcontroller to activate a feeding via the peristalitic pump. Note that the interrupt pin 
//input is suseptable to static electricity or RF and may be triggered as such. Better sheilding may be required.
//Some pull-down resistor values needed to be changed.
//To work without interrupts I lengthened the time of the HIGH trigger pulse from 200msec to 400msec in the feeding controller 
//sketch, to be sure this loop here catches it. One could add a forward, primming/purge momentary button. This could be 
//simply donw by supplying Vcc +5VDC to the input Pin 2. If you hold the button down (HIGH) it should continue to pump. 
//For now one has to manually use a wire jumper between the input pin 2 (also pin 2 on the DB9 connector) and Vcc +5VDC, which is
//being carried by pin 1 of the DB9 connector. Note that ground is carried on pin 9 of the DB9 connector.
//Sketch by Dr. J.J. Henry, UIUC, 12/3/2019.

#include <EEPROM.h>
#include <Stepper.h>

//int counter1 = 0;
byte setting1 = 0;//a variable which holds the speed value we set, is a byte for values up to 255
byte setting2 = 0;//a variable which holds the steps value we set, is a byte for values up to 255
//const int ledPin =  13;//the number of the LED pin as a diagnostic readout for the feeding cycle and used to put DRV8833 to sleep
//volatile int buttonState = 0;//variable for reading the pushbutton status
//const byte interruptPin = 2;//input from feeding station to trigger a feeding, used as an interrupt pin

const int  stepsPerRevolution = 200;//change this to fit the number of steps per complete revolution for your stepper motor
//currently we use 1.8 degrees per step

// initialize the stepper library for pins 8 through 11:
Stepper myStepper(stepsPerRevolution, 8, 9, 10, 11);

void setup() {
pinMode(7, INPUT_PULLUP); //input for storing stepper motor values permanently to eeprom memories = "sel" pin
//on adafruit joystick the push button is normally closed (1) and shorts to ground (0) when pressed.
pinMode(13, OUTPUT);//initialize the LED on pin 13 as an output:
digitalWrite(13, LOW);
pinMode(2, INPUT);//initialize the pin as an input
//digitalWrite(2, LOW);//start with this pin LOW to turn off pull up resistor
//attachInterrupt(digitalPinToInterrupt(interruptPin), pin_ISR, RISING);//Attach an interrupt to the ISR vector, and one cannot use "HIGH" on uno
setting1 = EEPROM.read(1);//recall stored value for speed (rpms)
setting2 = EEPROM.read(2);//recall stored value for steps, to deliver a specific volume, which needs to be calibrated in advance
//here one complete revolution is 180 ul with 4mm OD and 2mm ID silicone tubing

Serial.begin(9600); //baud rate for communications
delay(50);
//present the current settings on the LCD:
Serial.print (setting1);
  Serial.print ("RPM / ");//speed
  delay(400);
Serial.print (setting2*50);//multiply Byte value times 50 as each step is only 0.9ul
//this means that currently with a max byte value of 255 we can only deliver 12750 steps or 63.75 revolutions or 11.48ml
//one byte adds 0.45ul
//Note that we could add a calculation to figure out the actual volume instead and print this volume
  Serial.println ("ST");//"steps"
  delay(400);
}
void loop() {
 
  digitalWrite(13, LOW);//illuminate the LED when the interupt pin is HIGH, each time you run through the loop drive this pin LOW
  //to put the DRV8833 to sleep
 delay(50);
 
 if (digitalRead (2) == HIGH){//check to see if there is an input from the feeding controller to trigger the pump
 //Serial.println(buttonState);
 digitalWrite(13, HIGH);//shows that the pump is active and is needed to provide power to slp pin on the DVR8833 stepper motor Driver board
//NOTE! use this pin 13 to also activate the stepper motor driver board (DVR8833, connect it to the "slp" pin, which must be high to provide power to the motor)
 //In this way the motor will not get hot when it is idle. By default the slp pin is pulled low = inactive, and see code above

  //speed is set as rpm
setting1 = EEPROM.read(1);//recall stored value for speed (rpm)
  //or 1/42 revolution minimum
setting2 = EEPROM.read(2);//recall stored value for steps, to deliver a specific volume, which needs to be calibrated in advance
 Serial.println ("Feeding");
 myStepper.setSpeed(setting1);//set equal to rpms in memory, as a byte
    delay(200);
 myStepper.step(setting2*50);//deliver the food set to steps in memory, as a Byte value times 50
 //note that the pump will complete its pumping cycle before returning back here to the main loop
    delay(20);
    Serial.println ("Done");
    digitalWrite(13, LOW);//put the DRV8833 back to sleep

delay(1500);//wait long enough so as not to trigger another pumping, one may not really need this long delay, as pumping is long enough

//buttonState = 0;//reset the button state for the next feeding

//present the current settings on the LCD:
Serial.print (setting1);
  Serial.print ("RPM / ");
  //delay(400);
Serial.print (setting2*50);//multiply Byte value times 50
  Serial.println ("ST");
  delay(500);

 }
 
  
int sensorReading1 = analogRead(A0);
int sensorReading2 = analogRead(A1);

if (sensorReading1 <400){

  setting1 = (setting1 + 1);//increment the value
  Serial.print (setting1);
  Serial.println (" RPMs");
  delay(50);
}

if (sensorReading1 >600){ 

  setting1 = (setting1 - 1);//decrement the value
  Serial.print (setting1);
  Serial.println (" RPMs");
  delay(50);
}

if (sensorReading2 <400){ 

  setting2 = (setting2 + 1);//increment the value
  Serial.print (setting2*50);//multiply Byte value times 50
  Serial.println (" Steps");
  delay(50);
}

if (sensorReading2 >600){ 

  setting2 = (setting2 - 1);//decrement the value
  Serial.print (setting2*50);//multiply Byte value times 50
  Serial.println (" Steps");
  delay(50);
}
if ((digitalRead (7) == LOW) & (setting1 != 0) & (setting2 !=0)) {//check if the storage button is pressed to record the values, 
  //circuit does not appear to need debounce code here.  Also this is set up so we don't input a value of 0, see below.
    
    EEPROM.update(1, setting1);// save/update the reading only if it has changed to extend life of eeprom. Byte values 0-255.
    //Note if you did enter zero (0) the power will be applied to the motor but it will not move, and the program loop will freeze up.
    //So I added extra code above so we can't save zeros.
    delay(20);

    EEPROM.update(2, setting2);// save/update the reading only if it has changed to extend life of eeprom. Byte values 0-255.
    delay(20);
    Serial.println ("Settings stored");
    delay (500);
    //present the current settings on the LCD:
    Serial.print (setting1);
    Serial.print ("RPM / ");
    delay(400);Serial.print (setting2*50);//multiply Byte value times 50
    Serial.println ("ST");
    delay(400);
}
}
/*
void pin_ISR() {//This ISR has been removed
  buttonState = 3;//set the interrupt button state status to 3
  digitalWrite(ledPin, HIGH);//illuminate the LED when the interupt pin is HIGH
  //Serial.println("feeding");//note when using a push button the input is jumpy so it may trigger this ISR several times
  //also see note above regarding static.
  //Note, delays don't work in an ISR, and one cannot incorporate the peristaltic pump commands in here, so that is why we set the buttonstate as
  //a global variable to feed to the loop.
}*/
