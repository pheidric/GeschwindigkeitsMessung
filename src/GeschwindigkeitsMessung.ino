/**
 * Geschwindigkeitsmessung
 * Arduino Uno
 *
 * - v1.2 Code aufgeräumt
 *   02.04.2020
 *
 * - v1.1
 *   15.02.2020 10:47
 */

#include <Arduino.h>
#include <avr/interrupt.h>
#include <TM1637Display.h>

//#include "enums.h"
//#include "konfiguration.ino"

// Konfiguration

//Geschwindigkeitsmessung Anschlussplan (Arduino Uno, Nano theoretisch identisch)
//
//--- IR-Sensoren ---------------------------------
//const int PIN_SENSOR_1 = 2;
//const int PIN_SENSOR_2 = 3;
//
//--- Taster --------------------------------------
//const int PIN_BTN_RESET = 4;				(Messung zurücksetzen)
//const int PIN_BTN_DISPLAY = 5;			(Anzeigemodus umschalten: Zeit, Geschwindigkeit, Geschwindigkeit mit Maßstab)
//const int PIN_BTN_UNIT = 6;				(Einheit für Geschwindigkeit umschalten: m/s, km/h)
//
//--- Leuchtdioden --------------------------------
//const int PIN_LED_STATE_IDLE = 7;			(Status: Leerlauf/bereit)
//const int PIN_LED_STATE_RUNNING = 8;		(	"	 Messung läuft)
//const int PIN_LED_STATE_DONE = 9;			(	"	 Messung abgeschlossen
//
//const int PIN_LED_MODE_TIME = 10;			(Anzeigemodus: Zeit)
//const int PIN_LED_MODE_SPEED = 11;		(		"	   Geschwindigkeit)
//const int PIN_LED_MODE_SPEED_SCALED = 12;	(		"	    Geschwindigkeit mit Maßstab)
//
//const int PIN_LED_UNIT_MS = 13;			(Einheit: m/s)
//const int PIN_LED_UNIT_KMH = 14;			(	"	  km/h, A0)
//
//--- Anschlüsse Display (TM1637) -----------------
//const int PIN_DISPLAY_DIO = 15;			A1
//const int PIN_DISPLAY_CLK = 16;			A2


//const int LOOP_PAUSE = 1000;		// noch notwendig?
const int DISTANCE = 100;			// cm
//const int DISTANCE = 14;			// cm
const int SCALE = 87;
const int DEBOUNCE_SENSOR = 2500;	// ms
const int DEBOUNCE_BUTTONS = 250;	// ms

const byte DISPLAY_BRIGHTNESS = 7;	// 0 .. 7

// TODO: Konfiguration einstellbar? mit Potis?? <= erst mal nicht...

// Pins

const int PIN_SENSOR_1 = 2;
const int PIN_SENSOR_2 = 3;

const int PIN_BTN_RESET = 4;
const int PIN_BTN_DISPLAY = 5;
const int PIN_BTN_UNIT = 6;

const int PIN_LED_STATE_IDLE = 7;
const int PIN_LED_STATE_RUNNING = 8;
const int PIN_LED_STATE_DONE = 9;

const int PIN_LED_MODE_TIME = 10;
const int PIN_LED_MODE_SPEED = 11;
const int PIN_LED_MODE_SPEED_SCALED = 12;

const int PIN_LED_UNIT_MS = 13;
const int PIN_LED_UNIT_KMH = 14;	// A0

const int PIN_DISPLAY_DIO = 15;		// A1
const int PIN_DISPLAY_CLK = 16;		// A2


//extern int PIN_DISPLAY_DIO;
//extern int PIN_DISPLAY_CLK;

enum STATE {
	S_IDLE,
	S_RUNNING,
	S_DONE
};

enum DISPLAY_MODE {
	DM_TIME,
	DM_SPEED,
	DM_SPEED_SCALED
};

enum UNIT {
	U_MS,
	U_KMH
};

// Variablen

volatile unsigned long start = 0;
volatile unsigned long end = 0;

volatile byte phase = S_IDLE;
volatile byte displayMode = DM_TIME;
volatile byte unit = U_MS;

// TODO: volatile??
unsigned long last_btn_reset = 0;
unsigned long last_btn_display = 0;
unsigned long last_btn_unit = 0;

volatile byte loopInterrupted = 0;
int displayedValue = -1;

TM1637Display tm1637(PIN_DISPLAY_CLK, PIN_DISPLAY_DIO);

// Funktionen

