#include "flipflat.h"

static std::unique_ptr<FlipFlat> flipflat(new FlipFlat());

const char *FlipFlat::getDefaultName() {
	return "FlipFlat";
}


FlipFlat::FlipFlat() : LightBoxInterface(this), DustCapInterface(this) {
	setDefaultPollingPeriod(500);
	setVersion(0, 1);
}

void FlipFlat::cmdCrc(const char *cmd, char *out) {
  uint16_t crc = crcCalc(cmd);
  int len = strlen(cmd);
  memcpy(out+1, cmd, len);
  out[0] = '#';
  out[len+1] = '\0';
  out[len+2] = ((char *) &crc)[0];
  if ( out[len+2] == '$' ) {
    out[len+2] = '1';
  }
  out[len+3] = ((char *) &crc)[1];
  if ( out[len+3] == '$' ) {
    out[len+3] = '1';
  }
  out[len+4] = '$';
  out[len+5] = '\0';
}

bool FlipFlat::checkCrc(const char *rsp) {
  uint16_t crcGotten;
  size_t len = strlen(rsp);
  ((char *) &crcGotten)[0] = rsp[len+1];
  ((char *) &crcGotten)[1] = rsp[len+2];
  uint16_t crcCalculated = crcCalc((const void *) rsp, len);
  char *crcChar = (char *) &crcCalculated;
  if ( crcChar[0] == '$' ) {
    crcChar[0] = '1';
  }
  if ( crcChar[1] == '$' ) {
    crcChar[1] = '1';
  }
  if ( crcGotten == crcCalculated ) {
    return true;
  } else {
    LOG_ERROR("Checksum error");
    LOGF_ERROR("Message: %s", rsp);
    LOGF_ERROR("Checksum: %d (0x%02x 0x%02x), expected %d (0x%02x 0x%02x)",
    crcCalculated, ((unsigned char *) &crcCalculated)[0], ((unsigned char *) &crcCalculated)[1],
    crcGotten, ((unsigned char *) &crcGotten)[0], ((unsigned char *) &crcGotten)[1]);
    return false;
  }
}

bool FlipFlat::sendCommand(const char *cmd, char *rsp) {
  LOGF_DEBUG("Sending command: %s", cmd);
  int nbytes_written = 0, nbytes_read = 0, rc = -1;
  int PortFD = serialConnection->getPortFD();
  LOGF_DEBUG("PortFD: %d", PortFD);
  char buff[CMDBUFF];
  char err[ERRBUFF];
  char rspBuff[RSPBUFF];
  cmdCrc(cmd, buff);

  tcflush(PortFD, TCIOFLUSH);
  if ( (rc = tty_write(PortFD, buff, strlen(buff)+4, &nbytes_written)) != TTY_OK ) {
    tty_error_msg(rc, err, ERRBUFF);
    LOGF_ERROR("Error writing command %s: %s", cmd, err);
    return false;
  }

  LOG_DEBUG("RCV");

  // Somtimes there's garbage on the line so read until the next #
  rspBuff[0] = '\0';
  while ( rspBuff[0] != '#' ) {
    if ( (rc = tty_read(PortFD, rspBuff, 1, TIMEOUT, &nbytes_read) ) != TTY_OK ) {
      tty_error_msg(rc, err, ERRBUFF);
      LOGF_ERROR("Error reading response: %s", err);
      return false;
    }
  }

  if ( (rc = tty_read_section(PortFD, rspBuff+1, '$', TIMEOUT, &nbytes_read)) != TTY_OK ) {
    tty_error_msg(rc, err, ERRBUFF);
    LOGF_ERROR("Error reading response: %s", err);
    return false;
  }

  memcpy(rsp, rspBuff+1, strlen(rspBuff)+2);
  if ( ! checkCrc(rsp) ) {
    return false;
  }
  return true;
}

