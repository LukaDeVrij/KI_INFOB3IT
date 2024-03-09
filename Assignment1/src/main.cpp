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
void checkUsageType();
void usageEnded();
void checkOverrideButton();
void magnetCheck();
bool lightCheck();
void motionDetect();

// Pins:
//  constants won't change. They're used here to set pin numbers:
const int rs = 12, en = 11, d4 = 10, d5 = 9, d6 = 8, d7 = 7; // LCD pins
const int button0Pin = 2;									 // the number of the pushbutton pin
const int button1Pin = 3;									 // the number of the pushbutton pin
const int overrideButtonPin = A0;
const int led0Pin = 13; // the number of the LED pin
const int led1pin = A2;
const int distanceEcho = A4;
const int distanceTrig = A5;
const int sprayPin = 4;
const int tempPin = 6;
const int ldrPin = A1;
const int magnetPin = A3;
const int motionPin = 5;

const int basePingInterval = 1000;
// initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
unsigned int refreshPeriod = 2000;

int button0PressCount = 0;
int button0Pressed = false;
int button1Pressed = false;

// variables will change:
volatile int button0State = 0; // variable for reading the pushbutton status
volatile int button1State = 0; // variable for reading the pushbutton status

// Manual override debouce
int overrideButtonPressed = false;
volatile int overrideButtonState = 0; // variable for reading the pushbutton status
volatile int lastButtonState = HIGH;

// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastDebounceTime = 0; // the last time the output pin was toggled
unsigned long debounceDelay = 50;	// the debounce time; increase if the output flickers

// Op Mode
bool opMode = false;
volatile unsigned int opModeCursor = 0;
char opMenuLines[4][15] = {"Manual trigger", "SprayDelay: ", "Reset counter", "Exit menu"};
unsigned int opMenuLinesSize = sizeof(opMenuLines) / sizeof(opMenuLines[0]);

// User settings
unsigned int sprayDelay = 30;
unsigned int lightLevel = 600;

// EEPROM data
unsigned int spraysLeft = 2400; // TODO make non-voilatile
unsigned int uses = 0;


// Timing zooi - TODO ram optim possible surely -  now we have 2 timers independent; maybe possible to cut some vars here
unsigned long startMillis;
unsigned long startMillisDist;
unsigned long startMillisSpray;
unsigned long startMillisDelay;
unsigned long triggeredTime = 0;

// Motion  sens boolean
bool motionSensed = false;
unsigned long previousMillis;
bool anyMotionInInterval = false;

// Temp sensor
OneWire oneWire(tempPin);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

// Distance sensor

NewPing sonar(distanceTrig, distanceEcho, 200);

unsigned int lastDistance = 0; // global var that gets updated once every second
unsigned int seatedDistanceMax = 100;
unsigned int firstSeatedTime = 0;
unsigned int timeSpentInProx;
bool seated = false;
int consecutiveZeroDistance = 0;
unsigned int usageTypeTimeThreshold = 10000;  // if time >= this value: its a number 2 (make this value a setting in OP MENU?)
unsigned int pingInterval = basePingInterval; // time in between pings

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
		Serial.print("Attempting state switch ");
		Serial.print(StatesNames[current_state]);
		Serial.print(" to ");
		Serial.print(StatesNames[to]);
		Serial.println();
		switch (current_state)
		{
		case State::IDLE:
			if (to == State::TRIGGERED1 || to == State::TRIGGERED2)
			{

				Serial.println("1setstart");
				startMillisSpray = millis();
				// triggeredTime = millis();
				current_state = to;
			}
			if (to == State::IN_USE || to == State::IN_USE_2)
			{
				current_state = to;
			}
			break;
		case State::TRIGGERED2:
			if (to == State::TRIGGERED1)
			{

				Serial.println("2setstart");
				startMillisSpray = millis();
				triggeredTime = millis();
				current_state = to;
			}
			break;
		case State::TRIGGERED1:
			if (to == State::IDLE)
			{
				current_state = to;
			}
			break;
		case State::IN_USE:
			if (to == State::TRIGGERED1)
			{
				Serial.println("succes");
				startMillisSpray = millis();
				if (anyMotionInInterval)
				{
					break;
				}
				current_state = to;
			}
			if (to == State::IN_USE_2)
			{
				current_state = to;
			}
			if (to == State::IDLE)
			{
				current_state = to;
			}
			if (to == State::CLEANING)
			{
				current_state = to;
			}
			break;
		case State::IN_USE_2:
			if (to == State::TRIGGERED2)
			{
				Serial.println("succ7");
				if (anyMotionInInterval)
				{
					break;
				}
				startMillisSpray = millis();
				// triggeredTime = millis();
				current_state = to;
			}
			if (to == State::TRIGGERED1)
			{
				Serial.println("succ7");

				startMillisSpray = millis();
				// triggeredTime = millis();
				current_state = to;
			}
			if (to == State::IDLE)
			{
				current_state = to;
			}
			if (to == State::CLEANING)
			{
				current_state = to;
			}
			break;
		case State::CLEANING:
			if(lastDistance >= seatedDistanceMax || lastDistance == 0)
			{
				current_state = IDLE;
			}
			break;
		}
		// TODO meer cases toevoegen die ook de situaties in pollDistance en usageEnded() reflecteren en daadwerkelijk laten werken
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
	pinMode(led0Pin, OUTPUT);
	pinMode(led1pin, OUTPUT);
	pinMode(sprayPin, OUTPUT);
	// initialize the pushbutton pin as an input:
	pinMode(button0Pin, INPUT);
	attachInterrupt(digitalPinToInterrupt(button0Pin), button0Press, CHANGE); // TODO 2x zelfde pin???
	attachInterrupt(digitalPinToInterrupt(button1Pin), button1Press, CHANGE);

	// Other sensors Initialisation
	pinMode(ldrPin, INPUT);
	pinMode(motionPin, INPUT);
	pinMode(magnetPin, INPUT);
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
	sprayChecker();
	checkOverrideButton();
	magnetCheck();
}

