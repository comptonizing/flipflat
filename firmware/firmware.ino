
#include "CommandBuffer.h"
#include <ArduinoJson.h>

#include "Flap.h"

#define MSG_PREFIX '#'
#define MSG_POSTFIX '$'

#define BUFFSIZE 128


uint16_t crcCalc(const char *str) {
  uint16_t crc = 0;
  while ( *str != '\0' ) {
    crc = _crc16_update(crc, *str);
    str++;
  }
  return crc;
}

void sendMessage(char *msg) {
  uint16_t crc = crcCalc(msg);
  char crcMsg[3];
  crcMsg[0] = ((char *) &crc)[0];
  crcMsg[1] = ((char *) &crc)[1];
  crcMsg[2] = '\0';
  // Must handle '$' which is end of message character
  if ( crcMsg[0] == '$' ) {
    crcMsg[0] = '1';
  }
  if ( crcMsg[1] == '$' ) {
    crcMsg[1] = '1';
  }
  Serial.print(MSG_PREFIX);
  Serial.print(msg);
  Serial.print('\0');
  Serial.print(crcMsg[0]);
  Serial.print(crcMsg[1]);
  Serial.print(MSG_POSTFIX);
}

void sendErrorMessage(const __FlashStringHelper *msg) {
  char buff[128];
  DynamicJsonDocument json(128);
  json[F("Error")] = msg;
  serializeJson(json, buff, 128);
  sendMessage(buff);
}

void sendStatus() {
  char buff[BUFFSIZE];
  Flap::i().state(buff, BUFFSIZE);
  sendMessage(buff);
}

bool runStatus(const char *cmd) {
  if ( strcmp_P(cmd, F("status") ) ) {
    return false;
  }
  sendStatus();
  return true;
}

bool runOpen(const char *cmd) {
  if ( strcmp_P(cmd, F("open") ) ) {
    return false;
  }
  Flap::i().open();
  sendStatus();
  return true;
}

bool runClose(const char *cmd) {
  if ( strcmp_P(cmd, F("close")) ) {
    return false;
  }
  Flap::i().close();
  sendStatus();
  return true;
}

bool runOn(const char *cmd) {
  if ( strcmp_P(cmd, F("on")) ) {
    return false;
  }
  Flap::i().on();
  sendStatus();
  return true;
}

bool runOff(const char *cmd) {
  if ( strcmp_P(cmd, F("off")) ) {
    return false;
  }
  Flap::i().off();
  sendStatus();
  return false;
}

bool runCommand(const char *cmd) {
  if ( runStatus(cmd) ) {
    return true;
  }
  if ( runOpen(cmd) ) {
    return true;
  }
  if ( runClose(cmd) ) {
    return true;
  }
  if ( runOn(cmd) ) {
    return true;
  }
  if ( runOff(cmd) ) {
    return true;
  }
  sendErrorMessage(F("Unknown command"));
  return false;
}

void setup() {
  Flap::i().update();
  Flap::i().close();
  Serial.begin(9600);
  Serial.setTimeout(1000);
}

void loop() {
  static bool inCommand = false;
  Flap::i().update();
  while ( Serial.available() ) {
    char c = Serial.read();
    switch (c) {
      case MSG_PREFIX:
	inCommand = true;
	CommandBuffer::i().clear();
	CommandBuffer::i().add(c);
	break;
      case MSG_POSTFIX:
	if ( inCommand ) {
	  inCommand = false;
	  if ( ! CommandBuffer::i().add(c) ) {
	    CommandBuffer::i().clear();
	    break;
	  }
	  if ( CommandBuffer::i().verifyChecksum() ) {
	    runCommand(CommandBuffer::i().getCommand());
	  } else {
	    sendErrorMessage(F("Checksum error"));
	  }
	  CommandBuffer::i().clear();
	}
        break;
      default:
	if ( inCommand ) {
	  if ( ! CommandBuffer::i().add(c) ) {
	    CommandBuffer::i().clear();
	    inCommand = false;
	  }
	}
    }
  }
}
