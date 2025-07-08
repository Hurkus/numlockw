#include "InputDevice.hpp"
#include <linux/uinput.h>

using namespace std;


// ----------------------------------- [ Functions ] ---------------------------------------- //


inline bool isSet(const uint8_t* bitfield, uint32_t bit){
	const uint32_t byte = bit / 8;
	const uint32_t _bit = bit % 8;
	return (bitfield[byte] & (1 << bit)) != 0;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


bool InputDevice::supportsEventType(int fd, uint32_t eventId){
	uint8_t bitfield[EV_MAX/8 + 1] = {0};
	
	const int n = ioctl(fd, EVIOCGBIT(0, sizeof(bitfield)), bitfield);
	if (n <= 0){
		return false;
	}
	
	return isSet(bitfield, eventId);
}


bool InputDevice::supportsKeyEvent(int fd, uint32_t keyId){
	if (keyId > KEY_MAX){
		return false;
	}
	
	uint8_t bitfield[keyId/8 + 1] = {0};
	
	const int n = ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(bitfield)), bitfield);
	if (n <= 0){
		return false;
	}
	
	return isSet(bitfield, keyId);
}


bool InputDevice::supportsLED(int fd, uint32_t ledId){
	if (ledId > LED_MAX){
		return false;
	}
	
	uint8_t bitfield[ledId/8 + 1] = {0};
	
	const int n = ioctl(fd, EVIOCGBIT(EV_LED, sizeof(bitfield)), bitfield);
	if (n <= 0){
		return false;
	}
	
	return isSet(bitfield, ledId);
}


bool InputDevice::getLED(int fd, uint32_t ledId){
	if (ledId > LED_MAX){
		return false;
	}
	
	uint8_t bitfield[ledId/8 + 1] = {0};
	
	const int n = ioctl(fd, EVIOCGLED(sizeof(bitfield)), bitfield);
	if (n <= 0){
		return false;
	}
	
	return isSet(bitfield, ledId);
}


// ----------------------------------- [ Functions ] ---------------------------------------- //

string InputDevice::getName(int fd){
	string name;
	name.resize(64);
	constexpr int MAX_LEN = 2048;
	
	while (true){
		const int len = ioctl(fd, EVIOCGNAME(name.size()), name.data());
		
		if (len < name.size() || name.size() >= MAX_LEN){
			name.resize(len);
			break;
		} else {
			name.resize(name.size() * 2);
		}
		
	}
	
	return name;
}


// ------------------------------------------------------------------------------------------ //