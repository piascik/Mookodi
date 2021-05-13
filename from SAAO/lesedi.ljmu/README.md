# Lesedi Telescope Control Library

Control code for the Lesedi telescope at the South African Astronomical
Observatory.


## Software Development Kit

Regardless of whether you are developing or installing the library in
production, you will need to install the Lesedi SDK:

    $ pip install git+ssh://git@bitbucket.org/saao/lesedi-sdk.git#egg=lesedi-sdk


## Develop

Follow the steps below to get started developing.


### Install

You can clone the repository from Bitbucket:

    $ git clone git@bitbucket.org:saao/lesedi.git

Then install the package in development mode:

    $ cd lesedi && python setup.py develop

NOTE: It is recommended to do this step with an active virtual environment.


### Start the server

For end-to-end testing, you can start the Thrift server:

    $ lesedi

A simulator is also provided for testing without direct communication with the
telescope:

    $ lesedi --test


### Command line interface

You can run a command line interface to communicate with the Thrift server:

    $ lesedi-cli

To test the command line interface without a running Thrift server:

    $ lesedi-cli --test


## Deploy

The easiest way to install the package on a system is to use `pip`:

    $ pip install git+ssh://git@bitbucket.org/saao/lesedi.git#egg=lesedi-lib

If it has already been installed and you would like to upgrade to a new
release, do the following:

    $ pip install --upgrade git+ssh://git@bitbucket.org/saao/lesedi.git#egg=lesedi-lib


### Managing Services

A systemd service is automatically created on installation and can be
controlled by issuing any one of the standard commands:

    $ systemctl [start|stop|restart|status] lesedi

NOTE: Run `systemctl daemon-reload` on a fresh system after installation.


## For Developers

- [SDK](https://bitbucket.org/saao/lesedi-sdk)
- [Thrift](https://thrift.apache.org)
- [systemd](https://www.freedesktop.org/wiki/Software/systemd/)
- [virtualenv](https://virtualenv.pypa.io/en/stable/)
