#include <cassert>
#include <cerrno>
#include <cstring>
#include <vector>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <fcntl.h>
#include <unistd.h>

#include <linux/input-event-codes.h>
#include <linux/uinput.h>

#include "InputDevice.hpp"
#include "DEBUG.hpp"

using namespace std;
using namespace std::filesystem;


// ----------------------------------- [ Structures ] --------------------------------------- //


struct EventHandler {
	filesystem::path path;
	string event;
	int fd = -1;
};


// ----------------------------------- [ Functions ] ---------------------------------------- //

/*
static bool emit(int fd, uint16_t type, uint16_t code, int val){
	struct input_event event = {
		.time = {
			.tv_sec = 0,
			.tv_usec = 0
		},
		.type = type,
		.code = code,
		.value = val,
	};

	return write(fd, &event, sizeof(event)) == sizeof(event);
}


void f(){
	int fd = open("/dev/input/event16", O_WRONLY | O_NONBLOCK);
	if (fd < 0){
		ERROR("err");
		return;
	}
	
	emit(fd, EV_KEY, KEY_NUMLOCK, 1);
	emit(fd, EV_SYN, SYN_REPORT, 0);
	emit(fd, EV_KEY, KEY_NUMLOCK, 0);
	emit(fd, EV_SYN, SYN_REPORT, 0);
	
	close(fd);
}
*/

// ----------------------------------- [ Functions ] ---------------------------------------- //


void closeEventHandlers(vector<EventHandler>& v){
	for (EventHandler& ev : v){
		if (ev.fd >= 0){
			close(ev.fd);
			ev.fd = -1;
		}
	}
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


vector<EventHandler> getNumlockDevices(){
	vector<EventHandler> devices = {};
	int err_count = 0;
	
	for (const directory_entry& e : directory_iterator("/dev/input/")){
		const filesystem::path& handler_path = e.path();
		
		// Filter for `event*`
		if (e.is_directory()){
			continue;
		} if (!handler_path.filename().string().starts_with("event")){
			continue;
		}
		
		// Open event device and check for permission errors.
		const int fd = open(handler_path.c_str(), O_WRONLY | O_NONBLOCK);
		if (fd < 0){
			if (errno == EACCES)
				err_count++;
			continue;
		}
		
		// Check for numlock
		if (!InputDevice::supportsEventType(fd, EV_KEY)){
			close(fd);
			continue;
		} else if (!InputDevice::supportsKeyEvent(fd, KEY_NUMLOCK)){
			close(fd);
			continue;
		}
		
		EventHandler& ev = devices.emplace_back();
		ev.path = move(handler_path);
		ev.event = ev.path.filename();
		ev.fd = fd;
	}
	
	if (err_count > 0){
		if (devices.size() <= 0)
			WARNING_L0("Failed to open and check a few (%d) event handlers from '/dev/input/' due to lack of permissions.", err_count);
		else
			WARNING_L0("Permission denied when opening event handlers at '/dev/input/'.", err_count);
	} else if (devices.size() <= 0){
		WARNING_L0("Missing event handlers at '/dev/input/'.", err_count);
	}
	
	return devices;
}


// --------------------------------- [ Main Function ] -------------------------------------- //


int main(int argc, char const* const* argv){
	vector<EventHandler> devs = getNumlockDevices();
	
	sort(devs.begin(), devs.end(), [](EventHandler& a, EventHandler& b){
		return strcmp(a.event.c_str(), b.event.c_str());
	});
	
	for (EventHandler& dev : devs){
		string name = InputDevice::getName(dev.fd);
		printf("%s (%s):", dev.event.c_str(), dev.path.c_str());
		printf("  %s\n", name.c_str());
	}
	
	closeEventHandlers(devs);
	return 0;
}


// ------------------------------------------------------------------------------------------ //
