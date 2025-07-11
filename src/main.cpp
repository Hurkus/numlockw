#include <cassert>
#include <cerrno>
#include <vector>
#include <string_view>
#include <algorithm>
#include <filesystem>

#include <fcntl.h>
#include <unistd.h>
#include <linux/uinput.h>

#include "InputDevice.hpp"
#include "DEBUG.hpp"

using namespace std;


// ----------------------------------- [ Constants ] ---------------------------------------- //


constexpr int KEYPRESS_WAIT_MS = 500;


// ----------------------------------- [ Variables ] ---------------------------------------- //


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


// ----------------------------------- [ Functions ] ---------------------------------------- //


static bool getAllEventHandlers(vector<string>& handlers) noexcept {
	try {
		
		for (const filesystem::directory_entry& e : filesystem::directory_iterator("/dev/input/")){
			if (e.is_directory()){
				continue;
			}
			
			string name = e.path().filename().string();
			if (name.starts_with("event")){
				handlers.emplace_back(move(name));
			}
			
		}
		
	} catch (const exception& e){
		ERROR("Failed to list event handlers at '/dev/input/'.");
		return false;
	}
	return true;
}


void sortEventHandlers(vector<string>& handlers){
	sort(handlers.begin(), handlers.end(), [](const string& a, const string& b){
		return a.length() < b.length() || a < b;
	});
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


static bool printEventHandlers(const vector<string>& handlers){
	filesystem::path path;
	uint32_t err_count = 0;
	bool err_perm = false;
	
	for (const string& event : handlers){
		path.assign("/dev/input/").append(event);
		
		// Open event device and check for permission errors.
		const int fd = open(path.c_str(), O_WRONLY | O_NONBLOCK);
		if (fd < 0){
			err_count++;
			err_perm |= (errno == EACCES);
			continue;
		}
		
		// Print only numlock events.
		if (InputDevice::supportsKeyEvent(fd, KEY_NUMLOCK)){
			printf("%-7s", event.c_str());
			printf("  (%-18s)", path.c_str());
			printf("  '%s'\n", InputDevice::getName(fd).c_str());
		}
		
		close(fd);
	}
	
	// Permission error.
	if (err_count > 0){
		if (err_perm)
			ERROR("Permission denied.");
		if (handlers.size() == 1)
			ERROR("Failed to access event handler '/dev/input/%s'.", handlers.front().c_str());
		else if (handlers.size() == err_count)
			ERROR("Failed to access %d event handlers from '/dev/input/'.", err_count);
		else
			WARNING("Failed to access a few (%d) event handlers from '/dev/input/'.", err_count);
	}
	
	return err_count == 0;
}


static bool setNumlock(const vector<string>& handlers, bool state){
	filesystem::path path;
	uint32_t err_count = 0;
	bool err_perm = false;
	
	for (const string& event : handlers){
		path.assign("/dev/input/").append(event);
		
		// Open event device and check for permission errors.
		const int fd = open(path.c_str(), O_WRONLY | O_NONBLOCK);
		if (fd < 0){
			err_count++;
			err_perm |= (errno == EACCES);
			continue;
		}
		
		// Check for LED and numlock support
		if (!InputDevice::supportsKeyEvent(fd, KEY_NUMLOCK)){
			goto next;
		} else if (!InputDevice::supportsLED(fd, LED_NUML)){
			WARNING("Failed to determine numlock state of event handler '%s' because it doesn't support an LED.", event.c_str());
			goto next;
		}
		
		if (InputDevice::getLED(fd, LED_NUML) != state){
			INFO("Toggle %s", path.c_str());
			
			if (!InputDevice::toggleNumlock(fd)){
				ERROR("Failed to set numlock state of event handler '%s'.", event.c_str());
			}
			
			usleep(KEYPRESS_WAIT_MS * 1000);
		} else {
			INFO("Skip   %s", path.c_str());
		}
		
		next:
		close(fd);
	}
	
	// Permission error.
	if (err_count > 0){
		if (err_perm)
			ERROR("Permission denied.");
		if (handlers.size() == 1)
			ERROR("Failed to access event handler '/dev/input/%s'.", handlers.front().c_str());
		else if (handlers.size() == err_count)
			ERROR("Failed to access %d event handlers from '/dev/input/'.", err_count);
		else
			WARNING("Failed to access a few (%d) event handlers from '/dev/input/'.", err_count);
	}
	
	return err_count == 0;
}


static bool toggleNumlock(const vector<string>& handlers){
	filesystem::path path;
	uint32_t err_count = 0;
	bool err_perm = false;
	
	for (const string& event : handlers){
		path.assign("/dev/input/").append(event);
		
		// Open event device and check for permission errors.
		const int fd = open(path.c_str(), O_WRONLY | O_NONBLOCK);
		if (fd < 0){
			err_count++;
			err_perm |= (errno == EACCES);
			continue;
		}
		
		// Send key press event
		if (InputDevice::supportsKeyEvent(fd, KEY_NUMLOCK)){
			if (!InputDevice::toggleNumlock(fd))
				ERROR("Failed to toggle numlock state of event handler '%s'.", event.c_str());
		}
		
		close(fd);
	}
	
	// Permission error.
	if (err_count > 0){
		if (err_perm)
			ERROR("Permission denied.");
		if (handlers.size() == 1)
			ERROR("Failed to access event handler '/dev/input/%s'.", handlers.front().c_str());
		else if (handlers.size() == err_count)
			ERROR("Failed to access %d event handlers from '/dev/input/'.", err_count);
		else
			WARNING("Failed to access a few (%d) event handlers from '/dev/input/'.", err_count);
	}
	
	return err_count == 0;
}


static bool toggleVirtualNumlock(){
	const char* cpath = "/dev/uinput";
	
	const int fd = open(cpath, O_WRONLY | O_NONBLOCK);
	if (fd < 0){
		if (errno == EACCES)
			ERROR("Permission denied when accessing '%s'.", cpath);
		else
			ERROR("Failed to access '%s'.", cpath);
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
	INFO("Toggle %s", cpath);
	InputDevice::toggleNumlock(fd);
	usleep(KEYPRESS_WAIT_MS * 1000);

	ioctl(fd, UI_DEV_DESTROY);
	close(fd);
	return true;
}


// ----------------------------------- [ Functions ] ---------------------------------------- //


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
	
	vector<string>& evh = options.eventHandlers;
	
	// Enumerate and print numlock event handlers.
	if (options.list){
		if (evh.empty() && !getAllEventHandlers(evh)){
			return 1;
		} else {
			sortEventHandlers(evh);
			printEventHandlers(evh);
			return 0;
		}
	}
	
	bool res = false;
	switch (options.operation){
		case Operation::NONE:
			break;
		
		case Operation::ON:
		case Operation::OFF:
			if (!evh.empty() || getAllEventHandlers(evh))
				res = setNumlock(evh, options.operation == Operation::ON);
			break;
		
		case Operation::TOGGLE:
			if (evh.empty())
				res = toggleVirtualNumlock();
			else
				res = toggleNumlock(evh);
			break;
		
	}
	
	return res ? 0 : 1;
}


// ------------------------------------------------------------------------------------------ //
