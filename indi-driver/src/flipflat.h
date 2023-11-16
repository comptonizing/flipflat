#pragma once

#define CMDBUFF 1024
#define ERRBUFF 256
#define RSPBUFF 1024

#define TIMEOUT 2

#include <memory>
#include <termios.h>

#include "json.hpp"

using namespace nlohmann;

#include <defaultdevice.h>
#include <connectionplugins/connectionserial.h>
#include <indilightboxinterface.h>
#include <indidustcapinterface.h>

namespace Connection {
	class Serial;
}

class FlipFlat : public INDI::DefaultDevice, public INDI::LightBoxInterface, public INDI::DustCapInterface {
	public:
		FlipFlat();
		bool initProperties() override;
	protected:
		const char *getDefaultName() override;
		virtual IPState ParkCap() override;
		virtual IPState UnParkCap() override;
		virtual bool EnableLightBox(bool enable) override;
		void ISGetProperties(const char *dev) override;
		virtual bool updateProperties() override;
		virtual bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n) override;
		virtual bool ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n) override;
		virtual bool ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n) override;
		virtual bool ISSnoopDevice(XMLEle *root) override;
		virtual bool saveConfigItems(FILE *fp) override;
		virtual void TimerHit() override;
	private:
		bool sendCommand(const char *cmd, char *rsp);
		uint16_t crc16_update(uint16_t crc, uint8_t a);
		uint16_t crcCalc(const void *data, size_t n);
		uint16_t crcCalc(const char *str);
		void cmdCrc(const char *cmd, char *out);
		bool checkCrc(const char *rsp);
		const json& getJSON();
		bool Handshake();
		Connection::Serial *serialConnection{ nullptr };
		bool updateStatus();
		bool updateFromResponse(const char *rsp);
};
