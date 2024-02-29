#include <Arduino.h>
#include <LiquidCrystal.h>

// put function declarations here:
void pin0Press();
void pin1Press();
void printOpMenu();
void scrollInOpMode();
void opModeSelection();

// initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
const int rs = 9, en = 8, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
// constants won't change. They're used here to set pin numbers:
const int button0Pin = 0; // the number of the pushbutton pin
const int button1Pin = 1; // the number of the pushbutton pin
const int ledPin = 13;	  // the number of the LED pin
int button0PressCount = 0;
int button0Pressed = false;
int button1Pressed = false;
// variables will change:
volatile int button0State = 0; // variable for reading the pushbutton status
volatile int button1State = 0; // variable for reading the pushbutton status

// Op Mode
int opMode = false;
unsigned int opModeCursor = 0;
char opMenuLines[5][16] = {"Do A", "Do B > 60", "Go Back", "-"};

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


	// i dont get interrupts: we zouden alleen op pin 2 en 3 kunnen interrupten; maar heb toch echt pin 0 geintewrrupt?
	attachInterrupt(0, pin0Press, CHANGE); 
	attachInterrupt(digitalPinToInterrupt(1), pin1Press, CHANGE);
}

void loop()
{
}

void pin0Press()
{
	// TODO Check some state maybe
	// Check if OP mode - then pin 0 is SELECT

	// read the state of the pushbutton value:
	button0State = digitalRead(button0Pin);

	if (button0State == LOW) // means button is pressed NOW
	{
		if (!button0Pressed) // if not already pressed down
		{
			// this is only called once, as soon as button is pressed!

			button0Pressed = true;
			digitalWrite(ledPin, HIGH);
			if (opMode)
			{
				Serial.println("SE");
				opModeSelection(); // this function must do something based on opModeCursor (not yet implemented)
			};
			opMode = true;
			printOpMenu();
		}
	}
	else // means button is not pressed
	{
		digitalWrite(ledPin, LOW);
		button0Pressed = false;
	}

	lcd.setCursor(0, 1); // No clue why this is here but if we remove it everything crashes?!
}

void pin1Press()
{
	// read the state of the pushbutton value:
	button1State = digitalRead(button1Pin);

	if (button1State == LOW) // means button is pressed NOW
	{

		if (!button1Pressed) // if not already pressed down
		{
			// this is only called once, as soon as button is pressed!
			Serial.println("b1");
			button1Pressed = true;
			digitalWrite(ledPin, HIGH);
			if (opMode)
			{
				
				scrollInOpMode();
			};
		}
	}
	else
	{
		digitalWrite(ledPin, HIGH);
		button1Pressed = false;
	}
}

void printOpMenu()
{

	lcd.setCursor(0, 0);
	lcd.clear();
	lcd.print(">");
	lcd.print(opMenuLines[opModeCursor]);

	if (opModeCursor < sizeof(opMenuLines))
	{
		lcd.setCursor(0, 1);
		lcd.print(" ");
		lcd.print(opMenuLines[opModeCursor + 1]);
	}
}

void scrollInOpMode()
{
	opModeCursor++;
	printOpMenu();
	Serial.println("SC");
}

void opModeSelection()
{

	// niet de beste manier om erachter te komen op welke optie je staat maar anders moeten we mappen en ram is schaars
	Serial.println(opModeCursor);
	// switch(){

	// }
}
