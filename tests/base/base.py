#!/usr/bin/python3

import argparse
import atexit
import os
import subprocess
import sys
import time


class File(object):

    def __init__(self, path):

        self.m_path = path

    def exists(self):

        return os.path.exists(self.m_path)

    def read(self):
        with open(self.m_path, 'r') as fd:
            return fd.read().strip()

    def write(self, what):

        with open(self.m_path, 'w') as fd:
            fd.write(what)

    def __str__(self):
        return self.m_path


class DirBase(object):

    def __init__(self, path):

        self.m_path = path

    def getFile(self, which):

        return File(self.getPath(which))

    def getPath(self, which):

        return os.path.join(self.m_path, which)


class Window(DirBase):

    def __init__(self, _id):

        self.m_id = _id
        DirBase.__init__(self, self.getDir())

    @classmethod
    def setBase(self, base):
        self.m_base = base

    def getDir(self):

        windir = os.path.join(self.m_base.m_windows, self.m_id)

        if not os.path.isdir(windir):
            print("Test window directory", windir, "is not existing?")
            sys.exit(1)

        return windir

    def __str__(self):
        return os.path.basename(self.m_id)


class ManagerDir(DirBase):

    def __init__(self, path):

        DirBase.__init__(self, path)


class TestBase(object):

    def setupParser(self):

        self.m_parser = argparse.ArgumentParser("xwmfs unit test")

        self.m_parser.add_argument(
            "-l", "--logfile",
            help="Path to write xwmfs logs to"
        )

        self.m_parser.add_argument(
            "-d", "--debug",
            help="Run xwmfs with debugging extras",
            action='store_true'
        )

        self.m_parser.add_argument(
            "-b", "--binary",
            help="Location of the xwmfs executable to test",
            default=None
        )

    def parseArgs(self):

        self.m_args = self.m_parser.parse_args()

    def __init__(self):

        self.setupParser()
        self.m_res = 0

        atexit.register(self._cleanup)
        self.m_proc = None
        self.m_test_window = None
        self.m_mount_dir = "/tmp/xwmfs"
        Window.setBase(self)

    def _cleanup(self):

        if self.m_proc:
            self.m_proc.terminate()
            self.m_proc.wait()
            os.rmdir(self.m_mount_dir)

        if self.m_test_window:
            self.closeTestWindow()

    def getBinary(self):

        xwmfs = os.environ.get("XWMFS", None)

        if self.m_args.binary:
            ret = self.m_args.binary
        elif xwmfs:
            ret = xwmfs
        else:
            ret = None

        if not ret:
            print("Expecting path to xwmfs binary as parameter or in the XWMFS environment variable")
            sys.exit(1)

        if not os.path.isfile(ret):
            print("Not a regular file:", ret)
            sys.exit(1)

        return ret

    def logSetting(self):

        return "111{}".format(
            "1" if self.m_args.debug else "0"
        )

    def extraSettings(self):

        debug_opts = ["--xsync"]
        # ["-o", "debug"]

        return debug_opts if self.m_args.debug else []

    def mount(self):

        if os.path.exists(self.m_mount_dir):
            print("Refusing to operate on existing mount dir", self.m_mount_dir)
            sys.exit(1)

        os.makedirs(self.m_mount_dir)

        self.m_proc = subprocess.Popen(
            [
                self.m_xwmfs, "-f",
                "--logger={}".format(self.logSetting()),
            ] + self.extraSettings() + [self.m_mount_dir]
        )

        while len(os.listdir(self.m_mount_dir)) == 0:

            # poll until the directory is actually mounted
            try:
                res = self.m_proc.wait(timeout=0.25)
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

        if "DISPLAY" not in os.environ:
            # don't fail tests because of a missing DISPLAY. this
            # is typically the case on automated build servers and
            # alike. our tests are mor for interactive testing.

            # according to autotools documentation this is the
            # exit code to signal a skipped test:

            # https://www.gnu.org/software/automake/manual/html_node/Scripts_002dbased-Testsuites.html
            return 77

        self.parseArgs()
        self.m_xwmfs = self.getBinary()
        self.mount()
        self.m_windows = os.path.join(self.m_mount_dir, "windows")
        self.m_mgr = ManagerDir(os.path.join(self.m_mount_dir, "wm"))

        self.test()

        self.unmount()

        return self.m_res

    def getWindowList(self):

        return [Window(w) for w in os.listdir(self.m_windows)]

    def getTestWindow(self):

        our_id = os.environ.get("WINDOWID", None)

        if our_id:
            return Window(our_id)

        # otherwise just the first one we approach
        return Window(self.getWindowList()[0])

    def createTestWindow(self, required_files=[]):
        # creates a new window and returns its window ID

        # this currently assumes an xterm executable is around

        if self.m_test_window:
            raise Exception("Double create of test window, without closeTestWindow()")

        print("Creating test window")
        try:
            self.m_test_window = subprocess.Popen("xterm")
        except Exception:
            print("Failed to run xterm to create a test window")
            raise

        our_win = None

        while not our_win:

            windows = self.getWindowList()

            for window in windows:
                pid = window.getFile("pid")
                try:
                    pid = int(pid.read())
                except Exception:
                    # race condition, no PID yet
                    continue

                if pid == self.m_test_window.pid:
                    our_win = window
                    break
            else:
                time.sleep(0.25)

        print("Created window", our_win, "waiting for", required_files)

        for req in required_files:
            wf = our_win.getFile(req)

            count = 0

            print("Waiting for", req, "file")
            while not wf.exists():
                count += 1
                time.sleep(0.25)

                if count >= 50:
                    raise Exception("Required window file '{}' did not appear".format(req))

        print("All files present")

        # wait for the window to become mapped
        mapped = our_win.getFile("mapped")

        while mapped.read() != "1":
            time.sleep(0.25)

        return our_win

    def closeTestWindow(self):
        # waits for a previously created test window to exit

        self.m_test_window.kill()
        self.m_test_window.wait()
        self.m_test_window = None

    def getManagerFile(self, which):

        return self.m_mgr.getFile(which)

    def setGoodResult(self, text):
        print("Good:", text)

    def setBadResult(self, text):
        print("Bad:", text)
        self.m_res = 1
