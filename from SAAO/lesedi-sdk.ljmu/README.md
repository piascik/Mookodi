# Lesedi Software Development Kit

The SDK contains the Thrift interface definition, a default client as well as
command line interface to communicate with a Thrift server.


## Develop

Follow the steps below to get started developing.


### Install

You can clone the repository from Bitbucket:

    $ git clone git@bitbucket.org:saao/lesedi-sdk.git

Then install the package in development mode:

    $ cd lesedi-sdk && python setup.py develop

NOTE: It is recommended to do this step with an active virtual environment.


### Command line interface

You can run the command line interface to communicate with a Thrift server:

    $ lesedi-cli


## Deploy

Install the SDK in production environments with the following command:

    $ pip install git+ssh://git@bitbucket.org/saao/lesedi-sdk.git#egg=lesedi-sdk

If it has already been installed and you would like to upgrade to a new
release, do the following:

    $ pip install --upgrade git+ssh://git@bitbucket.org/saao/lesedi-sdk.git#egg=lesedi-sdk


## For Developers

- [Thrift](https://thrift.apache.org)
- [virtualenv](https://virtualenv.pypa.io/en/stable/)
