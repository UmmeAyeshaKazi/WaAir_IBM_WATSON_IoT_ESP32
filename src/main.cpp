#include "Arduino.h"
#include "Adafruit_Sensor.h"
#include "DHT.h"
#include "WiFi.h"
#include "PubSubClient.h"
#include "WiFiClient.h"

/* Author     : Umme Ayesha Kazi
*  Date       : 07-06-2021
*  IDE        : VS Code 1.56.2
*  Board      : DOIT ESP32 DEVKIT V1 
*  Framework  : PlatformIO & Arduino
*  Platform   : espressif32
*  Library    : adafruit/DHT sensor library@^1.4.2
*/

#define WIFI_NETWORK "Galaxy M51 E5ED" // defined constant value wifi ssid
#define WIFI_PASSWORD "1234567890" // defined constant value wifi pass
#define WIFI_TIMEOUT_MS 60000		  // defined constant value 60 seconds
#define DHTTYPE DHT11				  // Setting up sensor DHT sensor type to DHT11

/*	Need to define all the credentials that you have copied while adding a device on Watson platform.
*	Organization ID			: 63hyrd
*	Device Type				: ESP32
*	Device ID				: 4DC6B52DE6B4
*	Authentication Method	: use-token-auth
*	Authentication Token	: bt(wV*8v+UM6VyRX2P
*/

#define ORG "63hyrd"			   // Connection Information : Organization ID
#define DEVICE_TYPE "ESP32"		   // Connection Information : Device Type
#define DEVICE_ID "4DC6B52DE6B4"   // Connection Information : Device ID
#define TOKEN "bt(wV*8v+UM6VyRX2P" // Connection Information : Auth Token

/*  Using MQTT protocol to send and receive messages thats why we need PubSubClient.
*   GPIO D13 & GPIO D15 are used by WiFi.h Library 
*   Connect Analog pin A0 of MQ6 to GPIO D34 pin
*   GPIO pin D34 is input only pin (also known as ADC1 CH6)
*	For code flashing first press BOOT button of the ESP32
*   ESP32 Drivers : CP210x USB to UART Bridge VCP Drivers v10.1.10
*/

#define SENSOR_MQ6 34 // MQ6 Sensor analog pin A0 at GPIO pin 34
#define LED_GREEN 13  // LED GREEN output pin at GPIO pin 13
#define LED_RED 12	  // LED RED output pin at GPIO pin 12
#define BUILT_LED 2	  // BUILTIN LED output at GPIO pin 02
#define DHT_PIN 4	  // Set DHT11 sensor pin at GPIO pin 04

/*	Provide link on which we will publish the data. Also, provide topics on which we will publish data.
*	We have 2 topics, 1st for temperature and 2nd for humidity and give name as :
*	1. Event ID for temperature : IBM_Cloud_Watson_Temperature
*	2. Event ID for humidity 	: IBM_Cloud_Watson_Humidity
*	Give Auth method as “use-token-auth” and then define clientID as shown below.
*/

char server[] = ORG ".messaging.internetofthings.ibmcloud.com";
char pubTopic1[] = "iot-2/evt/IBM_Cloud_Watson_Temperature/fmt/json";
char pubTopic2[] = "iot-2/evt/IBM_Cloud_Watson_Humidity/fmt/json";
char authMethod[] = "use-token-auth";
char token[] = TOKEN;
char clientId[] = "d:" ORG ":" DEVICE_TYPE ":" DEVICE_ID;

/*  Power Usage & Power Pin connections information :
*   DHT11 sensor uses 3.3 Volts (3V3 pin on board) & GND pin
*   MQ6 sensor uses 5.0 Volts (VIN pin on board) & GND pin
*/

int sensorValue;   // For storing the analog input value from sensor to ESP32( max 4095 )
float sensorVolts; // For storing the voltage input value from sensor to ESP32 ( max 5.0 )
float temperature; // For storing the temperature input value from DHT11 sensor
float humidity;	   // For storing the humidity input value from DHT11 sensor
uint64_t chipid;   // (unsigned long long) 8 bytes -> [0-18446744073709551615] or [0x0000000000000000-0xFFFFFFFFFFFFFFFF]
long lastMsg = 0;  //

WiFiClient wifiClient;
PubSubClient client(server, 1883, NULL, wifiClient);

DHT dht(DHT_PIN, DHTTYPE); // Initialize DHT sensor.