bool FlipFlat::initProperties() {
	INDI::DefaultDevice::initProperties();
	DI::initProperties(MAIN_CONTROL_TAB);
	LI::initProperties(MAIN_CONTROL_TAB, 0);

	setDriverInterface(AUX_INTERFACE | LIGHTBOX_INTERFACE | DUSTCAP_INTERFACE);
	addAuxControls();
	serialConnection = new Connection::Serial(this);
	serialConnection->registerHandshake([&]() {
			return Handshake();
			});
	registerConnection(serialConnection);
	return true;
}

bool FlipFlat::Handshake() {
	updateStatus();
    SetTimer(getCurrentPollingPeriod());
    return true;
}


uint16_t FlipFlat::crc16_update(uint16_t crc, uint8_t a) {
  // Code from crc16.h from the Arduino core tools
  crc ^= a;
  for (int ii=0; ii<8; ii++) {
    if ( crc & 1 ) {
      crc = (crc >> 1) ^ 0xA001;
    } else {
      crc = (crc >> 1);
    }
  }
  return crc;
}

uint16_t FlipFlat::crcCalc(const void *data, size_t n) {
  const uint8_t *ptr = static_cast<const uint8_t *>(data);
  uint16_t crc = 0;
  for (size_t ii=0; ii<n; ii++) {
    crc = crc16_update(crc, ptr[ii]);
  }
  return crc;
}

uint16_t FlipFlat::crcCalc(const char *str) {
  return crcCalc(static_cast<const void *>(str), strlen(str));
}

void FlipFlat::ISGetProperties(const char *dev) {
	INDI::DefaultDevice::ISGetProperties(dev);
	LI::ISGetProperties(dev);
}

bool FlipFlat::updateProperties() {
	INDI::DefaultDevice::updateProperties();
	if ( isConnected() ) {
		DI::updateProperties();
		LI::updateProperties();
	} else {
		DI::updateProperties();
		LI::updateProperties();
	}
	return true;
}

bool FlipFlat::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n) {
	if (dev != nullptr && strcmp(dev, getDeviceName()) == 0) {
		if (DI::processSwitch(dev, name, states, names, n)) {
			return true;
		}
		if (LI::processSwitch(dev, name, states, names, n)) {
			return false;
		}
	}
	return INDI::DefaultDevice::ISNewSwitch(dev, name, states, names, n);
}

bool FlipFlat::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n) {
	if (LI::processNumber(dev, name, values, names, n)) {
		return true;
	}
	return INDI::DefaultDevice::ISNewNumber(dev, name, values, names, n);
}

bool FlipFlat::ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n) {
	if ( dev != nullptr && strcmp(dev, getDeviceName()) == 0 ) {
		if (LI::processText(dev, name, texts, names, n)) {
			return true;
		}
	}
	return INDI::DefaultDevice::ISNewText(dev, name, texts, names, n);
}

bool FlipFlat::ISSnoopDevice(XMLEle *root) {
	LI::snoop(root);
	return INDI::DefaultDevice::ISSnoopDevice(root);
}

bool FlipFlat::FlipFlat::saveConfigItems(FILE *fp) {
	INDI::DefaultDevice::saveConfigItems(fp);
	return LI::saveConfigItems(fp);
}

IPState FlipFlat::ParkCap() {
	if ( strcmp(previousFlapStatus, "closed") == 0 ) {
		return IPS_OK;
	}
	char rsp[RSPBUFF];
	if ( ! sendCommand("close", rsp) || ! updateFromResponse(rsp) ) {
		return IPS_ALERT;
	}
	return strcmp(previousFlapStatus, "closed") == 0 ? IPS_OK : IPS_BUSY; // if already parked return IPS_OK
}

IPState FlipFlat::UnParkCap() {
	if ( strcmp(previousFlapStatus, "open") == 0 ) {
		return IPS_OK;
	}
	char rsp[RSPBUFF];
	if ( ! updateStatus() ) {
		return IPS_ALERT;
	}
	if ( strcmp(previousPanelStatus, "on") == 0 ) {
		LOG_WARN("Opening cover, but panel switch on, switching off");
		if ( ! EnableLightBox(false) ) {
			return IPS_ALERT;
		}
	}

	if ( ! sendCommand("open", rsp) || ! updateFromResponse(rsp) ) {
		return IPS_ALERT;
	}
	return strcmp(previousFlapStatus, "open") == 0 ? IPS_OK : IPS_BUSY; // if already unparked return IPS_OK
}

