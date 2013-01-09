#!/usr/bin/python
"""\
@file   update_install.py
@author Nat Goodspeed
@date   2012-12-20
@brief  Update the containing Second Life application bundle to the version in
        the specified disk image file.

        This Python implementation is derived from the previous mac-updater
        application, a funky mix of C++, classic C and Objective-C.

$LicenseInfo:firstyear=2012&license=viewerlgpl$
Copyright (c) 2012, Linden Research, Inc.
$/LicenseInfo$
"""

import os
import sys
import cgitb
import errno
import glob
import plistlib
import re
import shutil
import subprocess
import tempfile
import time
from janitor import Janitor
from messageframe import MessageFrame
import Tkinter, tkMessageBox

TITLE = "Second Life Viewer Updater"
# Magic bundle identifier used by all Second Life viewer bundles
BUNDLE_IDENTIFIER = "com.secondlife.indra.viewer"

# Global handle to the MessageFrame so we can update message
FRAME = None
# Global handle to logfile, once it's open
LOGF  = None

# ****************************************************************************
#   Logging and messaging
#
#   This script is normally run implicitly by the old viewer to update to the
#   new viewer. Its UI consists of a MessageFrame and possibly a Tk error box.
#   Log details to updater.log -- especially uncaught exceptions!
# ****************************************************************************
def log(message):
    """write message only to LOGF (also called by status() and fail())"""
    # If we don't even have LOGF open yet, at least write to Console log
    logf = LOGF or sys.stderr
    logf.writelines((time.strftime("%Y-%m-%dT%H:%M:%SZ ", time.gmtime()), message, '\n'))
    logf.flush()

def status(message):
    """display and log normal progress message"""
    log(message)

    global FRAME
    if not FRAME:
        FRAME = MessageFrame(message, TITLE)
    else:
        FRAME.set(message)

def fail(message):
    """log message, produce error box, then terminate with nonzero rc"""
    log(message)

    # If we haven't yet called status() (we don't yet have a FRAME), perform a
    # bit of trickery to bypass the spurious "main window" that Tkinter would
    # otherwise pop up if the first call is showerror().
    if not FRAME:
        root = Tkinter.Tk()
        root.withdraw()

    # If we do have a LOGF available, mention it in the error box.
    if LOGF:
        message = "%s\n(Updater log in %s)" % (message, LOGF.name)

    # We explicitly specify the WARNING icon because, at least on the Tkinter
    # bundled with the system-default Python 2.7 on Mac OS X 10.7.4, the
    # ERROR, QUESTION and INFO icons are all the silly Tk rocket ship. At
    # least WARNING has an exclamation in a yellow triangle, even though
    # overlaid by a smaller image of the rocket ship.
    tkMessageBox.showerror(TITLE,
"""An error occurred while updating Second Life:
%s
Please download the latest viewer from www.secondlife.com.""" % message,
                           icon=tkMessageBox.WARNING)
    sys.exit(1)

def exception(err):
    """call fail() with an exception instance"""
    fail("%s exception: %s" % (err.__class__.__name__, str(err)))

def excepthook(type, value, traceback):
    """
    Store this hook function into sys.excepthook until we have a logfile.
    """
    # At least in older Python versions, it could be tricky to produce a
    # string from 'type' and 'value'. For instance, an OSError exception would
    # pass type=OSError and value=some_tuple. Empirically, this funky
    # expression seems to work.
    exception(type(*value))
sys.excepthook = excepthook

class ExceptHook(object):
    """
    Store an instance of this class into sys.excepthook once we have a logfile
    open.
    """
    def __init__(self, logfile):
        # There's no magic to the cgitb.enable() function -- it merely stores
        # an instance of cgitb.Hook into sys.excepthook, passing enable()'s
        # params into Hook.__init__(). Sadly, enable() doesn't forward all its
        # params using (*args, **kwds) syntax -- another story. But the point
        # is that all the goodness is in the cgitb.Hook class. Capture an
        # instance.
        self.hook = cgitb.Hook(file=logfile, format="text")

    def __call__(self, type, value, traceback):
        # produce nice text traceback to logfile
        self.hook(type, value, traceback)
        # Now display an error box.
        excepthook(type, value, traceback)

