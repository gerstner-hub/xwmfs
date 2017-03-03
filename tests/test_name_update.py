#!/usr/bin/env python

from __future__ import print_function
import os, sys
import random
import time
from base.base import TestBase

# tests whether changing a window name is reflected in the file system

class NameUpdateTest(TestBase):

	def __init__(self):

		TestBase.__init__(self)

	def test(self):

		test_window = self.getTestWindow()
		namefile = self.getWindowFile(test_window, "name")

		current_name = self.readContent(namefile)
		print("Current name of", test_window, "=", current_name)

		new_name = "random-name-" + str(random.randint(1, 999))
		self.writeContent(namefile, new_name)
		print("Changed name to", new_name)

		# give X some time to dispatch updates
		time.sleep(1)

		current_name = open(namefile, 'r').read().strip()

		if current_name == new_name:
			self.setGoodResult("New name found in FS")
		else:
			self.setBadResult("New name not reflected in file!")

nut = NameUpdateTest()
res = nut.run()
sys.exit(res)
