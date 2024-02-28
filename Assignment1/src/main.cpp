#include <Arduino.h>
#include <LiquidCrystal.h>

// put function declarations here:
void pin0Press();
void startOpMode();
void scrollInOpMode();
void opModeSelection();

// initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
const int rs = 9, en = 8, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
// constants won't change. They're used here to set pin numbers:
const int button0Pin = 0; // the number of the pushbutton pin
const int ledPin = 13;	  // the number of the LED pin
int button0PressCount = 0;
int button0Pressed = false;

// variables will change:
volatile int buttonState = 0; // variable for reading the pushbutton status

// Op Mode
int opMode = false;
int opModeCursor = 0;

// typedef enum
// {
// 	STATE_IDLE,
// 	STATE_IN_USE,
// 	STATE_IN_USE_1,
// 	STATE_IN_USE_2,
// 	STATE_CLEANING,
// 	STATE_TRIGGERED,
// } State;

void setup()
{
	Serial.begin(9600);
	// set up the LCD's number of columns and rows:
	lcd.begin(16, 2);
	pinMode(ledPin, OUTPUT);
	// initialize the pushbutton pin as an input:
	pinMode(button0Pin, INPUT);

	attachInterrupt(0, pin0Press, CHANGE);
}

void loop()
{
}

void pin0Press()
{
	// TODO Check some state maybe
	// Check if OP mode - then pin 0 is SELECT

	// read the state of the pushbutton value:
	buttonState = digitalRead(button0Pin);
	Serial.println(buttonState);

	if (buttonState == LOW) // means button is pressed NOW
	{
		if (!button0Pressed) // if not already pressed down
		{
			// this is only called once, as soon as button is pressed!
			
			button0Pressed = true;
			digitalWrite(ledPin, HIGH);
			if (opMode) {
				opModeSelection(); // this function must do something based on opModeCursor (not yet implemented)
			};
			opMode = true;
			startOpMode();
		}
	}
	else // means button is not pressed
	{
		digitalWrite(ledPin, LOW);
	}
	// set the cursor to column 0, line 1
	// (note: line 1 is the second row, since counting begins with 0):
	lcd.setCursor(0, 1);
	// print the number of seconds since reset:
	lcd.print(button0PressCount);
}

void startOpMode()
{
	lcd.setCursor(0,0);
	lcd.print("Do A");
	lcd.setCursor(0,1);
	lcd.print("Do B > 60");
}

void scrollInOpMode()
{
}

void opModeSelection(){

}
