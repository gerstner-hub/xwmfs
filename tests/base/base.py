from __future__ import print_function
import os, sys
import subprocess
import atexit

# tests whether changing a window name is reflected in the file system

class TestBase(object):

	def __init__(self):

		self.m_res = 0
		self.m_xwmfs = self.getBinary()

		if not os.path.isfile(self.m_xwmfs):

			print("Not a regular file:", self.m_xwmfs)
			sys.exit(1)

		atexit.register(self._cleanup)
		self.m_proc = None
		self.m_mount_dir = "/tmp/xwmfs"

	def _cleanup(self):

		if self.m_proc:
			self.m_proc.terminate()
			self.m_proc.wait()
			os.rmdir(self.m_mount_dir)

	def getBinary(self):

		xwmfs = os.environ.get("XWMFS", None)

		if xwmfs:
			return xwmfs

		if len(sys.argv) == 2:
			return sys.argv[1]

		print("Expecting path to xwmfs binary as parameter or in the XWMFS environment variable")
		sys.exit(1)

	def mount(self):

		if os.path.exists(self.m_mount_dir):
			print("Refusing to operate on existing mount dir", self.m_mount_dir)
			sys.exit(1)

		os.makedirs(self.m_mount_dir)

		self.m_proc = subprocess.Popen(
			[ self.m_xwmfs, "-f", self.m_mount_dir ]
		)

		while len(os.listdir(self.m_mount_dir)) == 0:

			# poll until the directory is actually mounted
			try:
				res = self.m_proc.wait(timeout = 0.25)
				print("Failed to mount xwmfs, exited with", res)
				sys.exit(1)
			except subprocess.TimeoutExpired:
				pass

	def unmount(self):

		self.m_proc.terminate()

		res = self.m_proc.wait()
		self.m_proc = None

		os.rmdir(self.m_mount_dir)

		if res != 0:
			print("xwmfs exited with non-zero code of", res)
			sys.exit(res)

	def run(self):

		self.mount()
		self.m_windows = os.path.join(self.m_mount_dir, "windows")

		self.test()

		self.unmount()

		return self.m_res

	def getTestWindow(self):

		our_id = os.environ.get("WINDOWID", None)

		if our_id:
			return our_id

		# otherwise just the first one we approach
		return os.listdir(self.m_windows)[0]

	def getWindowDir(self, window):

		windir = os.path.join(self.m_windows, window)

		if not os.path.isdir(windir):
			print("Test window directory", windir, "is not existing?")
			sys.exit(1)

		return windir

	def getWindowFile(self, window, which):

		windir = self.getWindowDir(window)
		return os.path.join(windir, which)

	def readContent(self, path):

		with open(path, 'r') as fd:
			return fd.read().strip()

	def writeContent(self, path, what):

		with open(path, 'w') as fd:
			fd.write(what)

	def setGoodResult(self, text):
		print("Good:", text)

	def setBadResult(self, text):
		print("Bad:", text)
		self.m_res = 1
