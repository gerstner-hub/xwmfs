#!/usr/bin/env python3

from base.base import TestBase
import random
import sys
import time

# tests whether changing a window name is reflected in the file system


class NameUpdateTest(TestBase):

    def __init__(self):

        TestBase.__init__(self)

    def test(self):

        test_window = self.getTestWindow()
        namefile = test_window.getFile("name")

        current_name = namefile.read()
        print("Current name of", test_window, "=", current_name)

        new_name = "random-name-" + str(random.randint(1, 999))
        namefile.write(new_name)
        print("Changed name to", new_name)

        # give X some time to dispatch updates
        time.sleep(1)

        orig_name = current_name
        current_name = namefile.read()

        if current_name == new_name:
            self.setGoodResult("New name found in FS")
        else:
            self.setBadResult("New name not reflected in file!")

        # reset original name
        namefile.write(orig_name)
        print("Reset to", orig_name)


nut = NameUpdateTest()
res = nut.run()
sys.exit(res)
