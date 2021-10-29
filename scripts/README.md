# scripts

This directory contains a couple of scripts that can help create the binary and public_html (documentation) directory structure when cloning the repository to a new location.

The scripts are written in tcsh and require the mookodi_environment.csh to be sourced first. Ensure the SRC_HOME
environment variable is set to the correct location (the directory containing the cloned 'Mookodi' directory) before running the scripts. e.g.

* tcsh
* source /home/dev/src/Mookodi/mookodi_environment.csh
* cd /home/dev/src/Mookodi/scripts/
* ./make_binary_directories

This creates /home/dev/src/Mookodi/bin and subdirectories.

* ./make_documentation_directories

This creates /home/dev/src/Mookodi/public_html and subdirectories.

