"""\
@file llmanifest.py
@author Ryan Williams
@brief Library for specifying operations on a set of files.

$LicenseInfo:firstyear=2007&license=mit$

Copyright (c) 2007-2009, Linden Research, Inc.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
$/LicenseInfo$
"""

from collections import namedtuple, defaultdict
import subprocess
import errno
import filecmp
import fnmatch
import getopt
import glob
import itertools
import operator
import os
import re
import shlex
import shutil
import subprocess
import sys
import tarfile

class ManifestError(RuntimeError):
    """Use an exception more specific than generic Python RuntimeError"""
    def __init__(self, msg):
        self.msg = msg
        super(ManifestError, self).__init__(self.msg)

class MissingError(ManifestError):
    """You specified a file that doesn't exist"""
    def __init__(self, msg):
        self.msg = msg
        super(MissingError, self).__init__(self.msg)

def path_ancestors(path):
    drive, path = os.path.splitdrive(os.path.normpath(path))
    result = []
    while len(path) > 0 and path != os.path.sep:
        result.append(drive+path)
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
    if not match:
        match = re.match('([a-zA-Z]):\\\(.*)', path)
    if not match:
        return None         # not an absolute path
    drive_letter = match.group(1)
    rel = match.group(2)
    if current_platform == "cygwin":
        return "/cygdrive/" + drive_letter.lower() + '/' + rel.replace('\\', '/')
    else:
        return drive_letter.upper() + ':\\' + rel.replace('/', '\\')

def get_default_platform(dummy):
    return {'linux':'linux',
            'cygwin':'windows',
            'win32':'windows',
            'darwin':'darwin'
            }[sys.platform]

DEFAULT_SRCTREE = os.path.dirname(sys.argv[0])
CHANNEL_VENDOR_BASE = 'Second Life'
RELEASE_CHANNEL = CHANNEL_VENDOR_BASE + ' Release'

BASE_ARGUMENTS=[
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
    dict(name='artwork', description='Artwork directory.', default=DEFAULT_SRCTREE),
    dict(name='build', description='Build directory.', default=DEFAULT_SRCTREE),
    dict(name='buildtype', description='Build type (i.e. Debug, Release, RelWithDebInfo).', default=None),
    dict(name='bundleid',
         description="""The macOS Bundle identifier.""",
         default="com.secondlife.indra.viewer"),
    dict(name='channel',
         description="""The channel to use for updates, packaging, settings name, etc.""",
         default='CHANNEL UNSET'),
    dict(name='channel_suffix',
         description="""Addition to the channel for packaging and channel value,
         but not application name (used internally)""",
         default=None),
    dict(name='configuration',
         description="""The build configurations sub directory used.""",
         default="Release"),
    dict(name='dest', description='Destination directory.', default=DEFAULT_SRCTREE),
    dict(name='grid',
         description="""Which grid the client will try to connect to.""",
         default=None),
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
    dict(name='signature',
         description="""This specifies an identity to sign the viewer with, if any.
        If no value is supplied, the default signature will be used, if any. Currently
        only used on macOS.""",
         default=None),
    dict(name='source',
         description='Source directory.',
         default=DEFAULT_SRCTREE),
    dict(name='touch',
         description="""File to touch when action is finished. Touch file will
        contain the name of the final package in a form suitable
        for use by a .bat file.""",
         default=None),
    dict(name='versionfile',
         description="""The name of a file containing the full version number."""),
    ]

def usage(arguments, srctree=""):
    nd = {'name':sys.argv[0]}
    print("""Usage:
    %(name)s [options] [destdir]
    Options:
    """ % nd)
    for arg in arguments:
        default = arg['default']
        if hasattr(default, '__call__'):
            default = "(computed value) \"" + str(default(srctree)) + '"'
        elif default is not None:
            default = '"' + default + '"'
        print("\t--%s        Default: %s\n\t%s\n" % (
            arg['name'],
            default,
            arg['description'] % nd))

