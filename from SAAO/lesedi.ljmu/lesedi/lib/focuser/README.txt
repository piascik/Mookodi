Starting with version 1.0 of SiTechFocuserTCP, there is a TCP/IP Server interface to SiTechFocuserTCP.
First you set up the port number in the Config tab, the textbox labeled "My Port Number"

Note, if something is connected when you change this, there will likely be an exception in the external app.
Best to disconnect all TCP processes before changing this SiTechFocuserTCP config item.

The following commands can be used using the ObservatoryControl Server by prefixing the command with "Focuser_" (ObservatoryControl version 1.1 or later)

You need to be running SiTechFocuserTCP, ServoCommunicatorTCP AND SiTechExe before connecting with your software.
Basically, you send an ASCII string as a command, SiTechFocuserTCP possibly does something, and returns a string.
Nothing blocks in SiTechFocuserTCP, the string is returned right away, even if the command will take a while to complete.
 
Unless otherwise stated, every command returns the standard return string. The command "GetFocuserData" returns "_GetFocuserData" as the message.
Other commands return a message after the "_";

The Standard response String:
Parameters are separated by a ';' semi-colon.
As of version 1.0, here is the standard return string description. 
The type (int, double, or string) is a literal string, your software must convert these strings to an int or double if necessary.
int boolParms (IsMoving, Tracking, BadComm, etc.)
double FocuserPosition (inches)	
String "_" + ReturnMessage

The boolParms have bits in it that mean certain things as follows:
Bit 00 (AND with    1) Focuser Is Moving
Bit 01 (AND with    2) Tertiary Mirror Is Moving
Bit 02 (AND with    4) Focuser Controller is in Auto (not blinky)
Bit 03 (AND with    8) Tertiary Mirror Controller is in Auto (not blinky)
Bit 04 (AND with   16) Focuser is At or Past a limit
Bit 05 (AND with   32) Focuser is performing an AutoFocus now
Bit 06 (AND with   64) Tertiary Mirror Controller is at Left Fork Position
Bit 07 (AND with  128) Tertiary Mirror Controller is at Right Fork Position
Bit 08 (AND with  256) Focuser can't communicate with ServoCommunicator
Bit 09 (AND with  512) ServoCommunicator isn't communicating with the ServoController

Return String Example:
278;2.739;347.457;_GetFocuserData
278 is equal to 100010110 binary.
In this case, the focuser is in Auto, It's at or past a limit, and we're NOT communicating with ServoCommunicator
The focuser position is 2.739 inches, the Tertiary Mirror angle is 347.457

Commands:
"GetFocuserTertiaryData"
Returns the standard response.  The ReturnMessage is "" (after the "_")

"StopFocuser"
This will stop the Focuser movement.
Returns the standard response.  The ReturnMessage is "StopFocuserAccepted"

"MoveFocuserTo nn.nn"
This will move the Focuser to nn.nn inches (or will move to the limit).
Returns the standard response.  The ReturnMessage is "MoveFocuserAccepted"

"DoAutoFocus"
Returns the standard response.
If already doing an autofocus, the ReturnMessage is "AlreadyDoingAutoFocus";
else The ReturnMessage is "FocuserToAutoAccepted"

"CancelAutoFocus"
Returns the standard response.
If already doing an autofocus, the ReturnMessage is "CancelAutoFocusAccepted";
else The ReturnMessage is "NotDoingAutoFocus!"

"ToAutoFocuser"
Returns the standard response.
The ReturnMessage is "ToAutoFocuserAccepted";

"ToBlinkyFocuser"
Returns the standard response.
The ReturnMessage is "ToBlinkyFocuserAccepted";

"RestartCommunicatorComms"
This will try to reconnect to ServoCommunicator if there is no communications.
Returns the standard response.  The ReturnMessage is "RestartCommunicatorCommsAccepted"

"GoLeftTertiary"
Returns the standard response.
The ReturnMessage is "GoLeftTertiaryAccepted";

"GoRightTertiary"
Returns the standard response.
The ReturnMessage is "GoRightTertiaryAccepted";

"MoveTertiaryTo nn.nn"
This will move the Tertiary mirror to nn.nn angle (or will move to the limit).
Returns the standard response.  The ReturnMessage is "MoveTertiaryAccepted"

"JogTertiary nn.nn"
This will jog the Tertiary mirror by nn.nn angle (or will move to the limit).
Returns the standard response.  The ReturnMessage is "JogTertiaryAccepted"

"ToAutoTertiary"
Puts the Tertiary Servo Motor into the "Auto" mode
Returns the standard response.
The ReturnMessage is "ToAutoTertiaryAccepted";

"ToBlinkyTertiary"
Puts the Tertiary Servo Motor into the "Manual"  (or blinky) mode
Returns the standard response.
The ReturnMessage is "ToBlinkyTertiaryAccepted";

"BaffleOpen"
Opens the Baffle
Returns the standard response.
The ReturnMessage is "BaffleOpenAccepted";

"BaffleClose"
Closes the Baffle
Returns the standard response.
The ReturnMessage is "CloseBaffleAccepted";

"MirrorCoverOpen"
Opens the Mirror Cover
Returns the standard response.
The ReturnMessage is "MirrorCoverOpenAccepted";

"MirrorCoverClose"
Closes the Mirror Cover
Returns the standard response.
The ReturnMessage is "MirrorCoverCloseAccepted";
