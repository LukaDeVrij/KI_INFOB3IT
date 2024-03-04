#include <Arduino.h>
#include <LiquidCrystal.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <NewPing.h>

// put function declarations here:
void button0Press();
void button1Press();
void printOpMenu();
void scrollInOpMode();
void opModeSelection();
void printMainMenu();
void exitOpMenu();
void refreshScreen();
void pollDistance();
void sprayChecker();

// initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
const int rs = 12, en = 11, d4 = 10, d5 = 9, d6 = 8, d7 = 7;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
unsigned int refreshPeriod = 2000;
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
int sprayDelay = 30;
int spraysLeft = 2400; // TODO make non-voilatile

// Timing zooi - TODO ram optim possible surely -  now we have 2 timers independent; maybe possible to cut some vars here
unsigned long startMillis;
unsigned long startMillisDist;
unsigned long startMillisSpray;

// Temp sensor
OneWire oneWire(6);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

// Distance sensor
int distanceEcho = A4;
int distanceTrig = A5;
NewPing sonar(distanceTrig, distanceEcho, 200);

int lastDistance = 0; // global var that gets updated once every second

int sprayPin = 4;
// State machine probeersel

enum State
{
	IDLE,
	IN_USE,
	IN_USE_1,
	IN_USE_2,
	CLEANING,
	TRIGGERED1,
	TRIGGERED2,
	SIZE // used for enum>string conversion
};
// https://stackoverflow.com/questions/9150538/how-do-i-tostring-an-enum-in-c++
static const char *StatesNames[] = {"IDLE", "IN_USE", "IN_USE_1", "IN_USE_2", "CLEANING", "TRIGGERED1", "TRIGGERED2"};
// statically check that the size of ColorNames fits the number of Colors
static_assert(sizeof(StatesNames) / sizeof(char *) == SIZE, "sizes dont match");

class StateMachine
{ // TODO make better? with member functions maybe?
public:
	State current_state;
	StateMachine() : current_state(State::IDLE) {} // constructor in cpp weird

	void transition(State to) //  did not intend for this to become massive; if our flash runs out we might need to solve this differently
	{
		switch (current_state)
		{
		case State::IDLE:
			if (to == State::TRIGGERED1 || to == State::TRIGGERED2)
			{
				Serial.println("1setstart");
				startMillisSpray = millis();
				current_state = to;
			}
			break;
		case State::TRIGGERED2:
			if (to == State::TRIGGERED1)
			{
				Serial.println("2setstart");
				startMillisSpray = millis();
				current_state = to;
				break;
			}
		case State::TRIGGERED1:
			if (to == State::IDLE)
			{
				current_state = to;
				break;
			}
		}
	}
};

StateMachine machine;

// Spray related vars
bool sprayingAllowed = true; // var that should be true/false based on movement in room: as soon as no movement (+-5 SECONDS!!!! - signal can fluctuate) spraying is allowed
bool cleaning = false;		 // TODO hook up to magnetic thing for toilet seat

void setup()
{
	Serial.begin(9600);
	// set up the LCD's number of columns and rows:
	lcd.begin(16, 2);
	pinMode(ledPin, OUTPUT);
	pinMode(sprayPin, OUTPUT);
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

	startMillis = millis();		// initial start time  voor temp sensor (niet altijd 0!)
	startMillisDist = millis(); // initial start time  voor temp sensor (niet altijd 0!)
}

void loop()
{
	refreshScreen();
	pollDistance();
	sprayChecker(); // might not be the best way to do this; we could also put it in the transition()
					// Problem there is that we need to supply power to the pin for 30 odd seconds, so we need to also switch it off after 30 seconds
					// How do we do that - not to mention preset the state to IDLE? delay for 30 seconds? not ideal - this isnt either tho
}

void pollDistance()
{
	// Every X seconds update main display with temperature
	// TODO add milis() timing check here
	unsigned int period = 1000;

	// https://forum.arduino.cc/t/using-millis-for-timing-a-beginners-guide/483573				   // get the current "time" (actually the number of milliseconds since the program started)
	if (millis() - startMillisDist >= period) // test whether the period has elapsed
	{
		startMillisDist = millis(); // IMPORTANT to save the start time of the current LED state.

		lastDistance = sonar.ping_cm();
	}
}

void sprayChecker()
{
	unsigned int period = 30000; // sprays can take up to 30 seconds to fire after power on TODO add spraydelay var in there
	if (machine.current_state == State::TRIGGERED1 || machine.current_state == State::TRIGGERED2)
	{
		if (!sprayingAllowed)
		{
			digitalWrite(sprayPin, LOW); // cancel if on
			return;
		}
		if (millis() - startMillisSpray < period)
		{						   // TODO look at this: rollover might be a problem
			digitalWrite(sprayPin, HIGH); // Start power to sprayer
			Serial.println("HIGH");
		}
		else
		{
			digitalWrite(4, LOW); // Stop after 30 seconds
			Serial.println("LOW");
			startMillisSpray = millis();
			// Based on state; go through this once more or back to IDLE
			if (machine.current_state == State::TRIGGERED2)
			{
				machine.transition(State::TRIGGERED1);
			}
			else
			{
				machine.transition(State::IDLE); // Transition back to IDLE state - so we only go through this once
			}
		}
	}
}

void refreshScreen()
{
	if (!opMode)
	{
		// Every X seconds update main display with temperature
		// TODO add milis() timing check here
		unsigned int period = 2000;

		// https://forum.arduino.cc/t/using-millis-for-timing-a-beginners-guide/483573
		if (millis() - startMillis >= period) // test whether the period has elapsed
		{
			startMillis = millis(); // IMPORTANT to save the start time of the current LED state.
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
			Serial.println("pressed1");
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
		opModeCursor = -1;
	}
	opModeCursor++;
	printOpMenu();
}

void opModeSelection()
{

	// niet de beste manier om erachter te komen op welke optie je staat maar anders moeten we mappen en ram is schaars
	Serial.println(opModeCursor);
	switch (opModeCursor)
	{
	case 0:
		machine.transition(State::TRIGGERED1);
		exitOpMenu();
		break;
	case 1:
		// SprayDelay
		lcd.setCursor(0, 0);
		if (sprayDelay >= 90 ? sprayDelay = 30 : sprayDelay += 5)
			;			   // ternary: cond ? then : else
		char delayStr[20]; // TODO ram optim possible
		sprintf(delayStr, "SprayDelay: %d", sprayDelay);
		// Copy the formatted string to opMenuLines[1]
		strcpy(opMenuLines[1], delayStr);
		lcd.print(">");
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
}

void printMainMenu()
{
	sensors.requestTemperatures();
	lcd.clear();
	lcd.setCursor(0, 0);
	lcd.print(spraysLeft);
	lcd.print(" - ");
	lcd.print(sensors.getTempCByIndex(0)); // TODO this is slow; hence flicker - can be fixed by either not clearing the lcd or just doing this last (I THINK)
	lcd.print("C");
	lcd.print("  ");
	if (sprayingAllowed)
	{
		lcd.setCursor(15, 0);
		lcd.print("*");
	}
	lcd.setCursor(0, 1);
	lcd.print(StatesNames[machine.current_state]);
}