/* USER DEFINED FUNCTION */ // Prints the ESP32 MAC ID or Device ID :

void showDeviceChipID()
{
	chipid = ESP.getEfuseMac();
	Serial.printf(" ESP32 Chip ID = %04X", (uint16_t)(chipid >> 32));
	Serial.printf("%08X\n", (uint32_t)chipid);
}

/* USER DEFINED FUNCTION */ // Read temperature values from DHT11 Sensor :

float readDHTTemperature()
{
	float t = dht.readTemperature(); // readTemperature is a inbuilt function
	if (isnan(t))					 // isnan is an inbuilt macro/function -> NaN (Not-A-Number) value
	{								 // The NaN values are used to identify undefined or non-representable values for floating-point elements.
		Serial.println("Failed to read from DHT sensor!");
		return 0.0; // if value of t is NULL then it will return 0.0
	}
	else
	{
		return t; // On a successfull read it will return the actual value.
	}
}

/* USER DEFINED FUNCTION */ // Read humidity values from DHT11 Sensor :

float readDHTHumidity()
{
	float h = dht.readHumidity(); // readHumidity is a inbuilt function
	if (isnan(h))				  // isnan is an inbuilt macro/function -> NaN (Not-A-Number) value
	{							  // The NaN values are used to identify undefined or non-representable values for floating-point elements.
		Serial.println("Failed to read from DHT sensor!");
		return 0.0; // if value of h is NULL then it will return 0.0
	}
	else
	{
		return h; // On a successfull read it will return the actual value.
	}
}

/* USER DEFINED FUNCTION */ // WiFi Connection Setup :

void connectionToWiFi()
{
	Serial.print("\n\nConnecting to WiFi ");
	WiFi.mode(WIFI_STA);					 // sets ESP32 operating mode as station/client
	WiFi.begin(WIFI_NETWORK, WIFI_PASSWORD); // passed ssid and pass and started connection

	unsigned long startAttemptTime = millis(); // millis() assigns the starting time of ESP 32

	while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < WIFI_TIMEOUT_MS)
	{
		Serial.print("."); // prints the " . " after every 100 milliseconds
		delay(100);			 // this loop ends when ESP32 connects to the wifi
	}

	if (WiFi.status() != WL_CONNECTED)
	{
		Serial.println("\nFailed to Connect WiFi !"); // it prints the fail message on serial interface
	}
	else
	{
		Serial.print("\n\nConnected to WiFi ! IP add. : "); // it prints the success message on serial
		Serial.print(WiFi.localIP());						// it prints the IP address of ESP32 on serial
		showDeviceChipID();									// it prints the device ID of ESP32 board on serial
	}

	if (!client.connected())
	{
		Serial.print("Reconnecting client to : ");
		Serial.println(server);
		while (!client.connect(clientId, authMethod, token))
		{
			Serial.print(".");
			delay(500);
		}
		Serial.println("Bluemix connected !\n");
	}
}

void setup()
{
	// put your setup code here, to run once:

	pinMode(BUILT_LED, OUTPUT);

	Serial.begin(9600);
	dht.begin();
	connectionToWiFi();
}

void loop()
{
	// put your main code here, to run repeatedly:
	Serial.printf("\nTemperature : %.2f", readDHTTemperature());
	Serial.printf("\nHumidity : %.2f", readDHTHumidity());

	digitalWrite(BUILT_LED, HIGH);
	delay(100);
	digitalWrite(BUILT_LED, LOW);
	delay(100);

	client.loop();
	long now = millis();
	if (now - lastMsg > 3000)
	{
		lastMsg = now;
		float humidity = dht.readHumidity();
		float temperature = dht.readTemperature();
		String payload = "{\"d\":{\"Name\":\"" DEVICE_ID "\"";
		payload += ",\"temperature\":";
		payload += temperature;
		payload += "}}";

		Serial.print("Sending payload: ");
		Serial.println(payload);

		if (client.publish(pubTopic1, (char *)payload.c_str()))
		{
			Serial.println("Publish ok");
		}
		else
		{
			Serial.println("Publish failed");
		}
		String payload1 = "{\"d\":{\"Name\":\"" DEVICE_ID "\"";
		payload1 += ",\"humidity\":";
		payload1 += humidity;
		payload1 += "}}";

		if (client.publish(pubTopic2, (char *)payload1.c_str()))
		{
			Serial.println("Publish ok");
		}
		else
		{
			Serial.println("Publish failed");
		}
	}
}