"""\
@file llmanifest.py
@author Ryan Williams
@brief Library for specifying operations on a set of files.

Copyright (c) 2007, Linden Research, Inc.
$License$
"""

import commands
import filecmp
import fnmatch
import getopt
import glob
import os
import os.path
import re
import shutil
import sys
import tarfile

def path_ancestors(path):
    path = os.path.normpath(path)
    result = []
    while len(path) > 0:
        result.append(path)
        path, sub = os.path.split(path)
    return result

def proper_windows_path(path, current_platform = sys.platform):
    """ This function takes an absolute Windows or Cygwin path and
    returns a path appropriately formatted for the platform it's
    running on (as determined by sys.platform)"""
    path = path.strip()
    drive_letter = None
    rel = None
    match = re.match("/cygdrive/([a-z])/(.*)", path)
    if(not match):
        match = re.match('([a-zA-Z]):\\\(.*)', path)
    if(not match):
        return None         # not an absolute path
    drive_letter = match.group(1)
    rel = match.group(2)
    if(current_platform == "cygwin"):
        return "/cygdrive/" + drive_letter.lower() + '/' + rel.replace('\\', '/')
    else:
        return drive_letter.upper() + ':\\' + rel.replace('/', '\\')

def get_default_platform(dummy):
    return {'linux2':'linux',
            'linux1':'linux',
            'cygwin':'windows',
            'win32':'windows',
            'darwin':'darwin'
            }[sys.platform]

def get_default_version(srctree):
    # look up llversion.h and parse out the version info
    paths = [os.path.join(srctree, x, 'llversionviewer.h') for x in ['llcommon', '../llcommon', '../../indra/llcommon.h']]
    for p in paths:
        if os.path.exists(p):
            contents = open(p, 'r').read()
            major = re.search("LL_VERSION_MAJOR\s=\s([0-9]+)", contents).group(1)
            minor = re.search("LL_VERSION_MINOR\s=\s([0-9]+)", contents).group(1)
            patch = re.search("LL_VERSION_PATCH\s=\s([0-9]+)", contents).group(1)
            build = re.search("LL_VERSION_BUILD\s=\s([0-9]+)", contents).group(1)
            return major, minor, patch, build

def get_channel(srctree):
    # look up llversionserver.h and parse out the version info
    paths = [os.path.join(srctree, x, 'llversionviewer.h') for x in ['llcommon', '../llcommon', '../../indra/llcommon.h']]
    for p in paths:
        if os.path.exists(p):
            contents = open(p, 'r').read()
            channel = re.search("LL_CHANNEL\s=\s\"([\w\s]+)\"", contents).group(1)
            return channel
    

DEFAULT_CHANNEL = 'Second Life Release'

ARGUMENTS=[
    dict(name='actions',
         description="""This argument specifies the actions that are to be taken when the
        script is run.  The meaningful actions are currently:
          copy     - copies the files specified by the manifest into the
                     destination directory.
          package  - bundles up the files in the destination directory into
                     an installer for the current platform
          unpacked - bundles up the files in the destination directory into
                     a simple tarball
        Example use: %(name)s --actions="copy unpacked" """,
         default="copy package"),
    dict(name='arch',
         description="""This argument is appended to the platform string for
        determining which manifest class to run.
        Example use: %(name)s --arch=i686
        On Linux this would try to use Linux_i686Manifest.""",
         default=""),
    dict(name='configuration',
         description="""The build configuration used. Only used on OS X for
        now, but it could be used for other platforms as well.""",
         default="Universal"),
    dict(name='grid',
         description="""Which grid the client will try to connect to. Even
        though it's not strictly a grid, 'firstlook' is also an acceptable
        value for this parameter.""",
         default=""),
    dict(name='channel',
         description="""The channel to use for updates.""",
         default=get_channel),
    dict(name='installer_name',
         description=""" The name of the file that the installer should be
        packaged up into. Only used on Linux at the moment.""",
         default=None),
    dict(name='login_url',
         description="""The url that the login screen displays in the client.""",
         default=None),
    dict(name='platform',
         description="""The current platform, to be used for looking up which
        manifest class to run.""",
         default=get_default_platform),
    dict(name='version',
         description="""This specifies the version of Second Life that is
        being packaged up.""",
         default=get_default_version)
    ]

def usage(srctree=""):
    nd = {'name':sys.argv[0]}
    print """Usage:
    %(name)s [options] [destdir]
    Options:
    """ % nd
    for arg in ARGUMENTS:
        default = arg['default']
        if hasattr(default, '__call__'):
            default = "(computed value) \"" + str(default(srctree)) + '"'
        elif default is not None:
            default = '"' + default + '"'
        print "\t--%s        Default: %s\n\t%s\n" % (
            arg['name'],
            default,
            arg['description'] % nd)

