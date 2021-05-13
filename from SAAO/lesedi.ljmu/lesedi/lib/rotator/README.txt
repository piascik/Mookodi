ROTATOR CONTROL SERVER DOCUMENT

Starting with version 1.0 of SiTechRotatorTCP, there is a TCP/IP Server interface to SiTechRotatorTCP. First you set up the port number in the Config tab, the textbox labeled "My Port Number"
Note, if something is connected when you change this, there will likely be an exception in the external app. Best to disconnect all TCP processes before changing this SiTechRotatorTCP config item.
The following commands can be used using the ObservatoryControl Server by prefixing the command with "RotatorLeft_" or "RotatorRight_" (ObservatoryControl version 1.1 or later)
You need to be running SiTechRotatorTCP AND SiTechExe before connecting with your software.
Basically, you send an ASCII string as a command, SiTechRotatorTCP possibly does something, and returns a string. Nothing blocks in SiTechRotatorTCP, the string is returned right away, even if the command will take a while to complete.
Unless otherwise stated, every command returns the standard return string. The command "GetRotatorData" returns "_GetRotatorData" as the message.
Other commands return a message after the "_";
The Standard response String:
Parameters are separated by a ';' semi-colon.
As of version 1.0, here is the standard return string description.
The type (int, double, or string) is a literal string, your software must convert these strings to an int or double if necessary. int boolParms (IsMoving, Tracking, BadComm, etc.)

double RotatorAngle (Degs)
double ParallacticAngle (Degs)
double ParallacticRate(Revolutions per sidereal day)
String "_" + ReturnMessage

Bit 00 (AND with 1) Rotator Is Moving (Tracking doesn't count)
Bit 01 (AND with 2) Rotator Is Tracking (remains true when moving) 4) Rotator Controller is in Auto (not blinky)
Bit 02 (AND with 8) Not used, should always be a '0'
Bit 03 (AND with 16) Rotator is At or Past a limit
Bit 04 (AND with 32) Not used, should always be a '0' 64) Not used, should always be a '0'
Bit 05 (AND with 128) Not used, should always be a '0'
Bit 06 (AND with 256) Rotator can't communicate with ServoCommunicator
Bit 07 (AND with 512) ServoCommunicator isn't communicating with the ServoController
Bit 08 (AND with 1024) Rotator is moving by Buttons

The boolParms have bits in it that mean certain things as follows:

Return String Example:

278;142.739;347.457;0.7029;_GetRotatorData
278 is equal to 100010110 binary.
In this case, the rotator is tracking, it's in Auto, It's at or past a limit, and we're NOT communicating with ServoCommunicator The rotator angle is 142.739 deg's,
The Parallactic angle of the telescope is 347.457 deg's,
The Parallactic Rate of the telescope (and thus, the rotator tracking speed) is 0.7029 revolutions per sidereal day.

Commands:

"GetFocuserTertiaryData"
Returns the standard response. The ReturnMessage is ""

"JogRotatorDegs nn.nn"
This will jog the rotator nn.nn deg's. If it's negative, it will decrease the angle, otherwise, it will increase the angle by nn.nn. Returns the standard response. The ReturnMessage is "JogRotatorDegs"

"MoveRotatorToAngle nn.nn"
This will move the rotator to nn.nn deg's (or will move to the limit).
Returns the standard response. The ReturnMessage is "MoveRotatorToAngleAccepted"

"MoveRotatorToParallacticAngle"
This will move the rotator to the parallactic angle (North Up) (or will move to the limit). Returns the standard response. The ReturnMessage is "MoveRotatorToAngleAccepted"

"RotatorTrackingOn"
This will move the rotator to the parallactic angle (North Up) (or will move to the limit). Returns the standard response. The ReturnMessage is "TrackingOnAccepted"

"RotatorTrackingOff"
This will move the rotator to the parallactic angle (North Up) (or will move to the limit). Returns the standard response. The ReturnMessage is "TrackingOffAccepted"

"SpinRotator nn.nn"
This will move the rotator to the limit at speed nn.nn degs per second
Returns the standard response. The ReturnMessage is "SpinRotatorAccepted"

"RotatorToAuto"
If the Servo Controller is in "Manual" mode (blinky), this command will put it in "Auto" mode. Returns the standard response. The ReturnMessage is "RotatorToAutoAccepted"

"RotatorToBlinky"
If the Servo Controller is in "Auto" mode, this command will put it in "Manual" (blinky) mode. Returns the standard response. The ReturnMessage is "RotatorToBlinkyAccepted"