void checkOverrideButton()
{
	// https://docs.arduino.cc/built-in-examples/digital/Debounce/
	//  read the state of the switch into a local variable:
	int reading = digitalRead(overrideButtonPin);
	reading = !reading;
	// Serial.println(reading);
	// check to see if you just pressed the button
	// (i.e. the input went from LOW to HIGH), and you've waited long enough
	// since the last press to ignore any noise:

	// If the switch changed, due to noise or pressing:
	if (reading != lastButtonState)
	{
		// reset the debouncing timer
		lastDebounceTime = millis();
	}

	if ((millis() - lastDebounceTime) > debounceDelay)
	{
		// whatever the reading is at, it's been there for longer than the debounce
		// delay, so take it as the actual current state:

		// if the button state has changed:
		if (reading != overrideButtonState)
		{
			overrideButtonState = reading;

			// only toggle the LED if the new button state is HIGH
			// machine.transition(State::TRIGGERED1);
			if (overrideButtonState == LOW)
			{
				machine.transition(State::IDLE);
				machine.transition(State::TRIGGERED1);
			}
		}
	}

	// save the reading. Next time through the loop, it'll be the lastButtonState:
	lastButtonState = reading;
}

void pollDistance()
{
	// Every X seconds update main display with temperature
	// TODO add milis() timing check here

	// https://forum.arduino.cc/t/using-millis-for-timing-a-beginners-guide/483573				   // get the current "time" (actually the number of milliseconds since the program started)
	if (millis() - startMillisDist >= pingInterval) // test whether the period has elapsed
	{
		if (!lightCheck)
		{
			pingInterval *= 2; // ECO mode
		}
		else
		{
			pingInterval = basePingInterval;
		}
		// Eveything in this loop is done once per period
		startMillisDist = millis(); // IMPORTANT to save the start time of the current LED state.

		lastDistance = sonar.ping_cm();

		if (lastDistance >= seatedDistanceMax || lastDistance == 0) // LastDistance kan ook 0 worden als distance sens niets ziet: dan is er dus niemand seated
		// TODO BUG: distance sens doet soms gwn ff 0 voor saus -> handle idk how
		{
			// Iterative bug fix voor bovenstaande - deze code is echt bagger in deze functie sorry ;-;
			consecutiveZeroDistance += 1;
			if (consecutiveZeroDistance <= 5)
			{
				// 0 was a false negative - someone still here!
				Serial.print(consecutiveZeroDistance);
				Serial.println(" - 0 seen: waiting for confirmation...");
				return;
			}

			if (seated == false)
			{
				// Zodat de code hieronder maar 1 keer gerunt wordt nadat iemand weggegaan is
				return;
			}
			Serial.println("Actually 0!");
			consecutiveZeroDistance = 0;
			// Distance sensors senses something far away/nothing at all
			seated = false;
			usageEnded();
			return;
		}
		consecutiveZeroDistance = 0;
		// Someone in distance - someone is seated
		if (seated == false)
		{
			// Happens once as soon as this code is reached
			seated = true;
			firstSeatedTime = millis();
		}
		else
		{
			// Als seated == true: update timeSpentInProx constantly
			timeSpentInProx = millis() - firstSeatedTime;
			if (timeSpentInProx >= usageTypeTimeThreshold)
			{
				machine.transition(State::IN_USE_2);
				// TODO prob more here
			}
			else
			{
				machine.transition(State::IN_USE);
				// IN USE -> dus we weten nog niet of de persoon weg gaat binnen de tijd, dan wordt t een nummer 1, maar eigenlijk boeit die state niet want dan gaan we gelijk naar TRIG1
				// eigenlijk hebben we IN_USE_1 niet echt nodig dus... oh well
			}
		}
	}
}

void usageEnded()
{
	Serial.println("Usage ended! noone in prox anymore");
	consecutiveZeroDistance = 0; // reset
	// Only triggeres after someone is out of proximity (seated == false)
	timeSpentInProx = millis() - firstSeatedTime; // Rollover problems? I am scared
	if (timeSpentInProx >= usageTypeTimeThreshold)
	{
		// Number 2
		machine.transition(State::TRIGGERED2);
		// TODO more here?
	}
	else
	{
		// Number 1
		machine.transition(State::TRIGGERED1);
		// TODO more here?
	}
}