def main(argv=None, srctree='.', dsttree='./dst'):
    if(argv == None):
        argv = sys.argv

    option_names = [arg['name'] + '=' for arg in ARGUMENTS]
    option_names.append('help')
    options, remainder = getopt.getopt(argv[1:], "", option_names)
    if len(remainder) >= 1:
        dsttree = remainder[0]

    print "Source tree:", srctree
    print "Destination tree:", dsttree

    # convert options to a hash
    args = {}
    for opt in options:
        args[opt[0].replace("--", "")] = opt[1]

    # early out for help
    if args.has_key('help'):
        # *TODO: it is a huge hack to pass around the srctree like this
        usage(srctree)
        return

    # defaults
    for arg in ARGUMENTS:
        if not args.has_key(arg['name']):
            default = arg['default']
            if hasattr(default, '__call__'):
                default = default(srctree)
            if default is not None:
                args[arg['name']] = default

    # fix up version
    if args.has_key('version') and type(args['version']) == str:
        args['version'] = args['version'].split('.')
        
    # default and agni are default
    if args['grid'] in ['default', 'agni']:
        args['grid'] = ''

    if args.has_key('actions'):
        args['actions'] = args['actions'].split()

    # debugging
    for opt in args:
        print "Option:", opt, "=", args[opt]

    wm = LLManifest.for_platform(args['platform'], args.get('arch'))(srctree, dsttree, args)
    wm.do(*args['actions'])
    return 0

class LLManifestRegistry(type):
    def __init__(cls, name, bases, dct):
        super(LLManifestRegistry, cls).__init__(name, bases, dct)
        match = re.match("(\w+)Manifest", name)
        if(match):
           cls.manifests[match.group(1).lower()] = cls

