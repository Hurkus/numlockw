#include <cassert>
#include <cerrno>
#include <cstring>
#include <string_view>
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


// ----------------------------------- [ Variables ] ---------------------------------------- //


enum class Operation : char {
	NONE,
	OFF,
	ON,
	TOGGLE,
	GET
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


static bool printNumlockDevices(){
	vector<EventHandler> evs;
	if (!options.eventHandlers.empty()) {
		evs = loadNumlockDevices(options.eventHandlers);
	} else {
		evs = loadNumlockDevices();
	}
	
	sort(evs.begin(), evs.end(), [](EventHandler& a, EventHandler& b){
		return strcmp(a.event.c_str(), b.event.c_str());
	});
	
	for (EventHandler& ev : evs){
		string name = InputDevice::getName(ev.fd);
		printf("%-7s", ev.event.c_str());
		printf("  (%-18s)", ev.path.c_str());
		printf("  '%s'\n", name.c_str());
	}
	
	closeEventHandlers(evs);
	return evs.size() > 0;
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
		} else if (arg == "get"){
			options.operation = Operation::GET;
		}
		
	}
	
	return true;
}


// --------------------------------- [ Main Function ] -------------------------------------- //


int main(int argc, char const* const* argv){
	if (!opts(argv, argc)){
		return 1;
	}
	
	if (options.help){
		printHelp();
		return 0;
	} else if (options.list){
		return printNumlockDevices() ? 0 : 1;
	}
	
	// switch (options.operation){
	// 	case Operation::ON:
	// 	case Operation::OFF:
	// }
	
	return 0;
}


// ------------------------------------------------------------------------------------------ //
