#!/usr/bin/env python
#
# @file develop.py
# @authors Bryan O'Sullivan, Mark Palange, Aaron Brashears
# @brief Fire and forget script to appropriately configure cmake for SL.
#
# $LicenseInfo:firstyear=2007&license=viewergpl$
# 
# Copyright (c) 2007-2009, Linden Research, Inc.
# 
# Second Life Viewer Source Code
# The source code in this file ("Source Code") is provided by Linden Lab
# to you under the terms of the GNU General Public License, version 2.0
# ("GPL"), unless you have obtained a separate licensing agreement
# ("Other License"), formally executed by you and Linden Lab.  Terms of
# the GPL can be found in doc/GPL-license.txt in this distribution, or
# online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
# 
# There are special exceptions to the terms and conditions of the GPL as
# it is applied to this Source Code. View the full text of the exception
# in the file doc/FLOSS-exception.txt in this software distribution, or
# online at
# http://secondlifegrid.net/programs/open_source/licensing/flossexception
# 
# By copying, modifying or distributing this software, you acknowledge
# that you have read and understood your obligations described above,
# and agree to abide by those obligations.
# 
# ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
# WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
# COMPLETENESS OR PERFORMANCE.
# $/LicenseInfo$


import errno
import getopt
import os
import random
import re
import shutil
import socket
import sys
import commands
import subprocess

class CommandError(Exception):
    pass


def mkdir(path):
    try:
        os.mkdir(path)
        return path
    except OSError, err:
        if err.errno != errno.EEXIST or not os.path.isdir(path):
            raise

def getcwd():
    cwd = os.getcwd()
    if 'a' <= cwd[0] <= 'z' and cwd[1] == ':':
        # CMake wants DOS drive letters to be in uppercase.  The above
        # condition never asserts on platforms whose full path names
        # always begin with a slash, so we don't need to test whether
        # we are running on Windows.
        cwd = cwd[0].upper() + cwd[1:]
    return cwd

def quote(opts):
    return '"' + '" "'.join([ opt.replace('"', '') for opt in opts ]) + '"'

