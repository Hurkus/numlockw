#include <vector>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <filesystem>

#include <linux/input-event-codes.h>

#include "DEBUG.hpp"

// https://gist.github.com/TriceHelix/de47ed38dcb4f7216b26291c47445d99

using namespace std;
using namespace std::filesystem;


// ----------------------------------- [ Functions ] ---------------------------------------- //


struct Device {
	filesystem::path path;
	string name;
};


// ----------------------------------- [ Functions ] ---------------------------------------- //


/**
 * @brief File sizes are larger than the content.
 *        Line by line reading required.
 */
static bool readFile(const filesystem::path& path, string& buff){
	ifstream in = ifstream(path);
	bool e = false;
	
	string line;
	while (getline(in, line)){
		if (e)
			buff.push_back('\n');
		e = true;
		buff.append(line);
	}
	
	return e;
}


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


static string getDeviceName(const string& name){
	ifstream in = ifstream(path("/sys/class/input/") / name / "device/name");
	string line;
	getline(in, line);
	return line;
}


static bool getDeviceCapabilities(const string& name, const char* capability){
	const filesystem::path dev_path = path("/sys/class/input/") / name / "device/capabilities/key";
	
	// Read in reverse and decode hex.
	// Example: 120013 -> 1100'1000'0000'0000'0100'1000
	// Example: f 2 -> 0100'0000'0000'0000'0000'0000'0000'0000'0000'0000'0000'0000'0000'0000'0000'0000 1111
	
	string data;
	readFile(dev_path, data);
	// data = "120013";	// DEBUG
	// data = "f 7";	// DEBUG
	// data = "33eff 0 0 483ffff17aff32d bfd4444600000000 1 130c730b17c007 ffbf7bfad9415fff febeffdfffefffff fffffffffffffffe";	// DEBUG
	
	vector<bool> bits = {};
	
	int b = 0;
	for (int i = int(data.length()) - 1 ; i >= 0 ; i--){
		const char c = data[i];
		
		// Ignore newline.
		if (c == '\n'){
			continue;
		}
		
		// Pad 4*(16-b) false bits.
		else if (isspace(c)){
			for (int ii = 4*(16-b) ; ii > 0 ; ii--)
				bits.push_back(0);
			b = 0;
		}
		
		// Append hex bits in reverse order.
		else if (isHex(c)){
			uint8_t h = unhex(c);
			bits.push_back((h >> 0) & 1);
			bits.push_back((h >> 1) & 1);
			bits.push_back((h >> 2) & 1);
			bits.push_back((h >> 3) & 1);
			b += 1;
		}
		
		else {
			ERROR("Failed to parse device capabilities of device '%s'.", dev_path.c_str());
			return false;
		}
		
	}
	
	cout << data << endl;
	for (int i = 0 ; i < bits.size() ; i++){
		cout << bits[i];
	}
	cout << endl;
	
	
	cout << "KEY_NUMLOCK: ";
	cout << (KEY_NUMLOCK < bits.size() ? bits[KEY_NUMLOCK] : 0);
	cout << endl;
	
	return true;
}


static void getEventDevices(vector<Device>& devs){
	for (const directory_entry& e : directory_iterator("/dev/input/")){
		if (e.is_directory()){
			continue;
		} if (!e.path().filename().string().starts_with("event")){
			continue;
		}
		
		const string devname = e.path().filename();
		
		Device& dev = devs.emplace_back();
		dev.path = e.path();
		dev.name = getDeviceName(devname);
	}
}


// --------------------------------- [ Main Function ] -------------------------------------- //


int main(int argc, char const* const* argv){
	vector<Device> devs = {};
	devs.reserve(64);
	
	getEventDevices(devs);
	
	for (Device& dev : devs){
		printf("%s:\n", dev.path.c_str());
		printf("    name: %s\n", dev.name.c_str());
	}
	
	// for (int i = int(devs.size()) - 1 ; i >= 0 ; i--){
	// 	if (devs[i].name.find("eyboard") == string::npos)
	// 		devs.erase(devs.begin() + i);
	// }
	
	// for (Device& dev : devs){
	// 	printf("%s:\n", dev.path.c_str());
	// 	printf("    name: %s\n", dev.name.c_str());
	// }
	
	// getDeviceCapabilities("event16", "key");
	return 0;
}


// ------------------------------------------------------------------------------------------ //