bool FlipFlat::EnableLightBox(bool enable) {
	updateStatus();
	if ( enable ) {
		if ( ParkCapSP[1].getState() == ISS_ON ) {
			LOG_ERROR("Can't switch on flat unless cover is closed!");
			return false;
		}
	}
	char cmd[CMDBUFF];
	char rsp[RSPBUFF];
	if ( enable ) {
		strcpy(cmd, "on");
	} else {
		strcpy(cmd, "off");
	}
	if ( ! sendCommand(cmd, rsp) || ! updateFromResponse(rsp) ) {
		return false;
	}
	return true;
}

bool FlipFlat::updateStatus() {
	char rsp[RSPBUFF];
	if ( ! sendCommand("status", rsp) ) {
		return false;
	}
	return updateFromResponse(rsp);
}

bool FlipFlat::updateFromResponse(const char *rsp) {
	LOGF_DEBUG("RSP: %s", rsp);
	json data;
	try {
		data = json::parse(rsp);
	} catch (...) {
		LOG_ERROR("JSON parse error");
		return false;
	}
    if ( data.contains("Error") ) {
        LOGF_ERROR("Device error: %s", data["Error"].template get<std::string>().c_str());
        return false;
    }
	if ( ! data.contains("F") or ! data.contains("P") ) {
		LOGF_ERROR("Wrong fields in JSON, wrong device? (%s)", rsp);
		return false;
	}
	std::string panelStatus = data["P"].template get<std::string>();
	std::string flapStatus = data["F"].template get<std::string>();


	if ( panelStatus != previousPanelStatus ) {
	    if ( panelStatus == "off" ) {
			LightSP.reset();
	    	LightSP[FLAT_LIGHT_ON].setState(ISS_OFF);
	    	LightSP[FLAT_LIGHT_OFF].setState(ISS_ON);
	    	LightSP.apply();
			LOG_INFO("Light off");
	    } else if ( panelStatus == "on" ) {
			LightSP.reset();
	    	LightSP[FLAT_LIGHT_ON].setState(ISS_ON);
	    	LightSP[FLAT_LIGHT_OFF].setState(ISS_OFF);
	    	LightSP.apply();
			LOG_INFO("Light on");
	    } else {
	    	LOGF_ERROR("Unknown flat status: %s", panelStatus.c_str());
	    	return false;
	    }
		strncpy(previousPanelStatus, panelStatus.c_str(), sizeof(previousPanelStatus)-1);
	}

	if ( flapStatus != previousFlapStatus ) {
	    if ( flapStatus == "closed" ) {
	    	 ParkCapSP.reset();
	    	 ParkCapSP[CAP_PARK].setState(ISS_ON);
	    	 ParkCapSP.setState(IPS_OK);
	    	 ParkCapSP.apply();
	    	 LOG_INFO("Cover closed");
	    } else if ( flapStatus == "open" ) {
	    	 ParkCapSP.reset();
	    	 ParkCapSP[CAP_UNPARK].setState(ISS_ON);
	    	 ParkCapSP.setState(IPS_OK);
	    	 ParkCapSP.apply();
	    	 LOG_INFO("Cover opened");
	    } else if ( flapStatus == "opening" || flapStatus == "closing" ) {
	    	 ParkCapSP.reset();
	    	 ParkCapSP.setState(IPS_BUSY);
	    	 ParkCapSP.apply();
	    	 LOG_INFO("Cover moving");
	    } else {
	    	LOGF_ERROR("Unknown cover status: %s", flapStatus.c_str());
	    	return false;
	    }
		strncpy(previousFlapStatus, flapStatus.c_str(), sizeof(previousFlapStatus)-1);
	}

    return true;
}

void FlipFlat::TimerHit() {
	if ( ! isConnected() ) {
		return;
	}
	updateStatus();
	SetTimer(getCurrentPollingPeriod());
}
