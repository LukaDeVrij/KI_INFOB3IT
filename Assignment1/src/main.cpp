#include <Arduino.h>
#include <LiquidCrystal.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// put function declarations here:
void button0Press();
void button1Press();
void printOpMenu();
void scrollInOpMode();
void opModeSelection();
void printMainMenu();
void exitOpMenu();

// initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
const int rs = 12, en = 11, d4 = 10, d5 = 9, d6 = 8, d7 = 7;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
// constants won't change. They're used here to set pin numbers:
const int button0Pin = 2; // the number of the pushbutton pin
const int button1Pin = 3; // the number of the pushbutton pin
const int ledPin = 13;	  // the number of the LED pin
int button0PressCount = 0;
int button0Pressed = false;
int button1Pressed = false;
// variables will change:
volatile int button0State = 0; // variable for reading the pushbutton status
volatile int button1State = 0; // variable for reading the pushbutton status

// Op Mode
bool opMode = false;
volatile unsigned int opModeCursor = 0;
char opMenuLines[4][15] = {"Manual trigger", "SprayDelay: ", "Reset counter", "Exit menu"};
unsigned int opMenuLinesSize = sizeof(opMenuLines) / sizeof(opMenuLines[0]);

// User settings
int sprayDelay = 10;
int spraysLeft = 2400; // TODO make non-voilatile

// Timing zooi
unsigned long startMillis;
unsigned long currentMillis;

// Temp sensor
OneWire oneWire(6);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

// State machine probeersel

enum States
{
	IDLE,
	IN_USE,
	IN_USE_1,
	IN_USE_2,
	CLEANING,
	TRIGGERED,
	SIZE // used for enum>string conversion
};
// https://stackoverflow.com/questions/9150538/how-do-i-tostring-an-enum-in-c++
static const char *StatesNames[] = {"IDLE", "IN_USE", "IN_USE_1", "IN_USE_2", "CLEANING", "TRIGGERED"};
// statically check that the size of ColorNames fits the number of Colors
static_assert(sizeof(StatesNames) / sizeof(char *) == SIZE, "sizes dont match");

class StateMachine
{ // TODO make better? with member functions maybe?
public:
	States current_state;
	StateMachine() : current_state(States::IDLE) {} // constructor in cpp weird
};

StateMachine machine;

void setup()
{
	Serial.begin(9600);
	// set up the LCD's number of columns and rows:
	lcd.begin(16, 2);
	pinMode(ledPin, OUTPUT);
	// initialize the pushbutton pin as an input:
	pinMode(button0Pin, INPUT);

	attachInterrupt(digitalPinToInterrupt(button0Pin), button0Press, CHANGE);
	attachInterrupt(digitalPinToInterrupt(button1Pin), button1Press, CHANGE);

	sensors.begin(); // temp sensor

	// Define a char array to hold the concatenated string
	char delayStr[20]; // Ensure this array has enough space
	// Using sprintf to format the string
	sprintf(delayStr, "SprayDelay: %d", sprayDelay); // TODO ram optim possible
	// Copy the formatted string to opMenuLines[1]
	strcpy(opMenuLines[1], delayStr);

	startMillis = millis(); // initial start time  voor temp sensor (niet altijd 0!)
}

void loop()
{
	if (!opMode)
	{
		// Every X seconds update main display with temperature
		// TODO add milis() timing check here
		unsigned int period = 2000;

		// https://forum.arduino.cc/t/using-millis-for-timing-a-beginners-guide/483573
		currentMillis = millis();				   // get the current "time" (actually the number of milliseconds since the program started)
		if (currentMillis - startMillis >= period) // test whether the period has elapsed
		{
			startMillis = currentMillis; // IMPORTANT to save the start time of the current LED state.
			printMainMenu();
		}
	}
}

void button0Press()
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
				return;
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

void button1Press()
{
	// read the state of the pushbutton value:
	button1State = digitalRead(button1Pin);

	if (button1State == LOW) // means button is pressed NOW
	{

		if (!button1Pressed) // if not already pressed down
		{
			// this is only called once, as soon as button is pressed!
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
		digitalWrite(ledPin, LOW);
		button1Pressed = false;
	}
}

void printOpMenu()
{
	// print first line, and second line if not OutOfBounds
	lcd.setCursor(0, 0);
	lcd.clear();
	lcd.print(">");
	lcd.print(opMenuLines[opModeCursor]);

	if (opModeCursor != opMenuLinesSize - 1)
	{
		lcd.setCursor(0, 1);
		lcd.print(" ");
		lcd.print(opMenuLines[opModeCursor + 1]);
	}
}

void scrollInOpMode()
{
	if (opModeCursor == opMenuLinesSize - 1)
	{
		// Prevent scrolling further than possible -> TODO wrap around maybe (now youre stuck on Go Back)
		return;
	}
	opModeCursor++;
	printOpMenu();
	Serial.println("SC");
}

void opModeSelection()
{

	// niet de beste manier om erachter te komen op welke optie je staat maar anders moeten we mappen en ram is schaars
	Serial.println(opModeCursor);
	switch (opModeCursor)
	{
	case 0:
		// TODO manual trigger
		// Set state to triggered-1 (or 2?)
		// Go back to main menu automatically
		exitOpMenu();
		break;
	case 1:
		// SprayDelay
		lcd.setCursor(0, 0);
		if (sprayDelay >= 60 ? sprayDelay = 0 : sprayDelay += 5)
			;			   // ternary: cond ? then : else
		char delayStr[20]; // TODO ram optim possible
		sprintf(delayStr, "SprayDelay: %d", sprayDelay);
		// Copy the formatted string to opMenuLines[1]
		strcpy(opMenuLines[1], delayStr);
		lcd.print(opMenuLines[1]);

		break;
	case 2:
		spraysLeft = 2500;
		// TODO write this to EEPROM?
		exitOpMenu();
		break;
	case 3:
		exitOpMenu();
		break;
	}
}

void exitOpMenu()
{
	lcd.clear();
	opMode = false;
	opModeCursor = 0;
	printMainMenu();
}

void printMainMenu()
{
	sensors.requestTemperatures();

	lcd.setCursor(0, 0);
	lcd.print(spraysLeft);
	lcd.print(" - ");
	lcd.print(sensors.getTempCByIndex(0));
	lcd.print("C");
	lcd.setCursor(0, 1);
	lcd.print(StatesNames[machine.current_state]);
}
