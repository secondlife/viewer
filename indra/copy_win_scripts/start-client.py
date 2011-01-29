#!/usr/bin/python
import sys, getopt
import os
import llstart

def usage():
    print """start-client.py
    
    --grid <grid>
    --farm <grid>
    --region <starting region name>
    """

def start_client(grid, slurl, build_config, my_args):
    login_url = "https://login.%s.lindenlab.com/cgi-bin/login.cgi" % (grid)

    viewer_args = { "--grid" : grid,
                    "--loginuri" : login_url }
    viewer_args.update(my_args)
    # *sigh*  We must put --url at the end of the argument list.
    if viewer_args.has_key("--url"):
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
    print >>f, "Viewer startup arguments:"
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
