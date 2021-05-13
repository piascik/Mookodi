#!/bin/bash
# Kill processes when exiting.
trap "kill 0" EXIT 
sudo /home/user/carel1/.venv/bin/lesedi-aux &
lesedi-dome &
lesedi-focuser &
lesedi-rotator --port 1958 &
lesedi-rotator --port 1959 &
lesedi-telescope &
lesedi & 
# Wait for exit.
wait
