#!/usr/bin/env python
"""Lesedi command line client."""

import cmd
import argparse
import sys
import time

from lesedi.sdk import constants
from lesedi.sdk.client import Connection


class CLI(cmd.Cmd):
    """Lesedi command line client.

    System controls:

         1. startup         Run the startup procedure
         2. shutdown        Run the shutdown procedure
         3. emergency stop  Stop all motion
         4. reset           Reset after emeregency stop


    System information:

         5. status          Display system status

    For help for a specific command type 'help <command>'.
    """

    prompt = 'Lesedi >> '

    # Make it easier for users to select a command.
    CMD_MAP = {
        '1': 'do_startup',
        '2': 'do_shutdown',
        '3': 'do_emergency_stop',
        '4': 'do_reset',
        '5': 'do_status',
    }

    def __init__(self, client):
        cmd.Cmd.__init__(self)
        if client is not None:
            self.client = client

    def default(self, line):
        command = line.split()[0].lower()
        if command == 's':
            self.do_status(line)
        elif command == 'q':
            return True
        elif command in self.CMD_MAP and hasattr(self, self.CMD_MAP[command]):
            getattr(self, self.CMD_MAP[command])(line)
        else:
            cmd.Cmd.default(self, line)

    def emptyline(self):
        """Do nothing when an empty line is entered.

        The default behaviour is to repeat the last command and that's not what
        we want.
        """
        pass

    def do_help(self, line):
        if not line:
            print(self.__doc__)
        else:
            cmd.Cmd.do_help(self, line)

    def do_quit(self, line):
        """Exit the program."""
        return True

    def do_EOF(self, line):
        return True

    def do_startup(self, line):
        self.client.startup()

    def do_shutdown(self, line):
        self.client.shutdown()

    def do_status(self, line):
        print(self.client.get_status())

    def do_emergency_stop(self, line):
        self.client.stop()

    def do_reset(self, line):
        self.client.reset()


def main():
    parser = argparse.ArgumentParser(description=__doc__, prog='lesedi-cli')

    parser.add_argument('--host', default=constants.DEFAULT_HOST,
        help='The remote host name')
    parser.add_argument('--port', default=constants.DEFAULT_PORT,
        help='The port on the remote host')

    arguments = parser.parse_args()

    try:
        client = Connection(arguments.host, arguments.port)
    except Exception as e:
        sys.exit(e)

    intro = 'Lesedi\nType "help" for more information.'

    try:
        CLI(client).cmdloop(intro)
    except KeyboardInterrupt:
        sys.exit(0)


if __name__ == '__main__':
    main()