void sprayChecker()
{
	unsigned int period = 22000; // sprays can take up to 30 seconds to fire after power on TODO add spraydelay var in there
	if (machine.current_state == State::TRIGGERED1 || machine.current_state == State::TRIGGERED2)
	{
		if (!sprayingAllowed || opMode) // in OpMode ook niet sprayen
		{
			digitalWrite(sprayPin, LOW); // cancel if on
			return;
		}
		if (millis() - triggeredTime < 3 * 1000)
		{ // Slight delay that is always there: to make sure Trig2 > Trig1 is smooth
			Serial.println("Delaying...");
			return;
		}
		if (millis() - startMillisSpray < period)
		{								  // TODO look at this: rollover might be a problem
			digitalWrite(sprayPin, HIGH); // Start power to sprayer
		}
		else
		{
			digitalWrite(sprayPin, LOW); // Stop after 30 seconds
			startMillisSpray = millis();

			// Based on state; go through this once more or back to IDLE
			if (machine.current_state == State::TRIGGERED2)
			{
				Serial.println("Trig2 finished, changing to Trig1");
				// triggeredTime = millis(); // idk
				machine.transition(State::TRIGGERED1);
				uses++;
			}
			else
			{
				machine.transition(State::IDLE); // Transition back to IDLE state - so we only go through this once
				uses++;

				Serial.println("Printing to eeprom");
				spraysLeft = spraysLeft - uses;
				uses = 0;
				Serial.println(spraysLeft);
				uses = 0;

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

unsigned long lastPressed0 = 0;
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
			Serial.println("pressed0");
			if (lastPressed0 + 250 >= millis()) // bouncer
			{
				Serial.println("bounce caught");
				digitalWrite(led0Pin, LOW);
				button0Pressed = false;
				return;
			}
			lastPressed0 = millis();
			// so this is only called once, as soon as button is pressed!

			button0Pressed = true;
			digitalWrite(led0Pin, HIGH);
			if (opMode)
			{
				Serial.println("SE");
				opModeSelection(); // this function does something based on value of opModeCursor
				return;
			};
			opMode = true;
			printOpMenu();
		}
	}
	else // means button is not pressed
	{
		digitalWrite(led0Pin, LOW);
		button0Pressed = false;
	}

	lcd.setCursor(0, 1); // No clue why this is here but if we remove it everything crashes?! classic
}

void motionDetect()
{
	unsigned long currentMillis = millis();
	if (currentMillis - previousMillis >= 10000)
	{
		previousMillis = currentMillis;
		anyMotionInInterval = false;
		Serial.println("motion bool reset");
	}
	int motionState = digitalRead(motionPin);
	if (motionState == HIGH && anyMotionInInterval == false)
	{
		Serial.println("Motion!!!11!");
		anyMotionInInterval = true;
	}
}

unsigned int magnetsApartLast = 0;
bool openToilet = false;
const int cleaningInterval = 1000;
void magnetCheck()
{
	int m = digitalRead(magnetPin);
	if(!openToilet)
	{
		if(m)
		{
			magnetsApartLast = millis();
			openToilet=true;
		}
	}
	else
	{
		Serial.println("open");
		if(!m)
		{
			openToilet = false;
		}
		if(millis() - magnetsApartLast >= cleaningInterval)
		{
			
			machine.transition(State::CLEANING);
		}
	}
}

bool lightCheck()
{
	int v = analogRead(ldrPin);
	// Serial.println(v);
	return (v > lightLevel);
}

unsigned long lastPressed1 = 0;
void button1Press()
{
	// read the state of the pushbutton value:
	button1State = digitalRead(button1Pin);

	if (button1State == LOW) // means button is pressed NOW
	{

		if (!button1Pressed) // if not already pressed down
		{
			Serial.print("pressed1 ");
			Serial.println(millis());
			if (lastPressed1 + 250 >= millis()) // bouncer
			{
				Serial.println("bounce caught");
				digitalWrite(led0Pin, LOW);
				button1Pressed = false;
				return;
			}
			lastPressed1 = millis();
			// this is only called once, as soon as button is pressed!
			button1Pressed = true;
			digitalWrite(led0Pin, HIGH);
			if (opMode)
			{

				scrollInOpMode();
			};
		}
	}
	else
	{
		digitalWrite(led0Pin, LOW);
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
		machine.transition(State::IDLE);
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
	// TODO
	// Suspend all other functionality whenever the menu system is active (status is ‘operator menu active’). Upon exit of the menu system, the system should return to the ‘not in use’ status.
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
	if (anyMotionInInterval)
	{
		lcd.setCursor(15, 0);
		lcd.print("*");
	}
	lcd.setCursor(0, 1);
	lcd.print(StatesNames[machine.current_state]);
	lcd.setCursor(13, 1);
	lcd.print(lastDistance);
}
