#!/usr/bin/python
"""\
@file   messageframe.py
@author Nat Goodspeed
@date   2013-01-03
@brief  Define MessageFrame class for popping up messages from a command-line
        script.

$LicenseInfo:firstyear=2013&license=viewerlgpl$
Copyright (c) 2013, Linden Research, Inc.
$/LicenseInfo$
"""

import Tkinter as tk
import os

# Tricky way to obtain the filename of the main script (default title string)
import __main__

# This class is intended for displaying messages from a command-line script.
# Getting the base class right took a bit of trial and error.
# If you derive from tk.Frame, the destroy() method doesn't actually close it.
# If you derive from tk.Toplevel, it pops up a separate Tk frame too. destroy()
# closes this frame, but not that one.
# Deriving from tk.Tk appears to do the right thing.
class MessageFrame(tk.Tk):
    def __init__(self, text="", title=os.path.splitext(os.path.basename(__main__.__file__))[0],
                 width=320, height=120):
        tk.Tk.__init__(self)
        self.grid()
        self.title(title)
        self.var = tk.StringVar()
        self.var.set(text)
        self.msg = tk.Label(self, textvariable=self.var)
        self.msg.grid()
        # from http://stackoverflow.com/questions/3352918/how-to-center-a-window-on-the-screen-in-tkinter :
        self.update_idletasks()

        # The constants below are to adjust for typical overhead from the
        # frame borders.
        xp = (self.winfo_screenwidth()  / 2) - (width  / 2) - 8
        yp = (self.winfo_screenheight() / 2) - (height / 2) - 20
        self.geometry('{0}x{1}+{2}+{3}'.format(width, height, xp, yp))
        self.update()

    def set(self, text):
        self.var.set(text)
        self.update()

if __name__ == "__main__":
    # When run as a script, just test the MessageFrame.
    import sys
    import time

    frame = MessageFrame("something in the way she moves....")
    time.sleep(3)
    frame.set("smaller")
    time.sleep(3)
    frame.set("""this has
several
lines""")
    time.sleep(3)
    frame.destroy()
    print "Destroyed!"
    sys.stdout.flush()
    time.sleep(3)