class LLManifest(object):
    __metaclass__ = LLManifestRegistry
    manifests = {}
    def for_platform(self, platform, arch = None):
        if arch:
            platform = platform + '_' + arch
        return self.manifests[platform.lower()]
    for_platform = classmethod(for_platform)

    def __init__(self, srctree, dsttree, args):
        super(LLManifest, self).__init__()
        self.args = args
        self.file_list = []
        self.excludes = []
        self.actions = []
        self.src_prefix = [srctree]
        self.dst_prefix = [dsttree]
        self.created_paths = []
        
    def default_grid(self):
        return self.args.get('grid', None) == ''
    def default_channel(self):
        return self.args.get('channel', None) == DEFAULT_CHANNEL

    def construct(self):
        """ Meant to be overriden by LLManifest implementors with code that
        constructs the complete destination hierarchy."""
        pass # override this method

    def exclude(self, glob):
        """ Excludes all files that match the glob from being included
        in the file list by path()."""
        self.excludes.append(glob)

    def prefix(self, src='', dst=None):
        """ Pushes a prefix onto the stack.  Until end_prefix is
        called, all relevant method calls (esp. to path()) will prefix
        paths with the entire prefix stack.  Source and destination
        prefixes can be different, though if only one is provided they
        are both equal.  To specify a no-op, use an empty string, not
        None."""
        if(dst == None):
            dst = src
        self.src_prefix.append(src)
        self.dst_prefix.append(dst)
        return True  # so that you can wrap it in an if to get indentation

    def end_prefix(self, descr=None):
        """Pops a prefix off the stack.  If given an argument, checks
        the argument against the top of the stack.  If the argument
        matches neither the source or destination prefixes at the top
        of the stack, then misnesting must have occurred and an
        exception is raised."""
        # as an error-prevention mechanism, check the prefix and see if it matches the source or destination prefix.  If not, improper nesting may have occurred.
        src = self.src_prefix.pop()
        dst = self.dst_prefix.pop()
        if descr and not(src == descr or dst == descr):
            raise ValueError, "End prefix '" + descr + "' didn't match '" +src+ "' or '" +dst + "'"

    def get_src_prefix(self):
        """ Returns the current source prefix."""
        return os.path.join(*self.src_prefix)

    def get_dst_prefix(self):
        """ Returns the current destination prefix."""
        return os.path.join(*self.dst_prefix)

    def src_path_of(self, relpath):
        """Returns the full path to a file or directory specified
        relative to the source directory."""
        return os.path.join(self.get_src_prefix(), relpath)

    def dst_path_of(self, relpath):
        """Returns the full path to a file or directory specified
        relative to the destination directory."""
        return os.path.join(self.get_dst_prefix(), relpath)

    def ensure_src_dir(self, reldir):
        """Construct the path for a directory relative to the
        source path, and ensures that it exists.  Returns the
        full path."""
        path = os.path.join(self.get_src_prefix(), reldir)
        self.cmakedirs(path)
        return path

    def ensure_dst_dir(self, reldir):
        """Construct the path for a directory relative to the
        destination path, and ensures that it exists.  Returns the
        full path."""
        path = os.path.join(self.get_dst_prefix(), reldir)
        self.cmakedirs(path)
        return path

    def run_command(self, command):
        """ Runs an external command, and returns the output.  Raises
        an exception if the command reurns a nonzero status code.  For
        debugging/informational purpoases, prints out the command's
        output as it is received."""
        print "Running command:", command
        fd = os.popen(command, 'r')
        lines = []
        while True:
            lines.append(fd.readline())
            if(lines[-1] == ''):
                break
            else:
                print lines[-1],
        output = ''.join(lines)
        status = fd.close()
        if(status):
            raise RuntimeError(
                "Command %s returned non-zero status (%s) \noutput:\n%s"
                % (command, status, output) )
        return output

    def created_path(self, path):
        """ Declare that you've created a path in order to
          a) verify that you really have created it
          b) schedule it for cleanup"""
        if not os.path.exists(path):
            raise RuntimeError, "Should be something at path " + path
        self.created_paths.append(path)

    def put_in_file(self, contents, dst):
        # write contents as dst
        f = open(self.dst_path_of(dst), "wb")
        f.write(contents)
        f.close()

    def replace_in(self, src, dst=None, searchdict={}):
        if(dst == None):
            dst = src
        # read src
        f = open(self.src_path_of(src), "rbU")
        contents = f.read()
        f.close()
        # apply dict replacements
        for old, new in searchdict.iteritems():
            contents = contents.replace(old, new)
        self.put_in_file(contents, dst)
        self.created_paths.append(dst)

    def copy_action(self, src, dst):
        if(src and (os.path.exists(src) or os.path.islink(src))):
            # ensure that destination path exists
            self.cmakedirs(os.path.dirname(dst))
            self.created_paths.append(dst)
            if(not os.path.isdir(src)):
                self.ccopy(src,dst)
            else:
                # src is a dir
                self.ccopytree(src,dst)
        else:
            print "Doesn't exist:", src

    def package_action(self, src, dst):
        pass

    def copy_finish(self):
        pass

    def package_finish(self):
        pass

    def unpacked_finish(self):
        unpacked_file_name = "unpacked_%(plat)s_%(vers)s.tar" % {
            'plat':self.args['platform'],
            'vers':'_'.join(self.args['version'])}
        print "Creating unpacked file:", unpacked_file_name
        # could add a gz here but that doubles the time it takes to do this step
        tf = tarfile.open(self.src_path_of(unpacked_file_name), 'w:')
        # add the entire installation package, at the very top level
        tf.add(self.get_dst_prefix(), "")
        tf.close()

    def cleanup_finish(self):
        """ Delete paths that were specified to have been created by this script"""
        for c in self.created_paths:
            # *TODO is this gonna be useful?
            print "Cleaning up " + c

    def process_file(self, src, dst):
        if(self.includes(src, dst)):
#            print src, "=>", dst
            for action in self.actions:
                methodname = action + "_action"
                method = getattr(self, methodname, None)
                if method is not None:
                    method(src, dst)
            self.file_list.append([src, dst])
        else:
            print "Excluding: ", src, dst


    def process_directory(self, src, dst):
        if(not self.includes(src, dst)):
            print "Excluding: ", src, dst
            return
        names = os.listdir(src)
        self.cmakedirs(dst)
        errors = []
        for name in names:
            srcname = os.path.join(src, name)
            dstname = os.path.join(dst, name)
            if os.path.isdir(srcname):
                self.process_directory(srcname, dstname)
            else:
                self.process_file(srcname, dstname)



    def includes(self, src, dst):
        if src:
            for excl in self.excludes:
                if fnmatch.fnmatch(src, excl):
                    return False
        return True

    def remove(self, *paths):
        for path in paths:
            if(os.path.exists(path)):
                print "Removing path", path
                if(os.path.isdir(path)):
                    shutil.rmtree(path)
                else:
                    os.remove(path)

    def ccopy(self, src, dst):
        """ Copy a single file or symlink.  Uses filecmp to skip copying for existing files."""
        if os.path.islink(src):
            linkto = os.readlink(src)
            if(os.path.islink(dst) or os.path.exists(dst)):
                os.remove(dst)  # because symlinking over an existing link fails
            os.symlink(linkto, dst)
        else:
            # Don't recopy file if it's up-to-date.
            # If we seem to be not not overwriting files that have been
            # updated, set the last arg to False, but it will take longer.
            if(os.path.exists(dst) and filecmp.cmp(src, dst, True)):
                return
            # only copy if it's not excluded
            if(self.includes(src, dst)):
                shutil.copy2(src, dst)

    def ccopytree(self, src, dst):
        """Direct copy of shutil.copytree with the additional
        feature that the destination directory can exist.  It
        is so dumb that Python doesn't come with this. Also it
        implements the excludes functionality."""
        if(not self.includes(src, dst)):
            return
        names = os.listdir(src)
        self.cmakedirs(dst)
        errors = []
        for name in names:
            srcname = os.path.join(src, name)
            dstname = os.path.join(dst, name)
            try:
                if os.path.isdir(srcname):
                    self.ccopytree(srcname, dstname)
                else:
                    self.ccopy(srcname, dstname)
                    # XXX What about devices, sockets etc.?
            except (IOError, os.error), why:
                errors.append((srcname, dstname, why))
        if errors:
            raise RuntimeError, errors


    def cmakedirs(self, path):
        """Ensures that a directory exists, and doesn't throw an exception
        if you call it on an existing directory."""