class PlatformSetup(object):
    generator = None
    build_types = {}
    for t in ('Debug', 'Release', 'RelWithDebInfo'):
        build_types[t.lower()] = t

    build_type = build_types['relwithdebinfo']
    standalone = 'OFF'
    unattended = 'OFF'
    universal = 'OFF'
    project_name = 'SecondLife'
    distcc = True
    cmake_opts = []
    word_size = 32

    def __init__(self):
        self.script_dir = os.path.realpath(
            os.path.dirname(__import__(__name__).__file__))

    def os(self):
        '''Return the name of the OS.'''

        raise NotImplemented('os')

    def arch(self):
        '''Return the CPU architecture.'''

        return None

    def platform(self):
        '''Return a stringified two-tuple of the OS name and CPU
        architecture.'''

        ret = self.os()
        if self.arch():
            ret += '-' + self.arch()
        return ret 

    def build_dirs(self):
        '''Return the top-level directories in which builds occur.

        This can return more than one directory, e.g. if doing a
        32-bit viewer and server build on Linux.'''

        return ['build-' + self.platform()]

    def cmake_commandline(self, src_dir, build_dir, opts, simple):
        '''Return the command line to run cmake with.'''

        args = dict(
            dir=src_dir,
            generator=self.generator,
            opts=quote(opts),
            standalone=self.standalone,
            unattended=self.unattended,
            word_size=self.word_size,
            type=self.build_type.upper(),
            )
        #if simple:
        #    return 'cmake %(opts)s %(dir)r' % args
        return ('cmake -DCMAKE_BUILD_TYPE:STRING=%(type)s '
                '-DSTANDALONE:BOOL=%(standalone)s '
                '-DUNATTENDED:BOOL=%(unattended)s '
                '-DWORD_SIZE:STRING=%(word_size)s '
                '-G %(generator)r %(opts)s %(dir)r' % args)

    def run_cmake(self, args=[]):
        '''Run cmake.'''

        # do a sanity check to make sure we have a generator
        if not hasattr(self, 'generator'):
            raise "No generator available for '%s'" % (self.__name__,)
        cwd = getcwd()
        created = []
        try:
            for d in self.build_dirs():
                simple = True
                if mkdir(d):
                    created.append(d)
                    simple = False
                try:
                    os.chdir(d)
                    cmd = self.cmake_commandline(cwd, d, args, simple)
                    print 'Running %r in %r' % (cmd, d)
                    self.run(cmd, 'cmake')
                finally:
                    os.chdir(cwd)
        except:
            # If we created a directory in which to run cmake and
            # something went wrong, the directory probably just
            # contains garbage, so delete it.
            os.chdir(cwd)
            for d in created:
                print 'Cleaning %r' % d
                shutil.rmtree(d)
            raise

    def parse_build_opts(self, arguments):
        opts, targets = getopt.getopt(arguments, 'o:', ['option='])
        build_opts = []
        for o, a in opts:
            if o in ('-o', '--option'):
                build_opts.append(a)
        return build_opts, targets

    def run_build(self, opts, targets):
        '''Build the default targets for this platform.'''

        raise NotImplemented('run_build')

    def cleanup(self):
        '''Delete all build directories.'''

        cleaned = 0
        for d in self.build_dirs():
            if os.path.isdir(d):
                print 'Cleaning %r' % d
                shutil.rmtree(d)
                cleaned += 1
        if not cleaned:
            print 'Nothing to clean up!'

    def is_internal_tree(self):
        '''Indicate whether we are building in an internal source tree.'''

        return os.path.isdir(os.path.join(self.script_dir, 'newsim'))

    def find_in_path(self, name, defval=None, basename=False):
        for ext in self.exe_suffixes:
            name_ext = name + ext
            if os.sep in name_ext:
                path = os.path.abspath(name_ext)
                if os.access(path, os.X_OK):
                    return [basename and os.path.basename(path) or path]
            for p in os.getenv('PATH', self.search_path).split(os.pathsep):
                path = os.path.join(p, name_ext)
                if os.access(path, os.X_OK):
                    return [basename and os.path.basename(path) or path]
        if defval:
            return [defval]
        return []


class UnixSetup(PlatformSetup):
    '''Generic Unixy build instructions.'''

    search_path = '/usr/bin:/usr/local/bin'
    exe_suffixes = ('',)

    def __init__(self):
        super(UnixSetup, self).__init__()
        self.generator = 'Unix Makefiles'

    def os(self):
        return 'unix'

    def arch(self):
        cpu = os.uname()[-1]
        if cpu.endswith('386'):
            cpu = 'i386'
        elif cpu.endswith('86'):
            cpu = 'i686'
        elif cpu in ('athlon',):
            cpu = 'i686'
        elif cpu == 'Power Macintosh':
            cpu = 'ppc'
        elif cpu == 'x86_64' and self.word_size == 32:
            cpu = 'i686'
        return cpu

    def run(self, command, name=None):
        '''Run a program.  If the program fails, raise an exception.'''
        sys.stdout.flush()
        ret = os.system(command)
        if ret:
            if name is None:
                name = command.split(None, 1)[0]
            if os.WIFEXITED(ret):
                st = os.WEXITSTATUS(ret)
                if st == 127:
                    event = 'was not found'
                else:
                    event = 'exited with status %d' % st
            elif os.WIFSIGNALED(ret):
                event = 'was killed by signal %d' % os.WTERMSIG(ret)
            else:
                event = 'died unexpectedly (!?) with 16-bit status %d' % ret
            raise CommandError('the command %r %s' %
                               (name, event))


