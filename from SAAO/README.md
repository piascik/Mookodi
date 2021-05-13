# Mookodi Toy Simulator and Template  

SAAO provided Mookodi python sample code and telescope simulator run-time environment


### Installation
Recommended prerequisites:
   sudo privilege
   Ubuntu 20.04 
   bash command shell
   python 3.7 or later
   virtualenv - virtual Python environment

Create and activate a virtual python environment

    $ python3 -m venv .venv
    $ source .venv/bin/activate

Build and deploy a LJMU specific development environment

    $ cd lesedi-sdk.ljmu
    $ python3 setup.py develop
    $ cd ..
    $ cd lesedi-sdk.ljmu
    $ python3 setup.py develop
    $ cd ..

### Running
Ensure the python virtual enviroment has been set up prior to running

    $ source .venv/bin/activate


The simulator processes are invoked by lesedi-test.sh script.
The lesedi-aux process accesses network devices so you will be prompted for sudo password

    $ ./lesedi-test.sh

The script starts these processes

  lesedi-dome
  lesedi-focuser
  lesedi-rotator
  lesedi-rotator
  lesedi-telescop
  lesedi
  lesedi-aux


### Command Line Interface

The running of the processes can sometime be verified using lesedi-cli

    $ lesedi-cli

It is erratic, sometimes working but mostly raising an exception.
Leaving the processes running for while before trying lesedi-cli appears to help.


### Template Example Python

SAAO provide template examples in the mookodi_toy sub-directory
It needs the lesedi server processes to be successfully running

    $ python3 test.py


### Relevant Web-links 

- [Thrift](https://thrift.apache.org)
- [virtualenv](https://virtualenv.pypa.io/en/stable/)

