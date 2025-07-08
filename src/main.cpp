#include <cassert>
#include <cerrno>
#include <cstring>
#include <string_view>
#include <vector>
#include <algorithm>
#include <filesystem>

#include <fcntl.h>
#include <unistd.h>
#include <linux/uinput.h>

#include "InputDevice.hpp"
#include "DEBUG.hpp"

using namespace std;


// ----------------------------------- [ Variables ] ---------------------------------------- //


constexpr int KEYPRESS_WAIT_MS = 500;


enum class Operation : char {
	NONE,
	OFF,
	ON,
	TOGGLE
};

struct {
	bool help = false;
	bool list = false;
	Operation operation = Operation::NONE;
	vector<string> eventHandlers;
} options;


// ----------------------------------- [ Structures ] --------------------------------------- //


struct EventHandler {
	filesystem::path path;
	string event;
	int fd = -1;
};


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


static bool supportsNumlock(int fd){
	assert(fd >= 0);
	return InputDevice::supportsEventType(fd, EV_KEY) && InputDevice::supportsKeyEvent(fd, KEY_NUMLOCK);
}


vector<EventHandler> loadNumlockDevices(){
	vector<EventHandler> devices = {};
	int err_count = 0;
	
	for (const filesystem::directory_entry& e : filesystem::directory_iterator("/dev/input/")){
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
		
		if (!supportsNumlock(fd)){
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
			WARNING("Failed to open and check a few (%d) event handlers from '/dev/input/' due to lack of permissions.", err_count);
		else
			WARNING("Permission denied when opening event handlers at '/dev/input/'.", err_count);
	} else if (devices.size() <= 0){
		WARNING("Missing event handlers at '/dev/input/'.", err_count);
	}
	
	return devices;
}


vector<EventHandler> loadNumlockDevices(const vector<string>& eventHandlers){
	vector<EventHandler> devices = {};
	
	for (const string& ev_name : eventHandlers){
		filesystem::path handler_path = filesystem::path("/dev/input") / ev_name;
		
		if (!filesystem::exists(handler_path)){
			WARNING("Event handler '%s' not found.", handler_path.c_str());
			continue;
		}
		
		// Open event device and check for permission errors.
		const int fd = open(handler_path.c_str(), O_WRONLY | O_NONBLOCK);
		if (fd < 0){
			if (errno == EACCES)
				WARNING("Permission denied when opening event handler '%s'.", handler_path.c_str());
			else
				WARNING("Failed to open event handler '%s'.", handler_path.c_str());
			continue;
		}
		
		if (!supportsNumlock(fd)){
			close(fd);
			WARNING("Event handler '%s' does not support numlock.", handler_path.c_str());
			continue;
		}
		
		EventHandler& ev = devices.emplace_back();
		ev.path = move(handler_path);
		ev.event = ev.path.filename();
		ev.fd = fd;
	}
	
	return devices;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


static void setNumlock(const vector<EventHandler>& events, bool state){
	for (const EventHandler& ev : events){
		assert(ev.fd >= 0);
		
		if (!InputDevice::supportsLED(ev.fd, LED_NUML)){
			WARNING("Failed to determine numlock state of event handler '%s' because it doesn't support an LED.", ev.event.c_str());
			continue;
		}
		
		const bool led = InputDevice::getLED(ev.fd, LED_NUML);
		if (led != state){
			INFO("Toggle %s", ev.path.c_str());
			
			if (!InputDevice::toggleNumlock(ev.fd)){
				ERROR("Failed to set numlock state of event handler '%s'.", ev.event.c_str());
			}
			
			usleep(KEYPRESS_WAIT_MS * 1000);
		} else {
			INFO("Skip   %s", ev.path.c_str());
		}
		
	}
}


static bool toggleNumlock(){
	const int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
	if (fd < 0){
		if (errno == EACCES)
			ERROR("Permission denied when opening event handler '/dev/uinput'.");
		else
			ERROR("Failed to open event handler '/dev/uinput'.");
		return false;
	}
	
	ioctl(fd, UI_SET_EVBIT, EV_KEY);
	ioctl(fd, UI_SET_KEYBIT, KEY_NUMLOCK);
	
	struct uinput_setup usetup = {
		.id = {
			.bustype = BUS_USB,
			.vendor = 0x1234,
			.product = 0x5678
		},
		.name = "Temporary numlockw"
	};

	ioctl(fd, UI_DEV_SETUP, &usetup);
	ioctl(fd, UI_DEV_CREATE);
	
	usleep(KEYPRESS_WAIT_MS * 1000);
	InputDevice::toggleNumlock(fd);
	usleep(KEYPRESS_WAIT_MS * 1000);

	ioctl(fd, UI_DEV_DESTROY);
	close(fd);
	return true;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


static void printNumlockDevices(vector<EventHandler>& events){
	sort(events.begin(), events.end(), [](EventHandler& a, EventHandler& b){
		return strcmp(a.event.c_str(), b.event.c_str());
	});
	
	for (EventHandler& ev : events){
		string name = InputDevice::getName(ev.fd);
		printf("%-7s", ev.event.c_str());
		printf("  (%-18s)", ev.path.c_str());
		printf("  '%s'\n", name.c_str());
	}
	
}


static void printHelp(){
	#define NL "\n"
	printf(ANSI_CYAN  ANSI_BOLD "numlockw" ANSI_RESET " by " ANSI_PURPLE "Hurkus" ANSI_RESET "." NL);
	printf("Usage: " ANSI_CYAN "numlockw" ANSI_RESET " [options] " ANSI_YELLOW "<on/off/toggle>" ANSI_RESET NL);
	printf(NL);
	printf("Operations:" NL);
	printf("  " ANSI_YELLOW "on" ANSI_RESET ", " ANSI_YELLOW "0" ANSI_RESET " ....... Activate numlock on all listed devices." NL);
	printf("  " ANSI_YELLOW "off" ANSI_RESET ", " ANSI_YELLOW "1" ANSI_RESET " ...... Deactivate numlock on all listed devices." NL);
	printf("  " ANSI_YELLOW "toggle" ANSI_RESET ", " ANSI_YELLOW "x" ANSI_RESET " ... Toggle numlock by spawning a temporary input device." NL);
	printf("                (Requires less permissions)" NL);
	printf("Options:" NL);
	printf("  " ANSI_YELLOW "-h" ANSI_RESET " ........... Print help." NL);
	printf("  " ANSI_YELLOW "-l" ANSI_RESET " ........... List all event handlers that support numlock." NL);
	printf("  " ANSI_YELLOW "-e <event>" ANSI_RESET " ... Use only the provided event handler(s) from /dev/input/." NL);
	printf("                 By default all event handlers that support numlock." NL);
	printf(NL);
}


static bool opts(char const* const* argv, int argc){
	if (argc <= 1){
		options.help = true;
		return true;
	}
	
	for (int i = 1 ; i < argc ; i++){
		if (argv[i] == nullptr){
			break;
		}
		
		string_view arg = argv[i];
		
		if (arg == "-h"){
			options.help = true;
		} else if (arg == "-l"){
			options.list = true;
		}
		
		else if (arg == "-e"){
			if (++i < argc && argv[i] != nullptr){
				options.eventHandlers.emplace_back(argv[i]);
			} else {
				ERROR("Missing event handler name. Usage: '-e <event*>' where '*' is the number of the event handler from /dev/input/.");
				return false;
			}
		}
		
		else if (arg == "off" || arg == "0"){
			options.operation = Operation::OFF;
		} else if (arg == "on" || arg == "1"){
			options.operation = Operation::ON;
		} else if (arg == "toggle" || arg == "x"){
			options.operation = Operation::TOGGLE;
		}
		
		else {
			ERROR("Unknown option '%s'.", argv[i]);
			return false;
		}
		
	}
	
	return true;
}


// --------------------------------- [ Main Function ] -------------------------------------- //


int main(int argc, char const* const* argv){
	if (!opts(argv, argc)){
		return 1;
	} else if (options.help){
		printHelp();
		return 0;
	}
	
	// Early check for toggle only.
	if (!options.list && options.operation == Operation::TOGGLE){
		return toggleNumlock() ? 0 : 1;
	}
	
	// Get list of numlock event handlers.
	vector<EventHandler> evs;
	if (!options.eventHandlers.empty()) {
		evs = loadNumlockDevices(options.eventHandlers);
	} else {
		evs = loadNumlockDevices();
	}
	
	if (evs.empty()){
		ERROR("No numlock devices listed.");
		return 1;
	} else if (options.list){
		printNumlockDevices(evs);
		closeEventHandlers(evs);
		return 0;
	}
	
	bool res = true;
	switch (options.operation){
		case Operation::NONE:
			break;
		case Operation::ON:
			setNumlock(evs, true);
			break;
		case Operation::OFF:
			setNumlock(evs, false);
			break;
		case Operation::TOGGLE:
			res = toggleNumlock();
			break;
	}
	
	closeEventHandlers(evs);
	return res ? 0 : 1;
}


// ------------------------------------------------------------------------------------------ //