class LinuxSetup(UnixSetup):
    def __init__(self):
        super(LinuxSetup, self).__init__()
        try:
            self.debian_sarge = open('/etc/debian_version').read().strip() == '3.1'
        except:
            self.debian_sarge = False

    def os(self):
        return 'linux'

    def build_dirs(self):
        # Only build the server code if we have it.
        platform_build = '%s-%s' % (self.platform(), self.build_type.lower())

        if self.arch() == 'i686' and self.is_internal_tree():
            return ['viewer-' + platform_build, 'server-' + platform_build]
        elif self.arch() == 'x86_64' and self.is_internal_tree():
            # the viewer does not build in 64bit -- kdu5 issues
            # we can either use openjpeg, or overhaul our viewer to handle kdu5 or higher
            # doug knows about kdu issues
            return ['server-' + platform_build]
        else:
            return ['viewer-' + platform_build]

    def cmake_commandline(self, src_dir, build_dir, opts, simple):
        args = dict(
            dir=src_dir,
            generator=self.generator,
            opts=quote(opts),
            standalone=self.standalone,
            unattended=self.unattended,
            type=self.build_type.upper(),
            project_name=self.project_name,
            word_size=self.word_size,
            )
        if not self.is_internal_tree():
            args.update({'cxx':'g++', 'server':'OFF', 'viewer':'ON'})
        else:
            if self.distcc:
                distcc = self.find_in_path('distcc')
                baseonly = True
            else:
                distcc = []
                baseonly = False
            if 'server' in build_dir:
                gcc = distcc + self.find_in_path(
                    self.debian_sarge and 'g++-3.3' or 'g++-4.1',
                    'g++', baseonly)
                args.update({'cxx': ' '.join(gcc), 'server': 'ON',
                             'viewer': 'OFF'})
            else:
                gcc41 = distcc + self.find_in_path('g++-4.1', 'g++', baseonly)
                args.update({'cxx': ' '.join(gcc41),
                             'server': 'OFF',
                             'viewer': 'ON'})
        cmd = (('cmake -DCMAKE_BUILD_TYPE:STRING=%(type)s '
                '-G %(generator)r -DSERVER:BOOL=%(server)s '
                '-DVIEWER:BOOL=%(viewer)s -DSTANDALONE:BOOL=%(standalone)s '
                '-DUNATTENDED:BOOL=%(unattended)s '
                '-DWORD_SIZE:STRING=%(word_size)s '
                '-DROOT_PROJECT_NAME:STRING=%(project_name)s '
                '%(opts)s %(dir)r')
               % args)
        if 'CXX' not in os.environ:
            args.update({'cmd':cmd})
            cmd = ('CXX=%(cxx)r %(cmd)s' % args)
        return cmd

    def run_build(self, opts, targets):
        job_count = None

        for i in range(len(opts)):
            if opts[i].startswith('-j'):
                try:
                    job_count = int(opts[i][2:])
                except ValueError:
                    try:
                        job_count = int(opts[i+1])
                    except ValueError:
                        job_count = True

        def get_cpu_count():
            count = 0
            for line in open('/proc/cpuinfo'):
                if re.match(r'processor\s*:', line):
                    count += 1
            return count

        def localhost():
            count = get_cpu_count()
            return 'localhost/' + str(count), count

        def get_distcc_hosts():
            try:
                hosts = []
                name = os.getenv('DISTCC_DIR', '/etc/distcc') + '/hosts'
                for l in open(name):
                    l = l[l.find('#')+1:].strip()
                    if l: hosts.append(l)
                return hosts
            except IOError:
                return (os.getenv('DISTCC_HOSTS', '').split() or
                        [localhost()[0]])

        def count_distcc_hosts():
            cpus = 0
            hosts = 0
            for host in get_distcc_hosts():
                m = re.match(r'.*/(\d+)', host)
                hosts += 1
                cpus += m and int(m.group(1)) or 1
            return hosts, cpus

        def mk_distcc_hosts(basename, range, num_cpus):
            '''Generate a list of LL-internal machines to build on.'''
            loc_entry, cpus = localhost()
            hosts = [loc_entry]
            dead = []
            stations = [s for s in xrange(range) if s not in dead]
            random.shuffle(stations)
            hosts += ['%s%d.lindenlab.com/%d,lzo' % (basename, s, num_cpus) for s in stations]
            cpus += 2 * len(stations)
            return ' '.join(hosts), cpus

        if job_count is None:
            hosts, job_count = count_distcc_hosts()
            hostname = socket.gethostname()
            if hosts == 1:
                if hostname.startswith('station'):
                    hosts, job_count = mk_distcc_hosts('station', 36, 2)
                    os.environ['DISTCC_HOSTS'] = hosts
                if hostname.startswith('eniac'):
                    hosts, job_count = mk_distcc_hosts('eniac', 71, 2)
                    os.environ['DISTCC_HOSTS'] = hosts
            if hostname.startswith('build'):
                max_jobs = 6
            else:
                max_jobs = 12
            if job_count > max_jobs:
                job_count = max_jobs;
            opts.extend(['-j', str(job_count)])

        if targets:
            targets = ' '.join(targets)
        else:
            targets = 'all'

        for d in self.build_dirs():
            cmd = 'make -C %r %s %s' % (d, ' '.join(opts), targets)
            print 'Running %r' % cmd
            self.run(cmd)

        
