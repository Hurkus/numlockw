#include <cassert>
#include <vector>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <filesystem>

#include <linux/input-event-codes.h>

#include "DEBUG.hpp"

using namespace std;
using namespace std::filesystem;


// ----------------------------------- [ Structures ] --------------------------------------- //


struct Device {
	filesystem::path path;
	string name;
	string description;
};


// ----------------------------------- [ Functions ] ---------------------------------------- //


constexpr bool isHex(char c){
	return ('0' <= c && c <= '9') || ('a' <= c && c <= 'f') || ('A' <= c && c <= 'F');
}


constexpr uint8_t unhex(char c){
	if ('0' <= c && c <= '9')
		return c - '0';
	else if ('a' <= c && c <= 'f')
		return 10 + c - 'a';
	else if ('A' <= c && c <= 'F')
		return 10 + c - 'A';
	else
		return UINT8_MAX;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


/**
 * @brief Extract capability bit from bit vector.
 * @param capabilites Bit field of capabilities.
 * @param eventCode Event code defined in `linux/input-event-codes.h`. Eg: `KEY_A`.
 */
static bool isCapable(const vector<uint64_t>& capabilites, uint32_t eventCode){
	const uint32_t w = eventCode / 64;
	const uint32_t b = eventCode % 64;
	
	if (w < capabilites.size())
		return (capabilites[w] & (1 << b)) != 0;
	else
		return false;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


static string getDeviceDescription(const string& name){
	assert(name.length() > 0);
	ifstream in = ifstream(path("/sys/class/input/") / name / "device/name");
	string line;
	getline(in, line);
	return line;
}


/**
 * @brief Decode capabilites file of an input device.
 * @note File structure: https://gist.github.com/TriceHelix/de47ed38dcb4f7216b26291c47445d99
 * @param name Name of the device at `/sys/class/input/<name>`
 * @param capability Capabilities file in `/sys/class/input/<name>/device/capabilities/<capability>`: `ev`, `key`, `led`, `abs`, `ff`, `msc`, `rel`, `snd`, `sw`, ...
 * @return Bit field of capabilities. Querryable with enums from `linux/input-event-codes.h` or `isCapable(...)`.
 */
static vector<uint64_t> getDeviceCapabilities(const string& name, const char* capability){
	assert(capability != nullptr);
	assert(name.length() > 0);
	
	const filesystem::path dev_path = path("/sys/class/input/") / name / "device/capabilities/" / capability;
	ifstream in = ifstream(dev_path);
	
	vector<uint64_t> bits = {};
	uint64_t hex_group = 0;
	int bit_count = 0;
	
	// Example: 120013 -> 1100'1000'0000'0000'0100'1000'0000'0000'0000'0000'0000'0000'0000'0000'0000'0000
	// Example: f 2    -> 0100'0000'0000'0000'0000'0000'0000'0000'0000'0000'0000'0000'0000'0000'0000'0000 1111
	
	char c;
	while (in.get(c)){
		
		if (c == '\n'){
			continue;
		}
		
		// Padded with zero bits.
		else if (isspace(c)){
			bits.push_back(hex_group);
			hex_group = 0;
			bit_count = 0;
		}
		
		// Append hex bits.
		else if (isHex(c)){
			const uint8_t hx = unhex(c);
			
			if (bit_count >= 64){
				bits.push_back(hex_group);
				hex_group = 0;
				bit_count = 0;
			}
			
			hex_group = (hex_group << 4) | (uint64_t(hx) & 0b1111);
			bit_count += 4;
		}
		
		else {
			ERROR("Failed to parse device capabilities of device '%s'.", dev_path.c_str());
			bits.clear();
			return bits;
		}
		
	}
	
	// Push remaining hex group.
	if (bit_count > 0){
		bits.push_back(hex_group);
		hex_group = 0;
		bit_count = 0;
	}
	
	reverse(bits.begin(), bits.end());
	
	// // DEBUG print
	// for (int i = 0 ; i < bits.size() ; i++){
	// 	if (i > 0)
	// 		cout << " ";
	// 	for (int ii = 0 ; ii < 64 ; ii++)
	// 		cout << ((bits[i] >> ii) & 1);
	// }
	// cout << endl;
	
	return bits;
}


static void getEventDevices(vector<Device>& devs){
	for (const directory_entry& e : directory_iterator("/dev/input/")){
		if (e.is_directory()){
			continue;
		} if (!e.path().filename().string().starts_with("event")){
			continue;
		}
		
		Device& dev = devs.emplace_back();
		dev.path = e.path();
		dev.name = e.path().filename();
		dev.description = getDeviceDescription(dev.name);
	}
}


// --------------------------------- [ Main Function ] -------------------------------------- //


int main(int argc, char const* const* argv){
	vector<Device> devs = {};
	devs.reserve(64);
	
	getEventDevices(devs);
	
	// for (Device& dev : devs){
	// 	printf("%s:\n", dev.path.c_str());
	// 	printf("    name: %s\n", dev.name.c_str());
	// }
	
	for (Device& dev : devs){
		vector<uint64_t> cap = getDeviceCapabilities(dev.name, "key");
		
		if (isCapable(cap, KEY_NUMLOCK)){
			printf("%s:\n", dev.path.c_str());
			printf("    name: %s\n", dev.description.c_str());
			printf("    numlock: %d\n", true);
		}
		
	}
	
	// vector<uint64_t> cap = getDeviceCapabilities("event16", "key");
	// cout << "KEY_NUMLOCK: ";
	// cout << isCapable(cap, KEY_NUMLOCK);
	// cout << endl;
	
	return 0;
}


// ------------------------------------------------------------------------------------------ //