def main(extra=[]):
##  print ' '.join((("'%s'" % item) if ' ' in item else item)
##                 for item in itertools.chain([sys.executable], sys.argv))
    # Supplement our default command-line switches with any desired by
    # application-specific caller.
    arguments = list(itertools.chain(BASE_ARGUMENTS, extra))
    # Alphabetize them by option name in case we display usage.
    arguments.sort(key=operator.itemgetter('name'))
    option_names = [arg['name'] + '=' for arg in arguments]
    option_names.append('help')
    options, remainder = getopt.getopt(sys.argv[1:], "", option_names)

    # convert options to a hash
    args = {'source':  DEFAULT_SRCTREE,
            'artwork': DEFAULT_SRCTREE,
            'build':   DEFAULT_SRCTREE,
            'dest':    DEFAULT_SRCTREE }
    for opt in options:
        args[opt[0].replace("--", "")] = opt[1]

    for k in 'artwork build dest source'.split():
        args[k] = os.path.normpath(args[k])

    print("Source tree:", args['source'])
    print("Artwork tree:", args['artwork'])
    print("Build tree:", args['build'])
    print("Destination tree:", args['dest'])

    # early out for help
    if 'help' in args:
        # *TODO: it is a huge hack to pass around the srctree like this
        usage(arguments, srctree=args['source'])
        return

    # defaults
    for arg in arguments:
        if arg['name'] not in args:
            default = arg['default']
            if hasattr(default, '__call__'):
                default = default(args['source'])
            if default is not None:
                args[arg['name']] = default

    # fix up version
    if isinstance(args.get('versionfile'), str):
        try: # read in the version string
            vf = open(args['versionfile'], 'r')
            args['version'] = vf.read().strip().split('.')
        except:
            print("Unable to read versionfile '%s'" % args['versionfile'])
            raise

    # unspecified, default, and agni are default
    if args['grid'] in ['', 'default', 'agni']:
        args['grid'] = None

    if 'actions' in args:
        args['actions'] = args['actions'].split()

    # debugging
    for opt in args:
        print("Option:", opt, "=", args[opt])

    # pass in sourceid as an argument now instead of an environment variable
    args['sourceid'] = os.environ.get("sourceid", "")

    # Build base package.
    touch = args.get('touch')
    if touch:
        print('================ Creating base package')
    else:
        print('================ Starting base copy')
    wm = LLManifest.for_platform(args['platform'], args.get('arch'))(args)
    wm.do(*args['actions'])
    # Store package file for later if making touched file.
    base_package_file = ""
    if touch:
        print('================ Created base package ', wm.package_file)
        base_package_file = "" + wm.package_file
    else:
        print('================ Finished base copy')

    # handle multiple packages if set
    # ''.split() produces empty list
    additional_packages = os.environ.get("additional_packages", "").split()
    if additional_packages:
        # Determine destination prefix / suffix for additional packages.
        base_dest_parts = list(os.path.split(args['dest']))
        base_dest_parts.insert(-1, "{}")
        base_dest_template = os.path.join(*base_dest_parts)
        # Determine touched prefix / suffix for additional packages.
        if touch:
            base_touch_parts = list(os.path.split(touch))
            # Because of the special insert() logic below, we don't just want
            # [dirpath, basename]; we want [dirpath, directory, basename].
            # Further split the dirpath and replace it in the list.
            base_touch_parts[0:1] = os.path.split(base_touch_parts[0])
            if "arwin" in args['platform']:
                base_touch_parts.insert(-1, "{}")
            else:
                base_touch_parts.insert(-2, "{}")
            base_touch_template = os.path.join(*base_touch_parts)
        for package_id in additional_packages:
            args['channel_suffix'] = os.environ.get(package_id + "_viewer_channel_suffix")
            args['sourceid']       = os.environ.get(package_id + "_sourceid")
            args['dest'] = base_dest_template.format(package_id)
            if touch:
                print('================ Creating additional package for "', package_id, '" in ', args['dest'])
            else:
                print('================ Starting additional copy for "', package_id, '" in ', args['dest'])
            try:
                wm = LLManifest.for_platform(args['platform'], args.get('arch'))(args)
                wm.do(*args['actions'])
            except Exception as err:
                sys.exit(str(err))
            if touch:
                print('================ Created additional package ', wm.package_file, ' for ', package_id)
                with open(base_touch_template.format(package_id), 'w') as fp:
                    fp.write('set package_file=%s\n' % wm.package_file)
            else:
                print('================ Finished additional copy "', package_id, '" in ', args['dest'])
    # Write out the package file in this format, so that it can easily be called
    # and used in a .bat file - yeah, it sucks, but this is the simplest...
    if touch:
        with open(touch, 'w') as fp:
            fp.write('set package_file=%s\n' % base_package_file)
        print('touched', touch)
    return 0