class DarwinSetup(UnixSetup):
    def __init__(self):
        super(DarwinSetup, self).__init__()
        self.generator = 'Xcode'

    def os(self):
        return 'darwin'

    def arch(self):
        if self.universal == 'ON':
            return 'universal'
        else:
            return UnixSetup.arch(self)

    def cmake_commandline(self, src_dir, build_dir, opts, simple):
        args = dict(
            dir=src_dir,
            generator=self.generator,
            opts=quote(opts),
            standalone=self.standalone,
            word_size=self.word_size,
            unattended=self.unattended,
            project_name=self.project_name,
            universal=self.universal,
            type=self.build_type.upper(),
            )
        if self.universal == 'ON':
            args['universal'] = '-DCMAKE_OSX_ARCHITECTURES:STRING=\'i386;ppc\''
        #if simple:
        #    return 'cmake %(opts)s %(dir)r' % args
        return ('cmake -G %(generator)r '
                '-DCMAKE_BUILD_TYPE:STRING=%(type)s '
                '-DSTANDALONE:BOOL=%(standalone)s '
                '-DUNATTENDED:BOOL=%(unattended)s '
                '-DWORD_SIZE:STRING=%(word_size)s '
                '-DROOT_PROJECT_NAME:STRING=%(project_name)s '
                '%(universal)s '
                '%(opts)s %(dir)r' % args)

    def run_build(self, opts, targets):
        cwd = getcwd()
        if targets:
            targets = ' '.join(['-target ' + repr(t) for t in targets])
        else:
            targets = ''
        cmd = ('xcodebuild -configuration %s %s %s | grep -v "^[[:space:]]*setenv" ; exit ${PIPESTATUS[0]}' %
               (self.build_type, ' '.join(opts), targets))
        for d in self.build_dirs():
            try:
                os.chdir(d)
                print 'Running %r in %r' % (cmd, d)
                self.run(cmd)
            finally:
                os.chdir(cwd)


