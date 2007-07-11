#!/usr/bin/python
# @file setup-path.py
# @brief Get the python library directory in the path, so we don't have
# to screw with PYTHONPATH or symbolic links.
#
# Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
# $License$

import sys
from os.path import realpath, dirname, join

# Walk back to checkout base directory
dir = dirname(dirname(realpath(__file__)))
# Walk in to libraries directory
dir = join(join(join(dir, 'indra'), 'lib'), 'python')

if dir not in sys.path:
    sys.path.insert(0, dir)
