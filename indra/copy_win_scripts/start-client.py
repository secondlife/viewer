#!/usr/bin/env python3
"""\
@file   start-client.py

$LicenseInfo:firstyear=2010&license=viewerlgpl$
Second Life Viewer Source Code
Copyright (C) 2010-2011, Linden Research, Inc.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation;
version 2.1 of the License only.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
$/LicenseInfo$
"""
import sys, getopt
import os
import llstart

def usage():
    print("""start-client.py

    --grid <grid>
    --farm <grid>
    --region <starting region name>
    """)

def start_client(grid, slurl, build_config, my_args):
    login_url = "https://login.%s.lindenlab.com/cgi-bin/login.cgi" % (grid)

    viewer_args = { "--grid" : grid,
                    "--loginuri" : login_url }
    viewer_args.update(my_args)
    # *sigh*  We must put --url at the end of the argument list.
    if "--url" in viewer_args:
        slurl = viewer_args["--url"]
        del(viewer_args["--url"])
    viewer_args = llstart.get_args_from_dict(viewer_args)
    if slurl is not None:
        viewer_args += " --url %s" % slurl

    # Figure out path stuff.
    # The client should run from indra/newview
    # but the exe is at indra/build-<xxx>/newview/<target>
    build_path = os.path.dirname(os.getcwd());
    f = open("start-client.log", "w")
    print("Viewer startup arguments:", file=f)
    llstart.start("viewer", "../../newview",
        "%s/newview/%s/secondlife-bin.exe" % (build_path, build_config),
        viewer_args, f)
    f.close()

if __name__ == "__main__":
    grid = llstart.get_config("grid")

    if grid == None:
        grid = "aditi"

    build_config = llstart.get_config("build_config")
    my_args = llstart.get_config("viewer_args", force_dict = True)
    opts, args = getopt.getopt(sys.argv[1:], "u:r:f:g:i:h",
        ["region=", "username=", "farm=", "grid=", "ip=", "help"])
    region = None
    for o, a in opts:
        if o in ("-r", "--region", "-u", "--username"):
            region = a
        if o in ("-f", "--farm", "-g", "--grid"):
            grid = a
        if o in ("-h", "--help"):
            usage()
            sys.exit(0)

    slurl = llstart.get_config("slurl")
    if slurl == None:
        if region is None:
            region = llstart.get_user_name()
        slurl = "//%s/128/128/" % (region)
    # Ensure the slurl has quotes around it.
    if slurl is not None:
        slurl = '"%s"' % (slurl.strip('"\''))

    start_client(grid, slurl, build_config, my_args)