class WindowsSetup(PlatformSetup):
    gens = {
        'vc71' : {
            'gen' : r'Visual Studio 7 .NET 2003',
            'ver' : r'7.1'
            },
        'vc80' : {
            'gen' : r'Visual Studio 8 2005',
            'ver' : r'8.0'
            },
        'vc90' : {
            'gen' : r'Visual Studio 9 2008',
            'ver' : r'9.0'
            }
        }
    gens['vs2003'] = gens['vc71']
    gens['vs2005'] = gens['vc80']
    gens['vs2008'] = gens['vc90']

    search_path = r'C:\windows'
    exe_suffixes = ('.exe', '.bat', '.com')

    def __init__(self):
        super(WindowsSetup, self).__init__()
        self._generator = None
        self.incredibuild = False

    def _get_generator(self):
        if self._generator is None:
            for version in 'vc80 vc90 vc71'.split():
                if self.find_visual_studio(version):
                    self._generator = version
                    print 'Building with ', self.gens[version]['gen']
                    break
            else:
                print >> sys.stderr, 'Cannot find a Visual Studio installation!'
                sys.exit(1)
        return self._generator

    def _set_generator(self, gen):
        self._generator = gen

    generator = property(_get_generator, _set_generator)

    def os(self):
        return 'win32'

    def build_dirs(self):
        return ['build-' + self.generator]

    def cmake_commandline(self, src_dir, build_dir, opts, simple):
        args = dict(
            dir=src_dir,
            generator=self.gens[self.generator.lower()]['gen'],
            opts=quote(opts),
            standalone=self.standalone,
            unattended=self.unattended,
            project_name=self.project_name,
            word_size=self.word_size,
            )
        #if simple:
        #    return 'cmake %(opts)s "%(dir)s"' % args
        return ('cmake -G "%(generator)s" '
                '-DSTANDALONE:BOOL=%(standalone)s '
                '-DUNATTENDED:BOOL=%(unattended)s '
                '-DWORD_SIZE:STRING=%(word_size)s '
                '-DROOT_PROJECT_NAME:STRING=%(project_name)s '
                '%(opts)s "%(dir)s"' % args)

    def get_HKLM_registry_value(self, key_str, value_str):
        import _winreg
        reg = _winreg.ConnectRegistry(None, _winreg.HKEY_LOCAL_MACHINE)
        key = _winreg.OpenKey(reg, key_str)
        value = _winreg.QueryValueEx(key, value_str)[0]
        print 'Found: %s' % value
        return value
        
    def find_visual_studio(self, gen=None):
        if gen is None:
            gen = self._generator
        gen = gen.lower()
        value_str = (r'EnvironmentDirectory')
        key_str = (r'SOFTWARE\Microsoft\VisualStudio\%s\Setup\VS' %
                   self.gens[gen]['ver'])
        print ('Reading VS environment from HKEY_LOCAL_MACHINE\%s\%s' %
               (key_str, value_str))
        try:
            return self.get_HKLM_registry_value(key_str, value_str)           
        except WindowsError, err:
            key_str = (r'SOFTWARE\Wow6432Node\Microsoft\VisualStudio\%s\Setup\VS' %
                       self.gens[gen]['ver'])

        try:
            return self.get_HKLM_registry_value(key_str, value_str)
        except:
            print >> sys.stderr, "Didn't find ", self.gens[gen]['gen']
            
        return ''

    def get_build_cmd(self):
        if self.incredibuild:
            config = self.build_type
            if self.gens[self.generator]['ver'] in [ r'8.0', r'9.0' ]:
                config = '\"%s|Win32\"' % config

            executable = 'buildconsole'
            cmd = "%(bin)s %(prj)s.sln /build /cfg=%(cfg)s" % {'prj': self.project_name, 'cfg': config, 'bin': executable}
            return (executable, cmd)

        # devenv.com is CLI friendly, devenv.exe... not so much.
        executable = '%sdevenv.com' % (self.find_visual_studio(),)
        cmd = ('"%s" %s.sln /build %s' % 
                (executable, self.project_name, self.build_type))
        return (executable, cmd)

    def run(self, command, name=None, retry_on=None, retries=1):
        '''Run a program.  If the program fails, raise an exception.'''
        assert name is not None, 'On windows an executable path must be given in name.'
        if os.path.isfile(name):
            path = name
        else:
            path = self.find_in_path(name)[0]
        while retries:
            retries = retries - 1
            print "develop.py tries to run:", command
            ret = subprocess.call(command, executable=path)
            print "got ret", ret, "from", command
            if ret:
                error = 'exited with status %d' % ret
                if retry_on is not None and retry_on == ret:
                    print "Retrying... the command %r %s" % (name, error)
                else:
                    raise CommandError('the command %r %s' % (name, error))

    def run_cmake(self, args=[]):
        '''Override to add the vstool.exe call after running cmake.'''
        PlatformSetup.run_cmake(self, args)
        if self.unattended == 'OFF':
            self.run_vstool()

    def run_vstool(self):
        for build_dir in self.build_dirs():
            stamp = os.path.join(build_dir, 'vstool.txt')
            try:
                prev_build = open(stamp).read().strip()
            except IOError:
                prev_build = ''
            if prev_build == self.build_type:
                # Only run vstool if the build type has changed.
                continue
            executable = os.path.join('tools','vstool','VSTool.exe')
            vstool_cmd = (executable +
                          ' --solution ' +
                          os.path.join(build_dir,'SecondLife.sln') +
                          ' --config ' + self.build_type +
                          ' --startup secondlife-bin')
            print 'Running %r in %r' % (vstool_cmd, getcwd())
            self.run(vstool_cmd, name=executable)        
            print >> open(stamp, 'w'), self.build_type
        
    def run_build(self, opts, targets):
        cwd = getcwd()
        executable, build_cmd = self.get_build_cmd()

        for d in self.build_dirs():
            try:
                os.chdir(d)
                if targets:
                    for t in targets:
                        cmd = '%s /project %s %s' % (build_cmd, t, ' '.join(opts))
                        print 'Running %r in %r' % (cmd, d)
                        self.run(cmd, name=executable, retry_on=4, retries=3)
                else:
                    cmd = '%s %s' % (build_cmd, ' '.join(opts))
                    print 'Running %r in %r' % (cmd, d)
                    self.run(cmd, name=executable, retry_on=4, retries=3)
            finally:
                os.chdir(cwd)
                
