#!/usr/bin/env python

# $LicenseInfo:firstyear=2016&license=internal$
# 
# Copyright (c) 2016, Linden Research, Inc.
# 
# The following source code is PROPRIETARY AND CONFIDENTIAL. Use of
# this source code is governed by the Linden Lab Source Code Disclosure
# Agreement ("Agreement") previously entered between you and Linden
# Lab. By accessing, using, copying, modifying or distributing this
# software, you acknowledge that you have been informed of your
# obligations under the Agreement and agree to abide by those obligations.
# 
# ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
# WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
# COMPLETENESS OR PERFORMANCE.
# $LicenseInfo:firstyear=2013&license=viewerlgpl$
# Copyright (c) 2013, Linden Research, Inc.
# $/LicenseInfo$

"""
@file   InstallerUserMessage.py
@author coyot
@date   2016-05-16
"""

"""
This does everything the old updater/scripts/darwin/messageframe.py script did and some more bits.  
Pushed up the manager directory to be multiplatform.
"""

import os
import Queue
import threading
import time
import Tkinter as tk
import ttk


class InstallerUserMessage(tk.Tk):
    #Goals for this class:
    #  Provide a uniform look and feel
    #  Provide an easy to use convenience class for other scripts
    #  Provide windows that automatically disappear when done (for differing notions of done)
    #  Provide a progress bar that isn't a glorified spinner, but based on download progress
    #Non-goals:
    #  No claim to threadsafety is made or warranted.  Your mileage may vary. 
    #     Please consult a doctor if you experience thread pain.

    #Linden standard green color, from Marketing
    linden_green = "#487A7B"

    def __init__(self, text="", title="", width=500, height=200, icon_name = None, icon_path = None):
        tk.Tk.__init__(self)
        self.grid()
        self.title(title)
        self.choice = tk.BooleanVar()
        self.config(background = 'black')
        # background="..." doesn't work on MacOS for radiobuttons or progress bars
        # http://tinyurl.com/tkmacbuttons
        ttk.Style().configure('Linden.TLabel', foreground=InstallerUserMessage.linden_green, background='black')
        ttk.Style().configure('Linden.TButton', foreground=InstallerUserMessage.linden_green, background='black')
        ttk.Style().configure("black.Horizontal.TProgressbar", foreground=InstallerUserMessage.linden_green, background='black')

        #This bit of configuration centers the window on the screen
        # The constants below are to adjust for typical overhead from the
        # frame borders.
        self.xp = (self.winfo_screenwidth()  / 2) - (width  / 2) - 8
        self.yp = (self.winfo_screenheight() / 2) - (height / 2) - 20
        self.geometry('{0}x{1}+{2}+{3}'.format(width, height, self.xp, self.yp))

        #find a few things
        self.script_dir = os.path.dirname(os.path.realpath(__file__))
        self.icon_dir = os.path.abspath(os.path.join(self.script_dir, 'icons'))

        #finds the icon and creates the widget
        self.find_icon(icon_path, icon_name)

        #defines what to do when window is closed
        self.protocol("WM_DELETE_WINDOW", self._delete_window)

    def _delete_window(self):
        #capture and discard all destroy events before the choice is set
        if not ((self.choice == None) or (self.choice == "")):
            try:
                self.destroy()
            except:
                #tk may try to destroy the same object twice
                pass

    def set_colors(self, widget):
        # #487A7B is "Linden Green"
        widget.config(foreground = InstallerUserMessage.linden_green)
        widget.config(background='black') 

    def find_icon(self, icon_path = None, icon_name = None):
        #we do this in each message, let's do it just once instead.
        if not icon_path:
            icon_path = self.icon_dir
        icon_path = os.path.join(icon_path, icon_name)
        if os.path.exists(icon_path):
            icon = tk.PhotoImage(file=icon_path)
            self.image_label = tk.Label(image = icon)
            self.image_label.image = icon
        else:
            #default to text if image not available
            self.image_label = tk.Label(text = "Second Life")

    def auto_resize(self, row_count = 0, column_count = 0, heavy_row = None, heavy_column = None):
        #auto resize window to fit all rows and columns
        #"heavy" gets extra weight
        for x in range(column_count):
            if x == heavy_column:
                self.columnconfigure(x, weight = 2)
            else:
                self.columnconfigure(x, weight=1)

        for y in range(row_count):
            if y == heavy_row:
                self.rowconfigure(y, weight = 2)
            else:
                self.rowconfigure(x, weight=1)

    def basic_message(self, message):
        #message: text to be displayed
        #icon_path: directory holding the icon, defaults to icons subdir of script dir
        #icon_name: filename of icon to be displayed
        self.choice.set(True)
        self.text_label = tk.Label(text = message)
        self.set_colors(self.text_label)
        self.set_colors(self.image_label)
        #pad, direction and weight are all experimentally derived by retrying various values
        self.image_label.grid(row = 1, column = 1, sticky = 'W')
        self.text_label.grid(row = 1, column = 2, sticky = 'W', padx =100)
        self.auto_resize(row_count = 1, column_count = 2)
        self.mainloop()

    def binary_choice_message(self, message, true = 'Yes', false = 'No'):
        #true: first option, returns True
        #false: second option, returns False
        #usage is kind of opaque and relies on this object persisting after the window destruction to pass back choice
        #usage:
        #   frame = InstallerUserMessage.InstallerUserMessage( ... )
        #   frame = frame.binary_choice_message( ... )
        #   (wait for user to click)
        #   value = frame.choice.get()

        self.text_label = tk.Label(text = message)
        #command registers the callback to the method named.  We want the frame to go away once clicked.
        #button 1 returns True/1, button 2 returns False/0
        self.button_one = ttk.Radiobutton(text = true, variable = self.choice, value = True, 
            command = self._delete_window, style = 'Linden.TButton')
        self.button_two = ttk.Radiobutton(text = false, variable = self.choice, value = False, 
            command = self._delete_window, style = 'Linden.TButton')
        self.set_colors(self.text_label)
        self.set_colors(self.image_label)
        #pad, direction and weight are all experimentally derived by retrying various values
        self.image_label.grid(row = 1, column = 1, rowspan = 3, sticky = 'W')
        self.text_label.grid(row = 1, column = 2, rowspan = 3)
        self.button_one.grid(row = 1, column = 3, sticky = 'W', pady = 40)
        self.button_two.grid(row = 2, column = 3, sticky = 'W', pady = 0)
        self.auto_resize(row_count = 2, column_count = 3, heavy_column = 3)
        #self.button_two.deselect()
        self.update()
        self.mainloop()

    def trinary_choice_message(self, message, one = 1, two = 2, three = 3):
        #one: first option, returns 1
        #two: second option, returns 2
        #three: third option, returns 3
        #usage is kind of opaque and relies on this object persisting after the window destruction to pass back choice
        #usage:
        #   frame = InstallerUserMessage.InstallerUserMessage( ... )
        #   frame = frame.binary_choice_message( ... )
        #   (wait for user to click)
        #   value = frame.choice.get()

        self.text_label = tk.Label(text = message)
        #command registers the callback to the method named.  We want the frame to go away once clicked.
        self.button_one = ttk.Radiobutton(text = one, variable = self.choice, value = 1, 
            command = self._delete_window, style = 'Linden.TButton')
        self.button_two = ttk.Radiobutton(text = two, variable = self.choice, value = 2, 
            command = self._delete_window, style = 'Linden.TButton')
        self.button_three = ttk.Radiobutton(text = three, variable = self.choice, value = 3, 
            command = self._delete_window, style = 'Linden.TButton')
        self.set_colors(self.text_label)
        self.set_colors(self.image_label)
        #pad, direction and weight are all experimentally derived by retrying various values
        self.image_label.grid(row = 1, column = 1, rowspan = 4, sticky = 'W')
        self.text_label.grid(row = 1, column = 2, rowspan = 4, padx = 5)
        self.button_one.grid(row = 1, column = 3, sticky = 'W', pady = 5)
        self.button_two.grid(row = 2, column = 3, sticky = 'W', pady = 5)
        self.button_three.grid(row = 3, column = 3, sticky = 'W', pady = 5)
        self.auto_resize(row_count = 3, column_count = 3, heavy_column = 3)
        #self.button_two.deselect()
        self.update()
        self.mainloop()

    def progress_bar(self, message = None, size = 0, interval = 100, pb_queue = None):
        #Best effort attempt at a real progress bar
        #  This is what Tk calls "determinate mode" rather than "indeterminate mode"
        #size: denominator of percent complete
        #interval: frequency, in ms, of how often to poll the file for progress
        #pb_queue: queue object used to send updates to the bar
        self.text_label = tk.Label(text = message)
        self.set_colors(self.text_label)
        self.set_colors(self.image_label)
        self.image_label.grid(row = 1, column = 1, sticky = 'NSEW')
        self.text_label.grid(row = 2, column = 1, sticky = 'NSEW')
        self.progress = ttk.Progressbar(self, style = 'black.Horizontal.TProgressbar', orient="horizontal", length=100, mode="determinate")
        self.progress.grid(row = 3, column = 1, sticky = 'NSEW')
        self.value = 0
        self.progress["maximum"] = size
        self.auto_resize(row_count = 1, column_count = 3)
        self.queue = pb_queue
        self.check_scheduler()

    def check_scheduler(self):
        if self.value < self.progress["maximum"]:
            self.check_queue()
            self.after(100, self.check_scheduler)

    def check_queue(self):
        while self.queue.qsize():
            try:
                msg = float(self.queue.get(0))
                #custom signal, time to tear down
                if msg == -1:
                    self.choice.set(True)
                    self.destroy()
                else:
                    self.progress.step(msg)
                    self.value = msg
            except Queue.Empty:
                #nothing to do
                return

