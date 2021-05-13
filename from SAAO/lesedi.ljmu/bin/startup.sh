# NOTE: These commands launch GUIs so be sure to enable X-forwarding using the "-Y" flag when executing this script remotely via SSH.

# Kill processes when exiting.
trap "kill 0" EXIT

cd /home/observer/bin

# Start all the SiTech software.
sudo mono SiTechExe.exe &
sleep 10

sudo mono ServoSCommunicator.exe &
sleep 10

mono DomeControlTCP.exe &
sleep 2
mono SiTechRotatorTCP.exe &
sleep 2
mono SiTechRotatorTCP.exe left &
sleep 2
mono SiTechFocuserTCP.exe &
sleep 4
mono ObservatoryControlSAAO.exe &

# Wait for exit.
wait
