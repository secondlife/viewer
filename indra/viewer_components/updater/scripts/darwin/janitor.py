#!/usr/bin/python
"""\
@file   janitor.py
@author Nat Goodspeed
@date   2011-09-14
@brief  Janitor class to clean up arbitrary resources

2013-01-04 cloned from vita because it's exactly what update_install.py needs.

$LicenseInfo:firstyear=2011&license=viewerlgpl$
Copyright (c) 2011, Linden Research, Inc.
$/LicenseInfo$
"""

import sys
import functools
import itertools

class Janitor(object):
    """
    Usage:

    Basic:
    self.janitor = Janitor(sys.stdout) # report cleanup actions on stdout
    ...
    self.janitor.later(os.remove, some_temp_file)
    self.janitor.later(os.remove, some_other_file)
    ...
    self.janitor.cleanup()          # perform cleanup actions

    Context Manager:
    with Janitor() as janitor:      # clean up quietly
        ...
        janitor.later(shutil.rmtree, some_temp_directory)
        ...
    # exiting 'with' block performs cleanup

    Test Class:
    class TestMySoftware(unittest.TestCase, Janitor):
        def __init__(self):
            Janitor.__init__(self)  # quiet cleanup
            ...

        def setUp(self):
            ...
            self.later(os.rename, saved_file, original_location)
            ...

        def tearDown(self):
            Janitor.tearDown(self)  # calls cleanup()
            ...
            # Or, if you have no other tearDown() logic for
            # TestMySoftware, you can omit the TestMySoftware.tearDown()
            # def entirely and let it inherit Janitor.tearDown().
    """
    def __init__(self, stream=None):
        """
        If you pass stream= (e.g.) sys.stdout or sys.stderr, Janitor will
        report its cleanup operations as it performs them. If you don't, it
        will perform them quietly -- unless one or more of the actions throws
        an exception, in which case you'll get output on stderr.
        """
        self.stream   = stream
        self.cleanups = []

    def later(self, func, *args, **kwds):
        """
        Pass the callable you want to call at cleanup() time, plus any
        positional or keyword args you want to pass it.
        """
        # Get a name string for 'func'
        try:
            # A free function has a __name__
            name = func.__name__
        except AttributeError:
            try:
                # A class object (even builtin objects like ints!) support
                # __class__.__name__
                name = func.__class__.__name__
            except AttributeError:
                # Shrug! Just use repr() to get a string describing this func.
                name = repr(func)
        # Construct a description of this operation in Python syntax from
        # args, kwds.
        desc = "%s(%s)" % \
               (name, ", ".join(itertools.chain((repr(a) for a in args),
                                                ("%s=%r" % (k, v) for (k, v) in kwds.iteritems()))))
        # Use functools.partial() to bind passed args and keywords to the
        # passed func so we get a nullary callable that does what caller
        # wants.
        bound = functools.partial(func, *args, **kwds)
        self.cleanups.append((desc, bound))

    def cleanup(self):
        """
        Perform all the actions saved with later() calls.
        """
        # Typically one allocates resource A, then allocates resource B that
        # depends on it. In such a scenario it's appropriate to delete B
        # before A -- so perform cleanup actions in reverse order. (This is
        # the same strategy used by atexit().)
        while self.cleanups:
            # Until our list is empty, pop the last pair.
            desc, bound = self.cleanups.pop(-1)

            # If requested, report the action.
            if self.stream is not None:
                print >>self.stream, desc

            try:
                # Call the bound callable
                bound()
            except Exception, err:
                # This is cleanup. Report the problem but continue.
                print >>(self.stream or sys.stderr), "Calling %s\nraised  %s: %s" % \
                      (desc, err.__class__.__name__, err)

    def tearDown(self):
        """
        If a unittest.TestCase subclass (or a nose test class) adds Janitor as
        one of its base classes, and has no other tearDown() logic, let it
        inherit Janitor.tearDown().
        """
        self.cleanup()

    def __enter__(self):
        return self

    def __exit__(self, type, value, tb):
        # Perform cleanup no matter how we exit this 'with' statement
        self.cleanup()
        # Propagate any exception from the 'with' statement, don't swallow it
        return False
