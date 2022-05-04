#!/usr/bin/env python3

import sys
from base.base import TestBase

# tests whether some typical events and corner cases with events are
# recognized


class EventsTest(TestBase):

    def __init__(self):

        TestBase.__init__(self)
        self.m_event_file = None

    def waitEvent(self, name):

        while True:

            line = self.m_event_file.readline()

            if not line:
                raise Exception("EOF on event file")

            if line.strip() != name:
                print("Other event:", line)
            else:
                print("Caught expected event", line)
                break
        else:
            sys.stdout.flush()

    def testNameEvent(self):

        name_path = self.m_new_win.getFile("name")
        new_name = "somename"
        name_path.write(new_name)
        self.waitEvent("name")
        if name_path.read() != new_name:
            self.setBadResult("Didn't yield the expected name")

    def testDesktopEvent(self):

        desktop_path = self.m_new_win.getFile("desktop")
        num_desktops = self.getManagerFile("number_of_desktops")
        num_desktops = int(num_desktops.read())

        if num_desktops <= 1:
            raise Exception("Too few desktops to test, need at least two")

        cur_desktop = int(desktop_path.read())

        for i in range(num_desktops):
            if i != cur_desktop:
                new_desktop = i
                break

        desktop_path.write(str(new_desktop))
        self.waitEvent("desktop")

    def testDestroyEvent(self):

        self.closeTestWindow()
        self.waitEvent("destroyed")

    def test(self):

        self.m_new_win = self.createTestWindow(
            required_files=["pid", "name", "desktop"]
        )
        self.m_event_file = open(self.m_new_win.getPath("events"), 'r')

        print("Testing name change event")
        self.testNameEvent()
        print("Testing desktop change event")
        self.testDesktopEvent()
        print("Testing destroy event")
        self.testDestroyEvent()
        self.m_event_file.close()


et = EventsTest()
res = et.run()
sys.exit(res)
