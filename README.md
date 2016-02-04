# hydrajoy

A program that exposes the Razer Hydra controller as a 20 axis virtual joystick on Linux.

The virtual joystick can be used inside html5 applications in the web browser, for example.

(c) 2016 GPLv3 Juan Jose Luna Espinosa juanjoluna@gmail.com

Please read the Sixense SDK license.


### Requirements

A GNU/Linux system is required, currently 64 bit. If you want 32 bit support please add it yourself and make a Pull request! :-)

Python is also required, just to install the Razer Hydra if you have not done so before.


### Description

This is a native Linux program that reads Razer Hydra controller and outputs a virtual joystick with
the `uinput` subsystem. The joystick has the name "Hydrajoy virtual joystick" and has 20 axis and 24 buttons.

The virtual joystick is accessible from any application in the system, such as HTML5 applications running
in the browser and accessing the gamepad API.


### Known issues

Currently the Chrome web browser exposes a maximum of 16 channels per joystick. As there are 20 in the hydra, the last 4 are not exposed. They are the thumb joysticks of the 2 controllers (left and right). They are exposed in Firefox currently.

For this reason the virtual joystick includes 4 additional buttons for each of the 2 controllers that represent the four
directions of each thumb joystick. At least in Chrome you have the (digital) values of the thumb joysticks.


### Installation

Hydrajoy is a native application. You can place its root directory anywhere in your file system, but a few `udev` rules need to be installed. Here is how:

Where `sudo` is used, perhaps you can use `su` instead to enter root privileges, depending on your distro.

1. Obtain a clone of the hydrajoy git repository, or download and unzip a packaged version following this link:
    * https://github.com/yomboprime/hydrajoy
2. Install Razer Hydra udev rule. If you already have done this before, skip to B. Otherwise, do the following steps:
    * `cd` to the `lib/sixense/install` directory
    * Execute the script `install.py` with root privileges, this will install the udev rule for the Razer Hydra:
        * `sudo python ./install.py`
3. Install the udev rule for uinput by doing the following steps:
    * `cd` again to the root of the hydrajoy folder, where the repo is.
    * Edit the file `98-uinput.rules` and substitute your username where you see yombo, in the line:
        * `KERNEL=="uinput", GROUP="yombo", MODE:="0660"`
    * And save the file.
    * Copy the rule file to your system directory by doing:
        * `sudo cp ./98-uinput.rules /etc/udev/rules.d`
4. Add uinput to the list of modules to be loaded at runtime by doing this:
    * Edit (with a text editor) the file /etc/modules with root privileges and add a line with just the name `uinput`.
    * Then save it.
5. Reboot the system.


### Execution

First connect the Razer Hydra to the system ;-)

You must open a terminal and leave it opened with the `hydrajoy` program running while you use the Razer Hydra controller.

To run the program, `cd` to the main folder of the application and issue the command:
* For 32 bit system:
    * TODO :If you want support for 32 bit, add it yourself and make a Pull Request! The 32 bit sixense libraries are included.
* For 64 bit system:
    * `./hydrajoy64.sh`

If you don't know, just try one or the other and see if it works.

If the program stops, read the last line (well, almost) of the console to know the reason.

While the program runs all right there will be error messages in the console about USB stuff, don't pay attention to them, it's a known harmless bug.

If all goes well, You should have now a /dev/input/js0 device available to applications. See "Joystick axis and buttons" section for usage.

To stop the program hit ctrl-c in the console.


### Building

Building is only needed if the program doesn't work for your distro, or you want to contribute to the project.

Just `cd` to the `src` directory and execute `make`

For 32 bit system, it is not supported right now. Please add support and give me feedback!


### Troubleshooting

1. Program stops and this message appears: `"Could not open uinput file. Tried with /dev/uinput and /dev/input/uinput"`
    * Try loading the `uinput` module by issuing the following command:
        * `sudo modprobe uinput`
    * And then restart the program `hydrajoy`.


### Feedback

Feedback via Github issue system is welcome at https://github.com/yomboprime/hydrajoy


### Joystick axis and buttons

The positional axis are pre-divided by 4 to fit it in the -1...1 joystick range.
To convert a positional coordinate to real value (in meters) from the base, do for example:

    x = 4 * PositionX

So the range for the position is -4..4 meters from the base in all directions.

The rotation quaternion values are as is, from -1 to 1.

The thumb joysticks have common mapping also from -1 to 1. The trigger also has a range from -1 (released) to 1 (pulled).

The buttons have released state 0 and pushed state 1 (no intermediate values)
The special buttons number 7 and 14 tell if the given controller (respectively the
left and the right one) is docked. The button state is 0 if the controller is docked, or 1 if undocked.
This gives the advantage of activating the controller in the web browser when you take off the controller from the base.

    Axis mapping:

    Virtual joystick --- Razer Hydra axis          Razer hydra controller
        axis number                                  (0 = left controller, 1 = right controller)
            0            Position X                 0
            1            Position Y                 0
            2            Position Z                 0
            3            Rotation quaternion X      0
            4            Rotation quaternion Y      0
            5            Rotation quaternion Z      0
            6            Rotation quaternion W      0
            7            Trigger                    0
            8            Position X                 1
            9            Position Y                 1
            10           Position Z                 1
            11           Rotation quaternion X      1
            12           Rotation quaternion Y      1
            13           Rotation quaternion Z      1
            14           Rotation quaternion W      1
            15           Trigger                    1
            16           Joystick X                 0
            17           Joystick Y                 0
            18           Joystick X                 1
            19           Joystick Y                 1


Note: The reason for putting the thumb joysticks at the end is to bypass a Chrome limitation, see ```Known issues``` section for more details.
            
    Button mapping:

    Virtual joystick --- Razer Hydra button          Razer hydra controller
        button number                                (0 = left controller, 1 = right controller)
            0            Select                      0
            1            Button 1                    0
            2            Button 2                    0
            3            Button 3                    0
            4            Button 4                    0
            5            Joystick button             0
            6            Bumper                      0
            7            Docked                      0
            8            Joystick X Negative         0
            9            Joystick X Positive         0
            10           Joystick Y Negative         0
            11           Joystick Y Positive         0
            12           Select                      0
            13           Button 1                    1
            14           Button 2                    1
            15           Button 3                    1
            16           Button 4                    1
            17           Joystick button             1
            18           Bumper                      1
            19           Docked                      1
            20           Joystick X Negative         1
            21           Joystick X Positive         1
            22           Joystick Y Negative         1
            23           Joystick Y Positive         1

### Change Log

03 February 2016 -  Initial release.