class LLManifestRegistry(type):
    def __init__(cls, name, bases, dct):
        super(LLManifestRegistry, cls).__init__(name, bases, dct)
        match = re.match("(\w+)Manifest", name)
        if match:
           cls.manifests[match.group(1).lower()] = cls

MissingFile = namedtuple("MissingFile", ("pattern", "tried"))

class LLManifest(object, metaclass=LLManifestRegistry):
    manifests = {}
    def for_platform(self, platform, arch = None):
        if arch:
            platform = platform + '_' + arch + '_'
        return self.manifests[platform.lower()]
    for_platform = classmethod(for_platform)

    def __init__(self, args):
        super(LLManifest, self).__init__()
        self.args = args
        self.file_list = []
        self.excludes = []
        self.actions = []
        self.src_prefix = [args['source']]
        self.artwork_prefix = [args['artwork']]
        self.build_prefix = [args['build']]
        self.dst_prefix = [args['dest']]
        self.created_paths = []
        self.package_name = "Unknown"
        self.missing = []

    def default_channel(self):
        return self.args.get('channel', None) == RELEASE_CHANNEL

    def construct(self):
        """ Meant to be overriden by LLManifest implementors with code that
        constructs the complete destination hierarchy."""
        pass # override this method

    def exclude(self, glob):
        """ Excludes all files that match the glob from being included
        in the file list by path()."""
        self.excludes.append(glob)

    def prefix(self, src='', build='', dst='', src_dst=None):
        """
        Usage:

        with self.prefix(...args as described...):
            self.path(...)

        For the duration of the 'with' block, pushes a prefix onto the stack.
        Within that block, all relevant method calls (esp. to path()) will
        prefix paths with the entire prefix stack. Source and destination
        prefixes are independent; if omitted (or passed as the empty string),
        the prefix has no effect. Thus:

        with self.prefix(src='foo'):
            # no effect on dst

        with self.prefix(dst='bar'):
            # no effect on src

        If you want to set both at once, use src_dst:

        with self.prefix(src_dst='subdir'):
            # same as self.prefix(src='subdir', dst='subdir')
            # Passing src_dst makes any src or dst argument in the same
            # parameter list irrelevant.

        Also supports the older (pre-Python-2.5) syntax:

        if self.prefix(...args as described...):
            self.path(...)
            self.end_prefix(...)

        Before the arrival of the 'with' statement, one was required to code
        self.prefix() and self.end_prefix() in matching pairs to push and to
        pop the prefix stacks, respectively. The older prefix() method
        returned True specifically so that the caller could indent the
        relevant block of code with 'if', just for aesthetic purposes.
        """
        if src_dst is not None:
            src = src_dst
            dst = src_dst
        self.src_prefix.append(src)
        self.artwork_prefix.append(src)
        self.build_prefix.append(build)
        self.dst_prefix.append(dst)

