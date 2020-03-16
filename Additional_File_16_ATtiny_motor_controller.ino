//"Digital Gateway": This is the script to use an ATtiny85 to control the Tunze DC motor 
//controller for the Comline DOC 9004 DC protein skimmer. When programming the ATtiny85 using the Arduino
//IDE, set the Board to ATtiny25/45/85; set the Processor to ATtiny85; set the Clock to Internal 1 MHz.
//This last step is critical! Never set this to an external clock! Port does not need to be specified for 
//the Sparkfun USB programmer.  Also set the Programmer to USBtinyISP (normally it is set to AVRISP mkII for the Uno).
//Sketch by J. J. Henry; January, 2019 UIUC.  This project was funded by and NSF EDGER grant (no 1827533).

void setup() {
  
pinMode(3, INPUT);// make this pin an input pin for sensing digital state from ATMEL 1724 pin 3
pinMode(2, INPUT);// make this pin an input pin for for sensing digital state from optocoupler
pinMode(0, OUTPUT);//make this pin an output pin to control the state of the motor controller
//may need to use 10K pull down resistors tied to ground to pull input pins LOW normally
digitalWrite(0, LOW);//start with this pin set to LOW = motor running
delay(1000);// wait one second to give DC controller time to startup first, if there is a power outage
}


void loop() {
if (digitalRead(2) == 1){//to activate a feeding cycle, input from Walchem controller via optocoupler
  digitalWrite(0, HIGH);//turn off skimmer motor
}
else if (digitalRead(3) == 1){//to activate cup cleaning, input from the DC motor controller via pushbutton
  digitalWrite(0, HIGH);//turn off skimmer motor
}
else
{
  digitalWrite(0, LOW);//turn on skimmer motor
}
delay(1000);//wait one second
}
