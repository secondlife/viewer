#!/usr/bin/python
"""\
@file   slp_conv.py
@author Callum Prentice
@date   2021-01-26
@brief  Convert a Second Life Performance (SLP) file generated
        by the Viewer into an comma separated value (CSV) file
        for import into spreadsheets and other data analytics tools.

$LicenseInfo:firstyear=2018&license=internal$
Copyright (c) 2018, Linden Research, Inc.
$/LicenseInfo$
"""
from llbase import llsd
import argparse

parser = argparse.ArgumentParser(
    description="Converts Viewer SLP files into CSV for import into spreadsheets etc."
)
parser.add_argument(
    "infilename",
    help="Name of SLP file to read",
)
parser.add_argument(
    "outfilename",
    help="Name of CSV file to create",
)
args = parser.parse_args()

with open(args.infilename, "r") as slp_file:
    slps = slp_file.readlines()
    print "Reading from %s - %d items" % (args.infilename, len(slps))

    with open(args.outfilename, "w") as csv_file:

        print "Writing to %s" % args.outfilename

        for index, each_slp in enumerate(slps):
            slp_entry = llsd.parse(each_slp)

            first_key = slp_entry.keys()[0]

            # first entry so write column headers
            if index == 0:
                line = ""
                for key, value in slp_entry[first_key].iteritems():
                    line += key
                    line += ", "
                csv_file.write("entry, %s, \n" % line)
            # write line of data
            line = ""
            for key, value in slp_entry[first_key].iteritems():
                line += str(value)
                line += ", "
            csv_file.write("%s, %s, \n" % (first_key, str(line)))
