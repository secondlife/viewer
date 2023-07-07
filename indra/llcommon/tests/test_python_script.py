#!/usr/bin/env python3
"""\
@file   test_python_script.py
@author Nat Goodspeed
@date   2023-07-07
@brief  Work around a problem running Python within integration tests on GitHub
        Windows runners.

$LicenseInfo:firstyear=2023&license=viewerlgpl$
Copyright (c) 2023, Linden Research, Inc.
$/LicenseInfo$
"""

import os
import sys

# use pop() so that if the referenced script in turn looks at sys.argv, it
# will see its arguments rather than its own filename
_script = sys.argv.pop(1)
exec(open(_script).read())
