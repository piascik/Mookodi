DOME CONTROL SERVER DOCUMENT

Starting with version 1.0 of DomeControlTCP, there is a TCP/IP Server interface to DomeControlTCP.

Note: the current version installed at Lesedi has the return string separated by spaces instead of semi-colon’s. The next update, it will change to spaces as is documented here.

First you set up the port number in the Config tab, the textbox labeled "My Port Number"

Note, if something is connected when you change this, there will likely be an exception in the external app. Best to disconnect all TCP processes before changing this DomeControlTCP config item.

The following commands can be used using the ObservatoryControl Server by prefixing the command with "DomeControl_" (ObservatoryControl version 1.1 or later)

You need to be running ServoCommunicator AND SiTechExe before connecting with this software.
Basically, you send an ASCII string as a command, DomeControlTCP possibly does something, and returns a string. Nothing blocks in DomeControlTCP, the string is returned right away, even if the command will take a while to complete.

Unless otherwise stated, every command returns the standard return string. The command "GetDomeData" returns "_" as the message.
Other commands return a message after the "_";

The Standard response String:

Parameters are separated by a ';' semi-colon.
As of version 1.0, here is the standard return string description.
The type (int, double, or string) is a literal string, your software must convert these strings to an int or double if necessary.

int DomePosition (Degs * 10)
int BitsFromDome0 (PLC Register 106Eh (D100) )
int BitsFromDome1 (PLC Register 106Fh (D101) )
int BitsFromDomeDriver (IsMoving, Slaved, BadComm, etc.)
String "_" + ReturnMessage

The BitsFromDome0 have bits in it that mean certain things as follows:

Bit 00 (AND with 1) Shutters are closed
Bit 01 (AND with 2) Shutters are opened
Bit 02 (AND with 4) Shutter motor is on
Bit 03 (AND with 8) Shutter fault
Bit 04 (AND with 16) Dome is moving
Bit 05 (AND with 32) Dome rotation fault
Bit 06 (AND with 64) Not used
Bit 07 (AND with 128) Dome lights are on
Bit 08 (AND with 256) Manual light switch position (If drop to manual this is what the dome lights will do)
Bit 09 (AND with 512) Shutters are bing opened manually
Bit 10 (AND with 1024) Shutters are being Closed manually
Bit 11 (AND with 2048) Dome is moving right manually
Bit 12 (AND with 4096) Dome is moving left manually
Bit 13 (AND with 8192) Not Used
Bit 14 (AND with 16384) Dome Shutter Power Status '1' is on
Bit 15 (AND with 32768) Dome Rotation Power Status '1' is on


The BitsFromDome1 have bits in it that mean certain things as follows:

Bit 00 (AND with 1) PLC is in Remote Operation
Bit 01 (AND with 2) Set if it's raining
Bit 02 (AND with 4) Set if the TCS is locked out
Bit 03 (AND with 8) Not Used
Bit 04 (AND with 16) Not Used
Bit 05 (AND with 32) Not Used
Bit 06 (AND with 64) Not Used
Bit 07 (AND with 128) Not Used
Bit 08 (AND with 256) Not Used
Bit 09 (AND with 512) Not Used
Bit 10 (AND with 1024) Set if Emergency Stop is presse
Bit 11 (AND with 2048) Not Used
Bit 12 (AND with 4096) Set if shutters closed due to r
Bit 13 (AND with 8192) Set if there is a power failure
Bit 14 (AND with 16384) Set if the shutters close beca
Bit 15 (AND with 32768) Set if the watchdog is tripped


BitsFromDomeDriver

Bit 00 (AND with 1) No communication with ServoSCommunicator
Bit 01 (AND with 2) No Communication with SiTechExe
Bit 02 (AND with 4) No Communication with PLC
Bit 03 (AND with 8) Dome is at position setpoint
Bit 04 (AND with 16) Dome is following
Bit 05 (AND with 32) Dome is moving
Bit 06 (AND with 64) Shutter is moving
Bit 07 (AND with 128) Slew Lights are On
Bit 08 (AND with 256) Dome Lights are On
Bit 09 (AND with 512)
Bit 10 (AND with 1024)
Bit 11 (AND with 2048) Not Used
Bit 12 (AND with 4096)
Bit 13 (AND with 8192)
Bit 14 (AND with 16384)
Bit 15 (AND with 32768)


Return String Example: 2700;2523;2178;356;_

Commands:

"GetDomeData"
Returns the standard response. The ReturnMessage is ""

"GoParkAndClose"
This will park the dome, and close the shutters
Returns the standard response. The ReturnMessage is ""

"ToRemoteControl"
Puts the PLC in Remote control if not locked out Returns the standard response. The ReturnMessage is ""

"SlewLightsOn"
Turns the slew lights on if in remote
Returns the standard response. The ReturnMessage is ""

"SlewLightsOff"
Turns the slew lights off if in remote
Returns the standard response. The ReturnMessage is ""

"DomeLightsOn"
Turns the dome lights on if in remote
Returns the standard response. The ReturnMessage is ""

"DomeLightsOff"
Turns the dome lights off if in remote
Returns the standard response. The ReturnMessage is ""

"OpenShutters"
Opens the shutters if in remote
Returns the standard response. The ReturnMessage is ""

"CloseShutters"
Closes the shutters if in remote
Returns the standard response. The ReturnMessage is ""

"FollowTelescopeStart"
Now the dome will follow the telescope if in remote Returns the standard response. The ReturnMessage is ""

"FollowTelescopeStop"
Now the dome won't follow the telescope if in remote Returns the standard response. The ReturnMessage is ""

"MoveDomeTo nn.n"
this will move the dome to 0.0 to 360.0 degs if in remote Returns the standard response. The ReturnMessage is ""

"EmergencyStop"
Stops all motion if in remote
Returns the standard response. The ReturnMessage is ""

"PowerMotorsOff"
Turns off both the Shutter power and the Rotation power if in remote Returns the standard response. The ReturnMessage is ""

"PowerMotorsOn"
Turns on both the Shutter power and the Rotation power if in remote Returns the standard response. The ReturnMessage is ""
