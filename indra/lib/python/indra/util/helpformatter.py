"""\
@file helpformatter.py
@author Phoenix
@brief Class for formatting optparse descriptions.

Copyright (c) 2007, Linden Research, Inc.
$License$
"""

import optparse
import textwrap

class Formatter(optparse.IndentedHelpFormatter):
    def __init__(
        self, 
        p_indentIncrement = 2, 
        p_maxHelpPosition = 24,
        p_width = 79,
        p_shortFirst = 1) :
        optparse.HelpFormatter.__init__(
            self, 
            p_indentIncrement,
            p_maxHelpPosition, 
            p_width, 
            p_shortFirst)
    def format_description(self, p_description):
        t_descWidth = self.width - self.current_indent
        t_indent = " " * (self.current_indent + 2)
        return "\n".join(
            [textwrap.fill(descr, t_descWidth, initial_indent = t_indent,
                           subsequent_indent = t_indent)
             for descr in p_description.split("\n")] )
