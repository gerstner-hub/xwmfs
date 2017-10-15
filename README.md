INTRODUCTION
============

This is xwmfs (X window manager file system), a userspace file system based on
fuse that allows interaction with an EWMH compliant X11 window manager
via files.

Some of its features are:

- newly appearing and disappearing windows in the X server are recognized and
  the file system is updated in an event based manner
- new values for properties of window manager and windows will be reflected in
  the file system in an event based manner
- properties of windows and window manager can be changed via writing to files
  in the file system
- some X operations are accessible via control files in the file system

The file system can be used for easily implementing scripts that operate on
the window manager and windows (for example identifying specific windows,
rename a window, move it around and so on).

For build and installation instructions see INSTALL.

EWMH (Extended Window Manager Hints) is a specification of how a window
manager makes information about the current state of itself and the windows it
manages available to other programs. Not all window managers support all
features of this specification and not all support all features correctly. The
following window managers have been basically tested:

- fluxbox
- i3 (does not support the desktop property per window)
- compiz
- xfce (window names can't be changed)
- enlightenment (window names can't be changed, many global wm attributes are
  not supported)
- iceWM (window names can't be changed)
- kde
- gnome

xwmfs is currently not broadly tested. Thus it is not necessarily very stable.
Bug reports via the github issue tracker are welcome.

USAGE
=====

xwmfs consists of a single executable named 'xwmfs'. Currently no configuration
files of any kind are used. The xwmfs program behaves like a standard fuse
program. This means a lot of standard command line options are provided that
influence the behaviour of the file system in general and are not specific to
the xwmfs software.

To mount the file system with default options you simply run xwmfs like this:

	xwmfs [-f] <mount-location>

The switch -f causes the program to run in the foreground, because by default
it will daemonize into the background. Running it in the foreground can be
helpful for testing and for getting log messages.

The <mount-location> can be any suitable location in the file system where the
xwmfs file system will be mounted on. The calling user needs to have write
permissions for the mount-location as is the case with any regular mount of
file systems.

Because the xwmfs acts as an X11 client on the X server it requires to have a
valid `DISPLAY` environment variable set and the authority to access the X11
display. Otherwise the xwmfs program will print an error and exit immediately.

To unmount the xwmfs file system the command

	fusermount -u <mount-location>

can be used.

FILE SYSTEM STRUCTURE
=====================

Once mounted the xwmfs file system presents the following hierarchy:

<pre>
-windows: A subdirectory containing further directories that represent all
 |        windows managed by the window manager on the current display.
 |
 |--------> <ID>: A directory representing a single window managed by the
 |       |        window manager with the given ID. The ID is unique per window
 |       |        and can also be used to match information from other tools
 |       |        like 'xprop'
 |       |--------> id: A virtual file that again contains the <ID> of the
 |       |              window it represents
 |       |--------> name: A virtual file that contains the user visible name or
 |       |                title of the window. You can also write to this file
 |       |                for giving the window a new name.
 |       |--------> desktop: The number of the desktop the window is currently
 |       |                   located on. Counting starts at zero. You can also
 |       |                   write a new desktop number to this file to move
 |       |                   the window to another desktop.
 |       |--------> pid: The PID of the process that owns the window
 |       |--------> control: Can be used to send a command to the window. When
 |       |                   read then a list of valid commands is returned.
 |       |                   Currently "destroy" or "delete" to force or ask
 |       |                   the window to be closed.
 |       |--------> events:  Produces one line for each event related to the
 |       |                   window's directory. Each line will consist of the
 |       |                   basename of the file that changed.
 |       |--------> mapped:  Produces a boolean value (0, 1) whether this
 |       |                   window is currently mapped (visible)
 |       |--------> properties:
 |       |                   Returns a line-wise list of all properties
 |       |                   attached to the window. Format is "NAME<TYPE> =
 |       |                   <VALUE>". You can also add or change properties
 |       |                   by writing strings of the same format. You can
 |       |                   delete a property by writing a string of the
 |       |                   format "!<NAME>".
 |       |--------> class:   Contains two newline separated string denoting
 |       |                   the name of the application class and instance
 |       |--------> command: Contains the command line that was used to start
 |       |                   the application that created the window
 |       |--------> locale:  Contains the locale name used by the window's
 |       |                   application (WM_LOCALE_NAME)
 |       |--------> procotols:
 |       |                   The newline separated list of protocols supported
 |       |                   by the window (WM_PROTOCOLS)
 |       |--------> client_leader:
 |       |                   The window ID of the client leader window for
 |       |                   this window
 |       |--------> window_type:
 |       |                   The type of this window, fixed set of constants
 |       |--------> geometry:
 |       |                   Contains a string of the from "X,Y:WxH", denoting
 |       |                   the x/y position of the upper left corner and the
 |       |                   width and height of the window. When writing a
 |       |                   correspondingly formatted value to this file then
 |       |                   the window will be moved and/or resized accordingly.
-wm: A directory containing global state information about the
 |   window manager
 |
 |--------> name: Contains the name of the running window manager.
 |--------> class: May contain an identifier that defines the kind of
 |                 window manager that is running
 |--------> active_desktop: Contains the number of the currently active
 |                          desktop, if virtual desktops are available
 |                          and configured. Counting starts at zero.
 |--------> number_of_desktops: Contains the number of virtual desktops
 |                              currently configured, if supported
 |--------> show_desktop_mode: Returns a 0/1 value whether currently the
 |                             "show desktop mode" is active. This means
 |                             that all windows are hidden from view and
 |                             desktop icons and background are shown
 |                             instead
 |--------> pid: Contains the PID of the running window manager process
 |--------> events: Produces one line for each event related to the wm
 |                    directory. Each line will consist of the basename of the
 |                    file that changed
 |--------> parent: Returns the window ID of the parent window of this window.
 |                  Note that this window might not be known by xwmfs, because
 |                  it can be a "pseudo window", a decoration window created
 |                  by the window manager or similar. Use the parameter
 |                  --handle-pseudo-windows to also display these kind of
 |                  windows in xwmfs. Contains 0 for the root window.
-selections: A directory containing files that allow access to the X server
 |           selection buffers also known as clipboard buffers
 |
 |--------> owners: contains one line per well-known selection buffer,
 |                  identifying the current window ID that _owns_ the
 |                  selection buffer in question.
 |--------> primary: On read this returns the content of the primary selection
 |                   formatted as UTF8 text. This is the selection that is
 |                   typically pasted when pressing the middle mouse button.
 |                   If there is currently no owner for the primary selection
 |                   then an error of EAGAIN is returned.
 |                   On write the xwmfs file system will become owner of
 |                   the primary selection and store the data written to
 |                   this node as the new selection content which will be
 |                   served when other X clients query the selection. Simply
 |                   put: The written data will become the new content of the
 |                   primary selection.
 |--------> clipboard: Just like primary above but operating on the clipboard
 |                     selection buffer instead. This is the selection buffer
 |                     that is typically operated on by using the ctrl-c /
 |                     ctrl-v key combinations.
</pre>

Should the window manager not support some of the properties like
"show_desktop_mode" then a value of -1 is contained in the file if the file
represents an integer value or the value "N/A" if the file represents a string
value.

CURRENT STATE OF DEVELOPMENT
============================

xwmfs conforms to the EWMH and ICCCM specifications regarding the name, type
and meaning of X window properties used for representing the state of window
managers.

Not all window managers implement these specifications. Some window
managers only implement part of them. Some don't exactly conform to them. Thus
xwmfs would require to adapt to different types and versions of window managers
to work around some of these limitations.

This is not currently the case. This means if anything unexcepted happens then
xwmfs might behave strangely or simply not be able to provide all features.

xwmfs is able to detect changes to windows (windows appearing and
(dis)appearing or changing properties) in an event based way. This makes is
rather efficient and the `events` file nodes can be used for efficiently
waiting for changes on a given window similar to what the `xev` program does.

CONTRIBUTION
============

xwmfs now has the feature set I originally intended for it. There will still
be a plethora of additional features and compatibility one could think of. But
I'm not actively developing new features at the moment. If you have a request
or problem, then please use the github issue tracker and pull request
interface. You can also email me directly if you like (see AUTHORS file).

TIPS AND TRICKS
===============

- You can get the window ID of the current window in most terminal emulators
  from the $WINDOWID environment variable

FUTURE DIRECTIONS
=================

Some features that might still be worth implementing:

* visibility status of windows (raised/lowered)
* actively lower/raise windows
* possibility to change names of virtual desktops as applicable
