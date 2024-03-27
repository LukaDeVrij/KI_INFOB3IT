/*
 Basic ESP8266 MQTT example
 This sketch demonstrates the capabilities of the pubsub library in combination
 with the ESP8266 board/library.
 It connects to an MQTT server then:
  - publishes "hello world" to the topic "outTopic" every two seconds
  - subscribes to the topic "inTopic", printing out any messages
    it receives. NB - it assumes the received payloads are strings not binary
  - If the first character of the topic "inTopic" is an 1, switch ON the ESP Led,
    else switch it off
 It will reconnect to the server if the connection is lost using a blocking
 reconnect function. See the 'mqtt_reconnect_nonblocking' example for how to
 achieve the same result without blocking the main loop.
 To install the ESP8266 board, (using Arduino 1.6.4+):
  - Add the following 3rd party board manager under "File -> Preferences -> Additional Boards Manager URLs":
       http://arduino.esp8266.com/stable/package_esp8266com_index.json
  - Open the "Tools -> Board -> Board Manager" and click install for the ESP8266"
  - Select your ESP8266 in "Tools -> Board"
*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <creds.h> // has credentials for wifi and mqtt broker
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_BMP280.h>
#include <Servo.h>

#define SCREEN_WIDTH 128    // OLED display width, in pixels
#define SCREEN_HEIGHT 64    // OLED display height, in pixels
#define OLED_RESET -1       // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_BMP280 bmp; // I2C

// Pin allocation:
const int selectPin = D6;
const int analogInputPin = A0;
const int servoPin = D5;
const int buttonPin = D7;

const int rodAngle = 90;

WiFiClient espClient;
PubSubClient client(espClient);
Servo myServo;

unsigned long lastMsg = 0;
unsigned long lastRefresh = 0;
unsigned long lastSoil = 0;
unsigned long lastWater = 0;
bool soilDelay = false;
bool waterDelay = false;

#define MSG_BUFFER_SIZE (50)
char msg[MSG_BUFFER_SIZE];
int value = 0;

// Method initialisation
void setup_hardware();
void setup_wifi();
void setup_hardware();
// void callback();
void mqtt_loop();
void hardware_loop();
// void reconnect();

void callback(char *topic, byte *payload, unsigned int length)
{
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    for (int i = 0; i < length; i++)
    {
        Serial.print((char)payload[i]);
    }
    Serial.println();

    // Switch on the LED if an 1 was received as first character
    if ((char)payload[0] == '1')
    {
        digitalWrite(LED_BUILTIN, LOW); // Turn the LED on (Note that LOW is the voltage level
                                        // but actually the LED is on; this is because
                                        // it is active low on the ESP-01)
    }
    else
    {
        digitalWrite(LED_BUILTIN, HIGH); // Turn the LED off by making the voltage HIGH
    }
}

void reconnect()
{
    // Loop until we're reconnected
    while (!client.connected())
    {
        Serial.print("Attempting MQTT connection...");
        // Create a random client ID
        String clientId = "ESP8266Client-";
        clientId += String(random(0xffff), HEX);
        // Attempt to connect
        if (client.connect(clientId.c_str(), mqtt_username, mqtt_password))
        {
            Serial.println("connected");
            // Once connected, publish an announcement...
            client.publish("infob3it/student018/outTopic", "hello world");
            // ... and resubscribe
            client.subscribe("infob3it/student018/inTopic/LED");
        }
        else
        {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
}


void setup()
{
    Serial.begin(9600);

    // Setup pin modes:
    pinMode(selectPin, OUTPUT);
    pinMode(analogInputPin, INPUT);
    pinMode(buttonPin, INPUT);
    pinMode(LED_BUILTIN, OUTPUT); // Initialize the BUILTIN_LED pin as an output

    digitalWrite(LED_BUILTIN, HIGH); // Set LED off by default

    setup_hardware();

    // MQTT
    setup_wifi();
    client.setServer(mqtt_server, 1883);
    client.setCallback(callback); // Callback is called when anything is received
}

void loop()
{

    mqtt_loop();
    hardware_loop();
}

void mqtt_loop()
{
    if (!client.connected())
    {
        reconnect();
    }
    client.loop();

    unsigned long now = millis();
    if (now - lastMsg > 10000)
    {
        lastMsg = now;
        ++value;
        snprintf(msg, MSG_BUFFER_SIZE, "hello world #%ld", value);
        Serial.print("Publish message: ");
        Serial.println(msg);
        client.publish("infob3it/student018/outTopic", msg);
    }
}

void hardware_loop()
{

    unsigned long now = millis();
    if (now - lastRefresh > 2000)
    {
        display.clearDisplay();
        display.setCursor(0,0);

        lastRefresh = now;

        float temp = bmp.readTemperature();
        float pressure = bmp.readPressure();
        display.println(temp);
        display.println(pressure);
        display.display();
        if(!soilDelay)
        {
            Serial.println(analogRead(analogInputPin));
        }
    }
    if(soilDelay && now - lastSoil > 100)
    {
        soilDelay = false;
        lastSoil = now;
        Serial.println(analogRead(analogInputPin));
        digitalWrite(selectPin,LOW);
        
        //plaats vervangend voor button / methode TODO
        waterDelay = true;
        lastWater = now;
        myServo.attach(servoPin);
        myServo.write(rodAngle);

    }
    
    if(now - lastSoil > 10000)
    {
        digitalWrite(selectPin,HIGH);
        soilDelay = true;
        lastSoil = now;
    }
    if(waterDelay && now - lastWater > 1000)
    {
        waterDelay = false;
        myServo.write(0);
        myServo.detach();
    }
    
}

void setup_wifi()
{

    delay(10);
    // We start by connecting to a WiFi network
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    randomSeed(micros());

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

void setup_hardware()
{
    // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
    {
        Serial.println(F("SSD1306 allocation failed"));
    }
    if (!bmp.begin(BMP280_ADDRESS_ALT, BMP280_CHIPID))
    {
        Serial.println(F("BM280 allocation failed"));
    }

    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);

}