##      self.display_stacks()

        # The above code is unchanged from the original implementation. What's
        # new is the return value. We're going to return an instance of
        # PrefixManager that binds this LLManifest instance and Does The Right
        # Thing on exit.
        return self.PrefixManager(self)

    def display_stacks(self):
        width = 1 + max(len(stack) for stack in self.PrefixManager.stacks)
        for stack in self.PrefixManager.stacks:
            print("{} {}".format((stack + ':').ljust(width),
                                 os.path.join(*getattr(self, stack))))

    class PrefixManager(object):
        # stack attributes we manage in this LLManifest (sub)class
        # instance
        stacks = ("src_prefix", "artwork_prefix", "build_prefix", "dst_prefix")

        def __init__(self, manifest):
            self.manifest = manifest
            # If the caller wrote:
            # with self.prefix(...):
            # as intended, then bind the state of each prefix stack as it was
            # just BEFORE the call to prefix(). Since prefix() appended an
            # entry to each prefix stack, capture len()-1.
            self.prevlen = { stack: len(getattr(self.manifest, stack)) - 1
                             for stack in self.stacks }

        def __bool__(self):
            # If the caller wrote:
            # if self.prefix(...):
            # then a value of this class had better evaluate as 'True'.
            return True

        def __enter__(self):
            # nobody uses 'with self.prefix(...) as variable:'
            return None

        def __exit__(self, type, value, traceback):
            # First, if the 'with' block raised an exception, just propagate.
            # Do NOT swallow it.
            if type is not None:
                return False

            # Okay, 'with' block completed successfully. Restore previous
            # state of each of the prefix stacks in self.stacks.
            # Note that we do NOT simply call pop() on them as end_prefix()
            # does. This is to cope with the possibility that the coder
            # changed 'if self.prefix(...):' to 'with self.prefix(...):' yet
            # forgot to remove the self.end_prefix(...) call at the bottom of
            # the block. In that case, calling pop() again would be Bad! But
            # if we restore the length of each stack to what it was before the
            # current prefix() block, it doesn't matter whether end_prefix()
            # was called or not.
            for stack, prevlen in list(self.prevlen.items()):
                # find the attribute in 'self.manifest' named by 'stack', and
                # truncate that list back to 'prevlen'
                del getattr(self.manifest, stack)[prevlen:]