#        print "making path: ", path
        path = os.path.normpath(path)
        self.created_paths.append(path)
        if not os.path.exists(path):
            os.makedirs(path)

    def find_existing_file(self, *list):
        for f in list:
            if(os.path.exists(f)):
                return f
        # didn't find it, return last item in list
        if len(list) > 0:
            return list[-1]
        else:
            return None

    def contents_of_tar(self, src_tar, dst_dir):
        """ Extracts the contents of the tarfile (specified
        relative to the source prefix) into the directory
        specified relative to the destination directory."""
        self.check_file_exists(src_tar)
        tf = tarfile.open(self.src_path_of(src_tar), 'r')
        for member in tf.getmembers():
            tf.extract(member, self.ensure_dst_dir(dst_dir))
            # TODO get actions working on these dudes, perhaps we should extract to a temporary directory and then process_directory on it?
            self.file_list.append([src_tar,
                           self.dst_path_of(os.path.join(dst_dir,member.name))])
        tf.close()


    def wildcard_regex(self, src_glob, dst_glob):
 #       print "regex_pair:", src_glob, dst_glob
        src_re = re.escape(src_glob)
        src_re = src_re.replace('\*', '([-a-zA-Z0-9._ ]+)')
        dst_temp = dst_glob
        i = 1
        while(dst_temp.count("*") > 0):
            dst_temp = dst_temp.replace('*', '\g<' + str(i) + '>', 1)
            i = i+1
 #       print "regex_result:", src_re, dst_temp
        return re.compile(src_re), dst_temp

    def check_file_exists(self, path):
        if(not os.path.exists(path) and not os.path.islink(path)):
            raise RuntimeError("Path %s doesn't exist" % (
                os.path.normpath(os.path.join(os.getcwd(), path)),))


    wildcard_pattern = re.compile('\*')
    def expand_globs(self, src, dst):
        def fw_slash(str):
            return str.replace('\\', '/')
        def os_slash(str):
            return str.replace('/', os.path.sep)
        dst = fw_slash(dst)
        src = fw_slash(src)
        src_list = glob.glob(src)
        src_re, d_template = self.wildcard_regex(src, dst)
        for s in src_list:
            s = fw_slash(s)
            d = src_re.sub(d_template, s)
            #print "s:",s, "d_t", d_template, "dst", dst, "d", d
            yield os_slash(s), os_slash(d)

    def path(self, src, dst=None):
        print "Processing", src, "=>", dst
        if src == None:
            raise RuntimeError("No source file, dst is " + dst)
        if dst == None:
            dst = src
        dst = os.path.join(self.get_dst_prefix(), dst)
        src = os.path.join(self.get_src_prefix(), src)

        # expand globs
        if(self.wildcard_pattern.search(src)):
            for s,d in self.expand_globs(src, dst):
                self.process_file(s, d)
        else:
            # if we're specifying a single path (not a glob),
            # we should error out if it doesn't exist
            self.check_file_exists(src)
            # if it's a directory, recurse through it
            if(os.path.isdir(src)):
                self.process_directory(src, dst)
            else:
                self.process_file(src, dst)


    def do(self, *actions):
        self.actions = actions
        self.construct()
        # perform finish actions
        for action in self.actions:
            methodname = action + "_finish"
            method = getattr(self, methodname, None)
            if method is not None:
                method()
        return self.file_list