def write_marker(markerfile, markertext):
    log("writing %r to %s" % (markertext, markerfile))
    try:
        with open(markerfile, "w") as markerf:
            markerf.write(markertext)
    except IOError, err:
        # write_marker() is invoked by fail(), and fail() is invoked by other
        # error-handling functions. If we try to invoke any of those, we'll
        # get infinite recursion. If for any reason we can't write markerfile,
        # try to log it -- otherwise shrug.
        log("%s exception: %s" % (err.__class__.__name__, err))

# ****************************************************************************
#   Main script logic
# ****************************************************************************
def main(dmgfile, markerfile, markertext, appdir=None):
    # Should we fail, we're supposed to write 'markertext' to 'markerfile'.
    # Wrap the fail() function so we do that.
    global fail
    oldfail = fail
    def fail(message):
        write_marker(markerfile, markertext)
        oldfail(message)

    try:
        # Starting with the Cocoafied viewer, we'll find viewer logs in
        # ~/Library/Application Support/$CFBundleIdentifier/logs rather than in
        # ~/Library/Application Support/SecondLife/logs as before. This could be
        # obnoxious -- but we Happen To Know that markerfile is a path specified
        # within the viewer's logs directory. Use that.
        logsdir = os.path.dirname(markerfile)

        # Move the old updater.log file out of the way
        logname = os.path.join(logsdir, "updater.log")
        try:
            os.rename(logname, logname + ".old")
        except OSError, err:
            # Nonexistence is okay. Anything else, not so much.
            if err.errno != errno.ENOENT:
                raise

        # Open new updater.log.
        global LOGF
        LOGF = open(logname, "w")

        # Now that LOGF is in fact open for business, use it to log any further
        # uncaught exceptions.
        sys.excepthook = ExceptHook(LOGF)

        # log how this script was invoked
        log(' '.join(repr(arg) for arg in sys.argv))

        # prepare for other cleanup
        with Janitor(LOGF) as janitor:

            # Hopefully caller explicitly stated the viewer bundle to update.
            # But if not, try to derive it from our own pathname. (The only
            # trouble with that is that the old viewer might copy this script
            # to a temp dir before running.)
            if not appdir:
                # Somewhat peculiarly, this script is currently packaged in
                # Appname.app/Contents/MacOS with the viewer executable. But even if we
                # decide to move it to Appname.app/Contents/Resources, we'll still find
                # Appname.app two levels up from dirname(__file__).
                appdir = os.path.abspath(os.path.join(os.path.dirname(__file__),
                                                      os.pardir, os.pardir))
            if not appdir.endswith(".app"):
                fail(appdir + " is not an application directory")

            # We need to install into appdir's parent directory -- can we?
            installdir = os.path.abspath(os.path.join(appdir, os.pardir))
            if not os.access(installdir, os.W_OK):
                fail("Can't modify " + installdir)

            # invent a temporary directory
            tempdir = tempfile.mkdtemp()
            log("created " + tempdir)
            # clean it up when we leave
            janitor.later(shutil.rmtree, tempdir)

            status("Mounting image...")

            mntdir = os.path.join(tempdir, "mnt")
            log("mkdir " + mntdir)
            os.mkdir(mntdir)
            command = ["hdiutil", "attach", dmgfile, "-mountpoint", mntdir]
            log(' '.join(command))
            # Instantiating subprocess.Popen launches a child process with the
            # specified command line. stdout=PIPE passes a pipe to its stdout.
            hdiutil = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=LOGF)
            # Popen.communicate() reads that pipe until the child process
            # terminates, returning (stdout, stderr) output. Select just stdout.
            hdiutil_out = hdiutil.communicate()[0]
            if hdiutil.returncode != 0:
                fail("Couldn't mount " + dmgfile)
            # hdiutil should report the devnode. Find that.
            found = re.search(r"/dev/[^ ]*\b", hdiutil_out)
            if not found:
                # If we don't spot the devnode, log it and continue -- we only
                # use it to detach it. Don't fail the whole update if we can't
                # clean up properly.
                log("Couldn't spot devnode in hdiutil output:\n" + hdiutil_out)
            else:
                # If we do spot the devnode, detach it when done.
                janitor.later(subprocess.call, ["hdiutil", "detach", found.group(0)],
                              stdout=LOGF, stderr=subprocess.STDOUT)

            status("Searching for app bundle...")

            for candidate in glob.glob(os.path.join(mntdir, "*.app")):
                log("Considering " + candidate)
                try:
                    # By convention, a valid Mac app bundle has a
                    # Contents/Info.plist file containing at least
                    # CFBundleIdentifier.
                    CFBundleIdentifier = \
                        plistlib.readPlist(os.path.join(candidate, "Contents",
                                                        "Info.plist"))["CFBundleIdentifier"]
                except Exception, err:
                    # might be IOError, xml.parsers.expat.ExpatError, KeyError
                    # Any of these means it's not a valid app bundle. Instead
                    # of aborting, just skip this candidate and continue.
                    log("%s not a valid app bundle: %s: %s" %
                        (candidate, err.__class__.__name__, err))
                    continue

                if CFBundleIdentifier == BUNDLE_IDENTIFIER:
                    break

                log("unrecognized CFBundleIdentifier: " + CFBundleIdentifier)

            else:
                fail("Could not find Second Life viewer in " + dmgfile)

            # Here 'candidate' is the new viewer to install
            log("Found " + candidate)
            status("Preparing to copy files...")

            # move old viewer to temp location in case copy from .dmg fails
            aside = os.path.join(tempdir, os.path.basename(appdir))
            log("mv %r %r" % (appdir, aside))
            # Use shutil.move() instead of os.rename(). move() first tries
            # os.rename(), but falls back to shutil.copytree() if the dest is
            # on a different filesystem.
            shutil.move(appdir, aside)

            status("Copying files...")

            # shutil.copytree()'s target must not already exist. But we just
            # moved appdir out of the way.
            log("cp -p %r %r" % (candidate, appdir))
            try:
                # The viewer app bundle does include internal symlinks. Keep them
                # as symlinks.
                shutil.copytree(candidate, appdir, symlinks=True)
            except Exception, err:
                # copy failed -- try to restore previous viewer before crumping
                type, value, traceback = sys.exc_info()
                log("exception response: mv %r %r" % (aside, appdir))
                shutil.move(aside, appdir)
                # let our previously-set sys.excepthook handle this
                raise type, value, traceback

            status("Clearing cache...")

            # We don't know whether the previous viewer was old-style or
            # new-style (Cocoa). Clear both kinds of caches.
            for cachesubdir in "SecondLife", BUNDLE_IDENTIFIER:
                wildcard = "~/Library/Caches/%s/*" % cachesubdir
                log("rm " + wildcard)
                for f in glob.glob(os.path.expanduser(wildcard)):
                    # Don't try to remove subdirs this way
                    if os.path.isfile(f):
                        try:
                            os.remove(f)
                        except Exception, err:
                            log("%s removing %s: %s" % (err.__class__.__name__, f, err))

            status("Cleaning up...")

            log("touch " + appdir)
            os.utime(appdir, None)      # set to current time

            command = ["open", appdir]
            log(' '.join(command))
            subprocess.check_call(command, stdout=LOGF, stderr=subprocess.STDOUT)

    except Exception, err:
        # Because we carefully set sys.excepthook -- and even modify it to log
        # the problem once we have our log file open -- you might think we
        # could just let exceptions propagate. But when we do that, on
        # exception in this block, we FIRST restore the no-side-effects fail()
        # and THEN implicitly call sys.excepthook(), which calls the (no-side-
        # effects) fail(). Explicitly call sys.excepthook() BEFORE restoring
        # fail(). Only then do we get the enriched fail() behavior.
        sys.excepthook(*sys.exc_info())

    finally:
        # When we leave main() -- for whatever reason -- reset fail() the way
        # it was before, because the bound markerfile, markertext params
        # passed to this main() call are no longer applicable.
        fail = oldfail

if __name__ == "__main__":
    # We expect this script to be invoked with:
    # - the pathname to the .dmg we intend to install;
    # - the pathname to an update-error marker file to create on failure;
    # - the content to write into the marker file;
    # - optionally, the pathname of the Second Life viewer to update.
    main(*sys.argv[1:])