##          self.manifest.display_stacks()

    def end_prefix(self, descr=None):
        """Pops a prefix off the stack.  If given an argument, checks
        the argument against the top of the stack.  If the argument
        matches neither the source or destination prefixes at the top
        of the stack, then misnesting must have occurred and an
        exception is raised."""
        # as an error-prevention mechanism, check the prefix and see if it matches the source or destination prefix.  If not, improper nesting may have occurred.
        src = self.src_prefix.pop()
        artwork = self.artwork_prefix.pop()
        build = self.build_prefix.pop()
        dst = self.dst_prefix.pop()
        if descr and not(src == descr or build == descr or dst == descr):
            raise ValueError("End prefix '" + descr + "' didn't match '" +src+ "' or '" +dst + "'")

    def get_src_prefix(self):
        """ Returns the current source prefix."""
        return os.path.join(*self.src_prefix)

    def get_artwork_prefix(self):
        """ Returns the current artwork prefix."""
        return os.path.join(*self.artwork_prefix)

    def get_build_prefix(self):
        """ Returns the current build prefix."""
        return os.path.join(*self.build_prefix)

    def get_dst_prefix(self):
        """ Returns the current destination prefix."""
        return os.path.join(*self.dst_prefix)

    def src_path_of(self, relpath):
        """Returns the full path to a file or directory specified
        relative to the source directory."""
        return os.path.join(self.get_src_prefix(), relpath)

    def build_path_of(self, relpath):
        """Returns the full path to a file or directory specified
        relative to the build directory."""
        return os.path.join(self.get_build_prefix(), relpath)

    def dst_path_of(self, relpath):
        """Returns the full path to a file or directory specified
        relative to the destination directory."""
        return os.path.join(self.get_dst_prefix(), relpath)

    def _relative_dst_path(self, dstpath):
        """
        Returns the path to a file or directory relative to the destination directory.
        This should only be used for generating diagnostic output in the path method.
        """
        dest_root=self.dst_prefix[0]
        if dstpath.startswith(dest_root+os.path.sep):
            return dstpath[len(dest_root)+1:]
        elif dstpath.startswith(dest_root):
            return dstpath[len(dest_root):]
        else:
            return dstpath

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

    def run_command(self, command, **kwds):
        """ 
        Runs an external command.  
        Raises ManifestError exception if the command returns a nonzero status.
        """
        print("Running command:", shlex.join(command))
        sys.stdout.flush()
        try:
            subprocess.check_call(command, **kwds)
        except subprocess.CalledProcessError as err:
            raise ManifestError( "Command %s returned non-zero status (%s)"
                                % (command, err.returncode) )

    def created_path(self, path):
        """ Declare that you've created a path in order to
          a) verify that you really have created it
          b) schedule it for cleanup"""
        if not os.path.exists(path):
            raise ManifestError("Should be something at path " + path)
        self.created_paths.append(path)

    def put_in_file(self, contents, dst, src=None):
        # write contents as dst
        dst_path = self.dst_path_of(dst)
        self.cmakedirs(os.path.dirname(dst_path))
        with open(dst_path, 'wb') as f:
            f.write(contents)

        # Why would we create a file in the destination tree if not to include
        # it in the installer? The default src=None (plus the fact that the
        # src param is last) is to preserve backwards compatibility.
        if src:
            self.file_list.append([src, dst_path])
        return dst_path

    def replace_in(self, src, dst=None, searchdict={}):
        if dst == None:
            dst = src
        # read src
        with open(self.src_path_of(src), "r") as f:
            contents = f.read()
        # apply dict replacements
        for old, new in searchdict.items():
            contents = contents.replace(old, new)
        self.put_in_file(contents.encode(), dst)
        self.created_paths.append(dst)

    def copy_action(self, src, dst):
        if src and (os.path.exists(src) or os.path.islink(src)):
            # ensure that destination path exists
            self.cmakedirs(os.path.dirname(dst))
            self.created_paths.append(dst)
            self.ccopymumble(src, dst)
        else:
            print("Doesn't exist:", src)

    def package_action(self, src, dst):
        pass

    def finish(self):
        """
        generic finish, always called before the ${action}_finish() methods
        """
        # Collecting MissingFile instances in self.missing, and checking that
        # here, is intended to minimize the number of (potentially lengthy)
        # build cycles a developer must run in order to fix missing-files
        # errors. The manifest processing is necessarily the last step in a
        # build, and if we only caught a single missing file error per run,
        # the developer would need to run a build for each additional missing-
        # file error until all were resolved. This way permits the developer
        # to resolve them all at once.
        if self.missing:
            print('*' * 72)
            print("Missing files:")
            # Instead of just dumping each missing file and all the places we
            # looked for it, group by common sets of places we looked. Use a
            # set to store the 'tried' directories, to avoid mismatches due to
            # reordering -- but since we intend to use the set of 'tried'
            # directories as a dict key, it must be a frozenset.
            organize = defaultdict(set)
            for missingfile in self.missing:
                organize[frozenset(missingfile.tried)].add(missingfile.pattern)
            # Now dump all the patterns sought in each group of 'tried'
            # directories.
            for tried, patterns in list(organize.items()):
                print("  Could not find in:")
                for dir in sorted(tried):
                    print("    %s" % dir)
                for pattern in sorted(patterns):
                    print("      %s" % pattern)
            print('*' * 72)
            raise MissingError('%s patterns could not be found' % len(self.missing))

    def copy_finish(self):
        pass

    def package_finish(self):
        pass

    def unpacked_finish(self):
        unpacked_file_name = "unpacked_%(plat)s_%(vers)s.tar" % {
            'plat':self.args['platform'],
            'vers':'_'.join(self.args['version'])}
        print("Creating unpacked file:", unpacked_file_name)
        # could add a gz here but that doubles the time it takes to do this step
        tf = tarfile.open(self.src_path_of(unpacked_file_name), 'w:')
        # add the entire installation package, at the very top level
        tf.add(self.get_dst_prefix(), "")
        tf.close()

    def cleanup_finish(self):
        """ Delete paths that were specified to have been created by this script"""
        for c in self.created_paths:
            # *TODO is this gonna be useful?
            print("Cleaning up " + c)

    def process_either(self, src, dst):
        # If it's a real directory, recurse through it --
        # but not a symlink! Handle those like files.
        if os.path.isdir(src) and not os.path.islink(src):
            return self.process_directory(src, dst)
        else:
            return self.process_file(src, dst)

    def process_file(self, src, dst):
        if self.includes(src, dst):
            for action in self.actions:
                methodname = action + "_action"
                method = getattr(self, methodname, None)
                if method is not None:
                    method(src, dst)
            self.file_list.append([src, dst])
            return 1
        else:
            sys.stdout.write(" (excluding %r, %r)" % (src, dst))
            sys.stdout.flush()
            return 0

    def process_directory(self, src, dst):
        if not self.includes(src, dst):
            sys.stdout.write(" (excluding %r, %r)" % (src, dst))
            sys.stdout.flush()
            return 0
        names = os.listdir(src)
        self.cmakedirs(dst)
        errors = []
        count = 0
        for name in names:
            srcname = os.path.join(src, name)
            dstname = os.path.join(dst, name)
            count += self.process_either(srcname, dstname)
        return count

    def includes(self, src, dst):
        if src:
            for excl in self.excludes:
                if fnmatch.fnmatch(src, excl):
                    return False
        return True

    def remove(self, *paths):
        for path in paths:
            if os.path.exists(path):
                print("Removing path", path)
                if os.path.isdir(path):
                    shutil.rmtree(path)
                else:
                    os.remove(path)

    def ccopymumble(self, src, dst):
        """Copy a single symlink, file or directory."""
        if os.path.islink(src):
            linkto = os.readlink(src)
            if os.path.islink(dst) or os.path.isfile(dst):
                os.remove(dst)  # because symlinking over an existing link fails
            elif os.path.isdir(dst):
                shutil.rmtree(dst)
            os.symlink(linkto, dst)
        elif os.path.isdir(src):
            self.ccopytree(src, dst)
        else:
            self.ccopyfile(src, dst)
            # XXX What about devices, sockets etc.?
            # YYY would we put such things into a viewer package?!

    def ccopyfile(self, src, dst):
        """ Copy a single file.  Uses filecmp to skip copying for existing files."""
        # Don't recopy file if it's up-to-date.
        # If we seem to be not not overwriting files that have been
        # updated, set the last arg to False, but it will take longer.