class ThreadedClient(threading.Thread):
    #for test only, not part of the functional code
    def __init__(self, queue):
        threading.Thread.__init__(self)
        self.queue = queue

    def run(self):
        for x in range(1, 90, 10):
            time.sleep(1)
            print "run " + str(x)
            self.queue.put(10)
        #tkk progress bars wrap at exactly 100 percent, look full at 99%
        print "leftovers"
        self.queue.put(9)
        time.sleep(5)
        # -1 is a custom signal to the progress_bar to quit
        self.queue.put(-1)

if __name__ == "__main__":
    #When run as a script, just test the InstallUserMessage.  
    #To proceed with the test, close the first window, select on the second.  The third will close by itself.
    import sys
    import tempfile

    def set_and_check(frame, value):
        print "value: " + str(value)
        frame.progress.step(value)
        if frame.progress["value"] < frame.progress["maximum"]:
            print "In Progress"
        else:
            print "Over now"

    #basic message window test
    frame2 = InstallerUserMessage(text = "Something in the way she moves....", title = "Beatles Quotes for 100", icon_name="head-sl-logo.gif")
    frame2.basic_message(message = "...attracts me like no other.")
    print "Destroyed!"
    sys.stdout.flush()

    #binary choice test.  User destroys window when they select.
    frame3 = InstallerUserMessage(text = "Something in the way she knows....", title = "Beatles Quotes for 200", icon_name="head-sl-logo.gif")
    frame3.binary_choice_message(message = "And all I have to do is think of her.", 
        true = "Don't want to leave her now", false = 'You know I believe and how')
    print frame3.choice.get()
    sys.stdout.flush()

    #progress bar
    queue = Queue.Queue()
    thread = ThreadedClient(queue)
    thread.start()
    print "thread started"

    frame4 = InstallerUserMessage(text = "Something in the way she knows....", title = "Beatles Quotes for 300", icon_name="head-sl-logo.gif")
    frame4.progress_bar(message = "You're asking me will my love grow", size = 100, pb_queue = queue)
    print "frame defined"
    frame4.mainloop()

    #trinary choice test.  User destroys window when they select.
    frame3a = InstallerUserMessage(text = "Something in the way she knows....", title = "Beatles Quotes for 200", icon_name="head-sl-logo.gif")
    frame3a.trinary_choice_message(message = "And all I have to do is think of her.", 
        one = "Don't want to leave her now", two = 'You know I believe and how', three = 'John is Dead')
    print frame3a.choice.get()
    sys.stdout.flush()
