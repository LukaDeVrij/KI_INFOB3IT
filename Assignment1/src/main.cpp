#include <Arduino.h>
#include <LiquidCrystal.h>

// put function declarations here:
int myFunction(int, int);

// initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
const int rs = 9, en = 8, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
// constants won't change. They're used here to set pin numbers:
const int buttonPin = 0; // the number of the pushbutton pin
const int ledPin = 13;   // the number of the LED pin

int buttonPresses = 0;
int currentlyPressed = false;
// variables will change:
int buttonState = 0; // variable for reading the pushbutton status

void setup()
{
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);

  pinMode(ledPin, OUTPUT);
  // initialize the pushbutton pin as an input:
  pinMode(buttonPin, INPUT);
}

void loop()
{

  // read the state of the pushbutton value:
  buttonState = digitalRead(buttonPin);

  // check if the pushbutton is pressed. If it is, the buttonState is HIGH:
  if (buttonState == HIGH)
  {
    // turn LED on:
    currentlyPressed = true;
    digitalWrite(ledPin, LOW);
  }
  else
  {
    // turn LED off:
    if (currentlyPressed == true)
    {
      buttonPresses++;
      currentlyPressed = false;
    }

    digitalWrite(ledPin, HIGH);
  }
  // set the cursor to column 0, line 1
  // (note: line 1 is the second row, since counting begins with 0):
  lcd.setCursor(0, 1);
  // print the number of seconds since reset:
  lcd.print(buttonPresses);
}
