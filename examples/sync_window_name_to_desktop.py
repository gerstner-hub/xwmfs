#!/usr/bin/python3

import os
import sys

if len(sys.argv) != 2:
    print("Expected path to xwmfs mount as argument")
    sys.exit(1)

mount = sys.argv[1]

try:
    our_win = os.environ["WINDOWID"]
except KeyError:
    print("Expected WINDOWID variable in environment")
    sys.exit(1)

our_win_dir = os.path.join(mount, "windows", our_win)

our_events = os.path.join(our_win_dir, "events")

if not os.path.isfile(our_events):
    print("event file at", our_events, "is no existing")
    sys.exit(1)

event_fd = open(our_events, 'r')
our_name = os.path.join(our_win_dir, "name")
our_desktop = os.path.join(our_win_dir, "desktop")


def changeName():
    with open(our_desktop, 'r') as desktop_fd:
        desktop = desktop_fd.read().strip()

    with open(our_name, 'w') as name_fd:
        name_fd.write("Desktop {}".format(desktop))


while True:
    event = event_fd.readline().strip()

    if event == "desktop":
        changeName()
    elif event == "destroyed":
        # window is gone
        sys.exit(0)
