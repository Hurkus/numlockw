<h1 align="center">NUMLOCKW</h1>

Program for setting the numlock state.
Simillar to [numlockx](https://github.com/rg3/numlockx), but works on Wayland.
Best usecase is for startup scripts when other options for setting the starting state of numlock fail.


By default **numlockw** scans all event handlers at `/dev/input/` (those being `event0`, `event1`, ...)
and then filters for handlers that support the numlock key.
After that it tries to retrieve the current state of the numlock key of each event handler by looking at their LED status.
Depending on the desired state, it sends a key press event through that event handler, changing the state of numlock.
The user launching the program needs sufficient permissions to access the event handler files (`sudo`).

Because multiple event handlers can reflect a single physical input device, the program has to sleep for less than a second before checking
 the next event handler after sending a key press event.
The timeout is necessary so that the input device has enough time to recieve and process the key press.
This is not a major issue since most users only use 1 keyboard and this the program would only sleep once and then skip
 the other event handlers as they would reflect the same LED state.

When toggling the numlock state, the program spawns a temporary input device on `/dev/uinput`,
not too dissimilar to what is described in the [kernel docs](https://www.kernel.org/doc/html/v4.12/input/uinput.html#libevdev).
Doing it this way has a higher chance of working when lacking elevated permissions.
This means that toggling might work without `sudo`.

Library [libevdev](https://www.freedesktop.org/software/libevdev/doc/latest/) is <ins>not</ins> used, as that would add an unecessary dependency due to the simplicity of the task.



# Usage

The command line interface is straight forward:
```sh
numlockw <on/off/toggle> 
```

Commands `on`, `off` and `toggle` can be shortened to `1`, `0` and `x`.

Additional options:
* `-h` for printing the manual.
* `-e <event>` for targeting only the specified event handler(s). Can be supplied multiple times.
* `-l` for printing information about event handlers from `/dev/input/` that support numlock.

An example of targetting a specific device would require the user to first examine the event handlers using `numlockw -l`,
 and then listing the desired targets with e.g. `numlockw -e event16 -e event18 on`.




# Build

Using [make](https://www.gnu.org/software/make/manual/make.html):
```sh
make all
```

Or manually using a C++ compiler:
```sh
mkdir bin
g++ src/*.cpp -std=c++2a -D'NDEBUG' -O2 -o 'bin/numlockw'
```