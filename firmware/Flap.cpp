#include "Flap.h"

Flap::Flap() {
	pinMode(m_flatPin, OUTPUT);
	digitalWrite(m_flatPin, LOW);
	m_panelState = OFF;
  m_servo.setDetach(false);
	m_servo.setPin(m_servoPin);
	m_servo.setSpeed(10.0);
	m_servo.setMinMax(700, 1500);
  m_servo.setInitialPosition(1.0); // closed
	m_flapState = CLOSED;
  open(); // Please don't ask me why, but I have to put an open here in order to get it closed.
  m_servo.update();
}

void Flap::update() {
	m_servo.update();
	if ( m_servo.isStopped() ) {
		if ( m_flapState == OPENING ) {
			m_flapState = OPEN;
		}
		if ( m_flapState == CLOSING ) {
			m_flapState = CLOSED;
		}
	}
}

void Flap::on() {
	digitalWrite(m_flatPin, HIGH);
	m_panelState = ON;
}

void Flap::off() {
	digitalWrite(m_flatPin, LOW);
	m_panelState = OFF;
}

void Flap::open() {
	if ( m_flapState == OPEN ) {
		return;
	}
	m_servo.goTo(0.0);
	m_flapState = OPENING;
}

void Flap::close() {
	if ( m_flapState == CLOSED ) {
		return;
	}
	m_servo.goTo(1.0);
	m_flapState = CLOSING;
}

Flap &Flap::i() {
	static Flap instance;
	return instance;
}

void Flap::state(char *buff, size_t n) {
	StaticJsonDocument<64> json;
	switch ( m_flapState ) {
		case CLOSED:
			json["F"] = "closed";
			break;
		case OPEN:
			json["F"] = "open";
			break;
		case CLOSING:
			json["F"] = "closing";
			break;
		case OPENING:
			json["F"] = "opening";
			break;
	}
	json["P"] = m_panelState == ON ? "on" : "off";
	serializeJson(json, buff, n);
}