void setup_()
{
	Serial.begin(9600);
	Serial.println("===== SETUP =====");

	pinMode(PIN_SENSOR_1, INPUT_PULLUP);
	pinMode(PIN_SENSOR_2, INPUT_PULLUP);

	pinMode(PIN_BTN_RESET, INPUT_PULLUP);
	pinMode(PIN_BTN_DISPLAY, INPUT_PULLUP);
	pinMode(PIN_BTN_UNIT, INPUT_PULLUP);

	pinMode(PIN_LED_STATE_IDLE, OUTPUT);
	pinMode(PIN_LED_STATE_RUNNING, OUTPUT);
	pinMode(PIN_LED_STATE_DONE, OUTPUT);

	pinMode(PIN_LED_MODE_TIME, OUTPUT);
	pinMode(PIN_LED_MODE_SPEED, OUTPUT);
	pinMode(PIN_LED_MODE_SPEED_SCALED, OUTPUT);

	pinMode(PIN_LED_UNIT_MS, OUTPUT);
	pinMode(PIN_LED_UNIT_KMH, OUTPUT);

//	attachInterrupt(digitalPinToInterrupt(PIN_SENSOR_1), sensorOne, FALLING);
//	attachInterrupt(digitalPinToInterrupt(PIN_SENSOR_2), sensorTwo, FALLING);

	// display vorbereiten
	tm1637.setBrightness(DISPLAY_BRIGHTNESS);

//	prepareInterrupts();
//	startIdle(true);
	reset(true);
}

// The loop function is called in an endless loop
void loop_()
{

	// Loop-Unterbrechung zurücksetzen
	loopInterrupted = 0;


	unsigned long now = millis();
	byte doUpdateDisplay = 0;

	if (digitalRead(PIN_BTN_RESET) == LOW && (last_btn_reset + DEBOUNCE_BUTTONS) < now) {
		// reset per button ausgelöst
//		prepareInterrupts();
//		startIdle(true);
		start = 0;
		end = 0;
		reset(true);
		last_btn_reset = now;
	}

	if (digitalRead(PIN_BTN_DISPLAY) == LOW && (last_btn_display + DEBOUNCE_BUTTONS) < now) {
		// display umgeschalten
		switchDisplayMode();
		doUpdateDisplay = 1;
		last_btn_display = now;
	}

	if (digitalRead(PIN_BTN_UNIT) == LOW && (last_btn_unit + DEBOUNCE_BUTTONS) < now) {
		// einheit umgeschalten
		switchUnit();
		doUpdateDisplay = 1;
		last_btn_unit = now;
	}

	if ((phase == S_RUNNING || doUpdateDisplay) && loopInterrupted == 0) {
		// Zeit/Geschwindigkeit auf Display aktualisieren

//		Serial.println("call updateDisplay()");

		updateDisplay();
	}

	if (phase == S_DONE && end + DEBOUNCE_SENSOR < now) {
//		prepareInterrupts();
//		startIdle(false);
		reset(false);
	}

	delay(25);
}


// --- functions ---------------------------------------------------------------


void setState(byte s)	// TODO: muss byte statt direkt enum angeben...
{
	digitalWrite(PIN_LED_STATE_IDLE, s == S_IDLE ? HIGH : LOW);
	digitalWrite(PIN_LED_STATE_RUNNING, s == S_RUNNING ? HIGH : LOW);
	digitalWrite(PIN_LED_STATE_DONE, s == S_DONE ? HIGH : LOW);

	Serial.print("STATE: ");
	Serial.println(s);

	phase = s;
}

void switchDisplayMode()
{
	digitalWrite(PIN_LED_MODE_TIME, LOW);
	digitalWrite(PIN_LED_MODE_SPEED, LOW);
	digitalWrite(PIN_LED_MODE_SPEED_SCALED, LOW);

	Serial.print("DISPLAY MODE: ");

	switch (displayMode) {
		case DM_TIME:
			displayMode = DM_SPEED;
			digitalWrite(PIN_LED_MODE_SPEED, HIGH);
			Serial.println("speed");
			break;
		case DM_SPEED:
			displayMode = DM_SPEED_SCALED;
			digitalWrite(PIN_LED_MODE_SPEED_SCALED, HIGH);
			Serial.println("speed scaled");
			break;
		case DM_SPEED_SCALED:
			displayMode = DM_TIME;
			digitalWrite(PIN_LED_MODE_TIME, HIGH);
			Serial.println("time");
			break;
	}
}

void switchUnit()
{
//	unit = (unit == U_MS) ? U_KMH : U_MS;
	digitalWrite(PIN_LED_UNIT_MS, LOW);
	digitalWrite(PIN_LED_UNIT_KMH, LOW);

	Serial.print("UNIT: ");

	if (unit == U_MS) {
		unit = U_KMH;
		digitalWrite(PIN_LED_UNIT_KMH, HIGH);
		Serial.println("km/h");
	} else {
		unit = U_MS;
		digitalWrite(PIN_LED_UNIT_MS, HIGH);
		Serial.println("m/s");
	}
}

float getDuration()
{
//	Serial.println("===== GET DURATION! =====");
//		float duration = (float)(((state == S_RUNNING) ? millis() : end) - start) / 1000;
//		Serial.println(duration);

	if (phase != S_RUNNING && end < start) {
		return 0.0;
	}

	return ((float)((phase == S_RUNNING) ? millis() : end) - start) / 1000.0;
}