class CygwinSetup(WindowsSetup):
    def __init__(self):
        super(CygwinSetup, self).__init__()
        self.generator = 'vc80'

    def cmake_commandline(self, src_dir, build_dir, opts, simple):
        dos_dir = commands.getoutput("cygpath -w %s" % src_dir)
        args = dict(
            dir=dos_dir,
            generator=self.gens[self.generator.lower()]['gen'],
            opts=quote(opts),
            standalone=self.standalone,
            unattended=self.unattended,
            project_name=self.project_name,
            word_size=self.word_size,
            )
        #if simple:
        #    return 'cmake %(opts)s "%(dir)s"' % args
        return ('cmake -G "%(generator)s" '
                '-DUNATTENDED:BOOl=%(unattended)s '
                '-DSTANDALONE:BOOL=%(standalone)s '
                '-DWORD_SIZE:STRING=%(word_size)s '
                '-DROOT_PROJECT_NAME:STRING=%(project_name)s '
                '%(opts)s "%(dir)s"' % args)

setup_platform = {
    'darwin': DarwinSetup,
    'linux2': LinuxSetup,
    'win32' : WindowsSetup,
    'cygwin' : CygwinSetup
    }


usage_msg = '''
Usage:   develop.py [options] [command [command-options]]

Options:
  -h | --help           print this help message
       --standalone     build standalone, without Linden prebuild libraries
       --unattended     build unattended, do not invoke any tools requiring
                        a human response
       --universal      build a universal binary on Mac OS X (unsupported)
  -t | --type=NAME      build type ("Debug", "Release", or "RelWithDebInfo")
  -m32 | -m64           build architecture (32-bit or 64-bit)
  -N | --no-distcc      disable use of distcc
  -G | --generator=NAME generator name
                        Windows: VC71 or VS2003 (default), VC80 (VS2005) or 
                          VC90 (VS2008)
                        Mac OS X: Xcode (default), Unix Makefiles
                        Linux: Unix Makefiles (default), KDevelop3
  -p | --project=NAME   set the root project name. (Doesn't effect makefiles)
                        
Commands:
  build      configure and build default target
  clean      delete all build directories, does not affect sources
  configure  configure project by running cmake (default command if none given)

Command-options for "configure":
  We use cmake variables to change the build configuration.
  -DSERVER:BOOL=OFF        Don't configure simulator/dataserver/etc
  -DVIEWER:BOOL=OFF        Don't configure the viewer
  -DPACKAGE:BOOL=ON        Create "package" target to make installers
  -DLOCALIZESETUP:BOOL=ON  Create one win_setup target per supported language

Examples:
  Set up a viewer-only project for your system:
    develop.py configure -DSERVER:BOOL=OFF
  
  Set up a Visual Studio 2005 project with "package" target:
    develop.py -G vc80 configure -DPACKAGE:BOOL=ON
'''

