#pragma once

#include <inttypes.h>
#include <stddef.h>
#include <ArduinoJson.h>

// #include <Servo.h>
#include <SlowMotionServo.h>

class Flap {
	public:
		static Flap &i();
		Flap();
		void update();
		void open();
		void close();
		void on();
		void off();
		void state(char *buff, size_t n);

		enum FlapState {
			CLOSED,
			OPEN,
			CLOSING,
			OPENING
		};

		enum PanelState {
			OFF,
			ON
		};
	private:
		uint8_t m_servoPin = 7;
		uint8_t m_flatPin = 4;
		SMSSmooth m_servo;
		FlapState m_flapState = CLOSED;
		PanelState m_panelState = OFF;
};