##      reldst = (dst[len(self.dst_prefix[0]):]
##                if dst.startswith(self.dst_prefix[0])
##                else dst).lstrip(r'\/')
        if os.path.exists(dst) and filecmp.cmp(src, dst, True):
##          print "{} (skipping, {} exists)".format(src, reldst)
            return
        # only copy if it's not excluded
        if self.includes(src, dst):
            try:
                os.unlink(dst)
            except OSError as err:
                if err.errno != errno.ENOENT:
                    raise

##          print "{} => {}".format(src, reldst)
            shutil.copy2(src, dst)

    def ccopytree(self, src, dst):
        """Direct copy of shutil.copytree with the additional
        feature that the destination directory can exist.  It
        is so dumb that Python doesn't come with this. Also it
        implements the excludes functionality."""
        if not self.includes(src, dst):
            return
        names = os.listdir(src)
        self.cmakedirs(dst)
        errors = []
        for name in names:
            srcname = os.path.join(src, name)
            dstname = os.path.join(dst, name)
            try:
                self.ccopymumble(srcname, dstname)
            except (IOError, os.error) as why:
                errors.append((srcname, dstname, why))
        if errors:
            raise ManifestError(errors)


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
            if os.path.exists(f):
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
        src_re = re.escape(src_glob)
        src_re = src_re.replace('\*', '([-a-zA-Z0-9._ ]*)')
        dst_temp = dst_glob
        i = 1
        while dst_temp.count("*") > 0:
            dst_temp = dst_temp.replace('*', '\g<' + str(i) + '>', 1)
            i = i+1
        return re.compile(src_re), dst_temp

    def check_file_exists(self, path):
        if not os.path.exists(path) and not os.path.islink(path):
            raise MissingError("Path %s doesn't exist" % (os.path.abspath(path),))


    wildcard_pattern = re.compile(r'\*')
    def expand_globs(self, src, dst):
        src_list = glob.glob(src)
        src_re, d_template = self.wildcard_regex(src.replace('\\', '/'),
                                                 dst.replace('\\', '/'))
        for s in src_list:
            d = src_re.sub(d_template, s.replace('\\', '/'))
            yield os.path.normpath(s), os.path.normpath(d)

    def path2basename(self, path, file):
        """
        It is a common idiom to write:
        self.path(os.path.join(somedir, somefile), somefile)

        So instead you can write:
        self.path2basename(somedir, somefile)

        Note that this is NOT the same as:
        self.path(os.path.join(somedir, somefile))

        which is the same as:
        temppath = os.path.join(somedir, somefile)
        self.path(temppath, temppath)
        """
        return self.path(os.path.join(path, file), file)

    def path(self, src, dst=None):
        sys.stdout.flush()
        if src == None:
            raise ManifestError("No source file, dst is " + dst)
        if dst == None:
            dst = src
        dst = os.path.join(self.get_dst_prefix(), dst)
        sys.stdout.write("Processing %s => %s ... " % (src, self._relative_dst_path(dst)))

        def try_path(src):
            # expand globs
            count = 0
            if self.wildcard_pattern.search(src):
                for s,d in self.expand_globs(src, dst):
                    assert(s != d)
                    count += self.process_file(s, d)
            else:
                # if we're specifying a single path (not a glob),
                # we should error out if it doesn't exist
                self.check_file_exists(src)
                count += self.process_either(src, dst)
            return count

        try_prefixes = [self.get_src_prefix(), self.get_artwork_prefix(), self.get_build_prefix()]
        for pfx in try_prefixes:
            try:
                count = try_path(os.path.join(pfx, src))
            except MissingError:
                # if we produce MissingError, just try the next prefix
                continue
            # If we actually found nonzero files, stop looking
            if count:
                break
        else:
            # no more prefixes left to try
            print(("\nunable to find '%s'; looked in:\n  %s" % (src, '\n  '.join(try_prefixes))))
            self.missing.append(MissingFile(pattern=src, tried=try_prefixes))
            # At this point 'count' might never have been successfully
            # assigned! Even if it was, though, we can be sure it is 0.
            return 0

        print("%d files" % count)

        # Let caller check whether we processed as many files as expected. In
        # particular, let caller notice 0.
        return count

    def path_optional(self, src, dst=None):
        sys.stdout.flush()
        if src == None:
            raise ManifestError("No source file, dst is " + dst)
        if dst == None:
            dst = src
        dst = os.path.join(self.get_dst_prefix(), dst)
        sys.stdout.write("Processing %s => %s ... " % (src, self._relative_dst_path(dst)))

        def try_path(src):
            # expand globs
            count = 0
            if self.wildcard_pattern.search(src):
                for s,d in self.expand_globs(src, dst):
                    assert(s != d)
                    count += self.process_file(s, d)
            else:
                # if we're specifying a single path (not a glob),
                # we should error out if it doesn't exist
                self.check_file_exists(src)
                count += self.process_either(src, dst)
            return count

        try_prefixes = [self.get_src_prefix(), self.get_artwork_prefix(), self.get_build_prefix()]
        for pfx in try_prefixes:
            try:
                count = try_path(os.path.join(pfx, src))
            except MissingError:
                # if we produce MissingError, just try the next prefix
                continue
            # If we actually found nonzero files, stop looking
            if count:
                break
        else:
            sys.stdout.write("Skipping %s\n" % (src))
            return 0

        print("%d files" % count)

        # Let caller check whether we processed as many files as expected. In
        # particular, let caller notice 0.
        return count

    def do(self, *actions):
        self.actions = actions
        self.construct()
        # perform finish actions
        # generic finish first
        self.finish()
        for action in self.actions:
            methodname = action + "_finish"
            method = getattr(self, methodname, None)
            if method is not None:
                method()
        return self.file_list