def main(arguments):
    if os.getenv('DISTCC_DIR') is None:
        distcc_dir = os.path.join(getcwd(), '.distcc')
        if not os.path.exists(distcc_dir):
            os.mkdir(distcc_dir)
        print "setting DISTCC_DIR to %s" % distcc_dir
        os.environ['DISTCC_DIR'] = distcc_dir
    else:
        print "DISTCC_DIR is set to %s" % os.getenv('DISTCC_DIR')
 
    setup = setup_platform[sys.platform]()
    try:
        opts, args = getopt.getopt(
            arguments,
            '?hNt:p:G:m:',
            ['help', 'standalone', 'no-distcc', 'unattended', 'universal', 'type=', 'incredibuild', 'generator=', 'project='])
    except getopt.GetoptError, err:
        print >> sys.stderr, 'Error:', err
        print >> sys.stderr, """
Note: You must pass -D options to cmake after the "configure" command
For example: develop.py configure -DSERVER:BOOL=OFF"""
        print >> sys.stderr, usage_msg.strip()
        sys.exit(1)

    for o, a in opts:
        if o in ('-?', '-h', '--help'):
            print usage_msg.strip()
            sys.exit(0)
        elif o in ('--standalone',):
            setup.standalone = 'ON'
        elif o in ('--unattended',):
            setup.unattended = 'ON'
        elif o in ('--universal',):
            setup.universal = 'ON'
        elif o in ('-m',):
            if a in ('32', '64'):
                setup.word_size = int(a)
            else:
                print >> sys.stderr, 'Error: unknown word size', repr(a)
                print >> sys.stderr, 'Supported word sizes: 32, 64'
                sys.exit(1)
        elif o in ('-t', '--type'):
            try:
                setup.build_type = setup.build_types[a.lower()]
            except KeyError:
                print >> sys.stderr, 'Error: unknown build type', repr(a)
                print >> sys.stderr, 'Supported build types:'
                types = setup.build_types.values()
                types.sort()
                for t in types:
                    print ' ', t
                sys.exit(1)
        elif o in ('-G', '--generator'):
            setup.generator = a
        elif o in ('-N', '--no-distcc'):
            setup.distcc = False
        elif o in ('-p', '--project'):
            setup.project_name = a
        elif o in ('--incredibuild'):
            setup.incredibuild = True
        else:
            print >> sys.stderr, 'INTERNAL ERROR: unhandled option', repr(o)
            sys.exit(1)
    if not args:
        setup.run_cmake()
        return
    try:
        cmd = args.pop(0)
        if cmd in ('cmake', 'configure'):
            setup.run_cmake(args)
        elif cmd == 'build':
            for d in setup.build_dirs():
                if not os.path.exists(d):
                    raise CommandError('run "develop.py cmake" first')
            setup.run_cmake()
            opts, targets = setup.parse_build_opts(args)
            setup.run_build(opts, targets)
        elif cmd == 'clean':
            if args:
                raise CommandError('clean takes no arguments')
            setup.cleanup()
        else:
            print >> sys.stderr, 'Error: unknown subcommand', repr(cmd)
            print >> sys.stderr, "(run 'develop.py --help' for help)"
            sys.exit(1)
    except getopt.GetoptError, err:
        print >> sys.stderr, 'Error with %r subcommand: %s' % (cmd, err)
        sys.exit(1)


if __name__ == '__main__':
    try:
        main(sys.argv[1:])
    except CommandError, err:
        print >> sys.stderr, 'Error:', err
        sys.exit(1)