float getSpeed()
{
	Serial.println("===== GET SPEED! =====");

	Serial.println(start);
	Serial.println(end);

	float distance = (float)DISTANCE / 100.0;		// m
	float duration = getDuration();					// s

	if (duration <= 0.0) {
		return 0.0;
	}

	if (displayMode == DM_SPEED_SCALED) {
		distance *= SCALE;
	}

	float speed = distance / duration;				// m/s

	if (unit == U_KMH) {
		speed *= 3.6;								// km/h
	}

	Serial.print("  distance: ");
	Serial.println(distance);
	Serial.print("  duration: ");
	Serial.println(duration);
	Serial.print("  speed: ");
	Serial.println(speed);

	return speed;
}

void displayValue(int val)
{
	if (val != displayedValue) {
		tm1637.showNumberDec(val);
		displayedValue = val;
	}
}

void updateDisplay()
{
//	Serial.println("updateDisplay now...");

	if (phase == S_RUNNING || displayMode == DM_TIME) {
		// Zeit anzeigen
		displayTime();
	} else {
		// Geschwindigkeit anzeigen
		displaySpeed();
	}
}

void displayTime()
{
//	Serial.println("display time");
	int duration = (int)round(getDuration() * 10);

//	if (duration <= 0) {
//		return 0;
//	}

//	Serial.println(duration);
	if (duration > 9990) {
//		prepareInterrupts();
//		startIdle(true);
		reset(true);
	} else {
		displayValue(duration);
	}
}

void displaySpeed()
{
	Serial.println("display speed (v2)");

//	int speed = (int)round((getSpeed() * 10));
//	displayValue(speed);

	float speed = getSpeed();
	displayValue((int)round(speed * 10.0));

}

//void prepareInterrupts()
//{
//	Serial.println("=== prepare interrupts ===");
//
//	// clear interrupt flags
//	EIFR = bit(INTF0);
//	EIFR = bit(INTF1);
//
//	// enable interrupts
//	// TODO: was passisert, wenn der Interrupt voer NICHT detached wurde??
//	attachInterrupt(digitalPinToInterrupt(PIN_SENSOR_1), sensorOne, RISING);
//	attachInterrupt(digitalPinToInterrupt(PIN_SENSOR_2), sensorTwo, FALLING);
//}
//
//void startIdle(bool clearDisplay)
//{
//	Serial.println("=== start idling... ===");
//
//	start = 0;
//	end = 0;
//
//	// Status setzen
//	setState(S_IDLE);
//	Serial.println("STATE: ? => IDLE, ");
//
////	displayMode = DM_TIME;
//	// displayMode & unit müssen nicht resettet werden
//
//	// TODO: LEDs aktualisieren?!?!
//
//	if (clearDisplay) {
//		// Display aktualisieren
//		displayValue(0);
//	}
//}

void reset(bool clearDisplay)
{
	Serial.println("=== RESET ===");

	// clear interrupt flags
	EIFR = bit(INTF0);
	EIFR = bit(INTF1);

	// enable interrupts
	// TODO: was passisert, wenn der Interrupt vorher NICHT detached wurde??
	// TODO: RISING oder FALLING, was nun??
	attachInterrupt(digitalPinToInterrupt(PIN_SENSOR_1), sensorOne, FALLING);
	attachInterrupt(digitalPinToInterrupt(PIN_SENSOR_2), sensorTwo, FALLING);

	// Status setzen
	setState(S_IDLE);
	Serial.println("STATE: ? => IDLE, ");

//	displayMode = DM_TIME;
	// displayMode & unit müssen nicht resettet werden

	// TODO: LEDs aktualisieren?!?!

	if (clearDisplay) {
		// Display aktualisieren
		displayValue(0);
	}
}

// ----- ISR functions ---------------------------------------------------------

void sensorOne()
{
	Serial.print("sensor one => ");
	detachInterrupt(digitalPinToInterrupt(PIN_SENSOR_1));
	sensorTriggered();
}
void sensorTwo()
{
	Serial.print("sensor two => ");
	detachInterrupt(digitalPinToInterrupt(PIN_SENSOR_2));
	sensorTriggered();
}

void sensorTriggered()
{
	Serial.print("sensor triggered");

	loopInterrupted = 1;

	switch (phase) {
		case S_IDLE:
			// erster Kontakt
			Serial.print(" (erster Kontakt, ");
			setState(S_RUNNING);
			Serial.print(" STATE IDLE => RUNNING, ");
			start = millis();
			end = 0;
			Serial.println(start);
			break;
		case S_RUNNING:
			// zweiter Kontakt
			Serial.print(" (zweiter Kontakt, ");
			setState(S_DONE);
			Serial.print(" STATE RUNNING => DONE, ");
			end = millis();
			Serial.print(end);
			Serial.println(")");

			if (displayMode != DM_TIME) {
				updateDisplay();
			}

//			getSpeed(false);
//			getSpeed(true);
//			getSpeed();
//			reset();
			break;
		case S_DONE:
			// nothing happens...
			Serial.println("nothing happens ...");
			break;
	}
}
