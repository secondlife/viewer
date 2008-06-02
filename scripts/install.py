#!/usr/bin/env python
"""\
@file install.py
@author Phoenix
@date 2008-01-27
@brief Install files into an indra checkout.

Install files as specified by:
https://wiki.lindenlab.com/wiki/User:Phoenix/Library_Installation


$LicenseInfo:firstyear=2007&license=mit$

Copyright (c) 2007, Linden Research, Inc.

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

import copy
import errno
import md5
import optparse
import os
import pprint
import sys
import tarfile
import urllib
import urlparse

from sets import Set as set, ImmutableSet as frozenset

# Locate -our- python library relative to our install location.
from os.path import realpath, dirname, join

# Walk back to checkout base directory
base_dir = dirname(dirname(realpath(__file__)))
# Walk in to libraries directory
lib_dir = join(join(join(base_dir, 'indra'), 'lib'), 'python')

if lib_dir not in sys.path:
    sys.path.insert(0, lib_dir)

from indra.base import llsd
from indra.util import helpformatter

class InstallFile(object):
    "This is just a handy way to throw around details on a file in memory."
    def __init__(self, pkgname, url, md5sum, cache_dir, platform_path):
        self.pkgname = pkgname
        self.url = url
        self.md5sum = md5sum
        filename = urlparse.urlparse(url)[2].split('/')[-1]
        self.filename = os.path.join(cache_dir, filename)
        self.platform_path = platform_path

    def __str__(self):
        return "ifile{%s:%s}" % (self.pkgname, self.url)

    def _is_md5_match(self):
        hasher = md5.new(file(self.filename).read())
        if hasher.hexdigest() == self.md5sum:
            return  True
        return False

    def is_match(self, platform):
        """@brief Test to see if this ifile is part of platform
@param platform The target platform. Eg, win32 or linux/i686/gcc/3.3
@return Returns True if the ifile is in the platform.
        """
        if self.platform_path == 'common':
            return True
        req_platform_path = platform.split('/')
        #print "platform:",req_platform_path
        #print "path:",self.platform_path
        # to match, every path part much match
        match_count = min(len(req_platform_path), len(self.platform_path))
        for ii in range(0, match_count):
            if req_platform_path[ii] != self.platform_path[ii]:
                return False
        #print "match!"
        return True
                               

    def fetch_local(self):
        print "Looking for:",self.filename
        if not os.path.exists(self.filename):
            print "not there -- fetching"
        elif self.md5sum and not self._is_md5_match():
            print "Found, but md5 does not match."
            os.remove(self.filename)
        else:
            print "Found matching package"
            return
        print "Downloading",self.url,"to local file",self.filename
        urllib.urlretrieve(self.url, self.filename)
        if self.md5sum and not self._is_md5_match():
            raise RuntimeError("Error matching md5 for %s" % self.url)

class LicenseDefinition(object):
    def __init__(self, definition):
        #probably looks like:
        # { text : ...,
        #   url : ...
        #   blessed : ...
        # }
        self._definition = definition
    

class BinaryDefinition(object):
    def __init__(self, definition):
        #probably looks like:
        # { packages : {platform...},
        #   copyright : ...
        #   license : ...
        #   description: ...
        # }
        self._definition = definition

    def _ifiles_from(self, tree, pkgname, cache_dir):
        return self._ifiles_from_path(tree, pkgname, cache_dir, [])

    def _ifiles_from_path(self, tree, pkgname, cache_dir, path):
        ifiles = []
        if tree.has_key('url'):
            ifiles.append(InstallFile(
                pkgname,
                tree['url'],
                tree.get('md5sum', None),
                cache_dir,
                path))
        else:
            for key in tree:
                platform_path = copy.copy(path)
                platform_path.append(key)
                ifiles.extend(
                    self._ifiles_from_path(
                        tree[key],
                        pkgname,
                        cache_dir,
                        platform_path))
        return ifiles

    def ifiles(self, pkgname, platform, cache_dir):
        """@brief return a list of appropriate InstallFile instances to install
@param pkgname The name of the package to be installed, eg 'tut'
@param platform The target platform. Eg, win32 or linux/i686/gcc/3.3
@param cache_dir The directory to cache downloads.
@return Returns a list of InstallFiles which are part of this install
        """
        if 'packages' not in self._definition:
            return []
        all_ifiles = self._ifiles_from(
            self._definition['packages'],
            pkgname,
            cache_dir)
        if platform == 'all':
            return all_ifiles
        return [ifile for ifile in all_ifiles if ifile.is_match(platform)]

class InstalledPackage(object):
    def __init__(self, definition):
        # looks like:
        # { url1 : [file1,file2,...],
        #   url2 : [file1,file2,...],...
        # }
        self._installed = {}
        for url in definition:
            self._installed[url] = definition[url]

    def urls(self):
        return self._installed.keys()

    def files_in(self, url):
        return self._installed[url]

    def remove(self, url):
        self._installed.pop(url)

    def add_files(self, url, files):
        self._installed[url] = files

class Installer(object):
    def __init__(self, install_filename, installed_filename, dryrun):
        self._install_filename = install_filename
        self._install_changed = False
        self._installed_filename = installed_filename
        self._installed_changed = False
        self._dryrun = dryrun
        self._binaries = {}
        self._licenses = {}
        self._installed = {}
        self.load()

    def load(self):
        if os.path.exists(self._install_filename):
            install = llsd.parse(file(self._install_filename).read())
            try:
                for name in install['binaries']:
                    self._binaries[name] = BinaryDefinition(
                        install['binaries'][name])
            except KeyError:
                pass
            try:
                for name in install['licenses']:
                    self._licenses[name] = LicenseDefinition(install['licenses'][name])
            except KeyError:
                pass
        if os.path.exists(self._installed_filename):
            installed = llsd.parse(file(self._installed_filename).read())
            try:
                bins = installed['binaries']
                for name in bins:
                    self._installed[name] = InstalledPackage(bins[name])
            except KeyError:
                pass

    def _write(self, filename, state):
        print "Writing state to",filename
        if not self._dryrun:
            file(filename, 'w').write(llsd.format_xml(state))

    def save(self):
        if self._install_changed:
            state = {}
            state['licenses'] = {}
            for name in self._licenses:
                state['licenses'][name] = self._licenses[name]._definition
            #print "self._binaries:",self._binaries
            state['binaries'] = {}
            for name in self._binaries:
                state['binaries'][name] = self._binaries[name]._definition
            self._write(self._install_filename, state)
        if self._installed_changed:
            state = {}
            state['binaries'] = {}
            bin = state['binaries']
            for name in self._installed:
                #print "installed:",name,self._installed[name]._installed
                bin[name] = self._installed[name]._installed
            self._write(self._installed_filename, state)

    def is_license_info_valid(self):
        valid = True
        for bin in self._binaries:
            binary = self._binaries[bin]._definition
            if not binary.has_key('license'):
                valid = False
                print >>sys.stderr, "No license info for binary", bin + '.'
                continue
            if binary['license'] not in self._licenses:
                valid = False
                lic = binary['license']
                print >>sys.stderr, "Missing license info for '" + lic + "'",
                print >>sys.stderr, 'in binary', bin + '.'
        return valid

    def detail_binary(self, name):
        "Return a binary definition detail"
        try:
            detail = self._binaries[name]._definition
            return detail
        except KeyError:
            return None

    def _update_field(self, binary, field):
        """Given a block and a field name, add or update it.
        @param binary[in,out] a dict containing all the details about a binary.
        @param field the name of the field to update.
        """
        if binary.has_key(field):
            print "Update value for '" + field + "'"
            print "(Leave blank to keep current value)"
            print "Current Value:  '" + binary[field] + "'"
        else:
            print "Specify value for '" + field + "'"
        value = raw_input("Enter New Value: ")
        if binary.has_key(field) and not value:
            pass
        elif value:
            binary[field] = value

    def _add_package(self, binary):
        """Add an url for a platform path to the binary.
        @param binary[in,out] a dict containing all the details about a binary."""
        print """\
Please enter a new package location and url. Some examples:
common -- specify a package for all platforms
linux -- specify a package for all arch and compilers on linux
darwin/universal -- specify a mac os x universal
win32/i686/vs/2003 -- specify a windows visual studio 2003 package"""
        target = raw_input("Package path: ")
        url = raw_input("Package URL:  ")
        md5sum = raw_input("Package md5:  ")
        path = target.split('/')
        if not binary.has_key('packages'):
            binary['packages'] = {}
        update = binary['packages']
        for child in path:
            if not update.has_key(child):
                update[child] = {}
            parent = update
            update = update[child]
        parent[child]['url'] = llsd.uri(url)
        parent[child]['md5sum'] = md5sum

    def adopt_binary(self, name):
        "Interactively pull a new binary into the install"
        if not self._binaries.has_key(name):
            print "Adding binary '" + name + "'."
            self._binaries[name] = BinaryDefinition({})
        else:
            print "Updating binary '" + name + "'."
        binary  = self._binaries[name]._definition
        for field in ('copyright', 'license', 'description'):
            self._update_field(binary, field)
        self._add_package(binary)
        print "Adopted binary '" + name + "':"
        pprint.pprint(self._binaries[name])
        self._install_changed = True
        return True

    def orphan_binary(self, name):
        self._binaries.pop(name)
        self._install_changed = True

    def add_license(self, name, text, url):
        if self._licenses.has_key(name):
            print "License '" + name + "' being overwritten."
        definition = {}
        if url:
            definition['url'] = url
        if not url and text is None:
            print "Please enter license text. End input with EOF (^D)."
            text = sys.stdin.read()
            definition['text'] = text
        self._licenses[name] = LicenseDefinition(definition)
        self._install_changed = True
        return True

    def remove_license(self, name):
        self._licenses.pop(name)
        self._install_changed = True

    def _determine_install_set(self, ifiles):
        """@brief determine what to install
@param ifiles A list of InstallFile instances which are necessary for this install
@return Returns the tuple (ifiles to install, ifiles to remove)"""
        installed_list = []
        for package in self._installed:
            installed_list.extend(self._installed[package].urls())
        installed_set = set(installed_list)
        #print "installed_set:",installed_set
        install_list = [ifile.url for ifile in ifiles]
        install_set = set(install_list)
        #print "install_set:",install_set
        remove_set = installed_set.difference(install_set)
        to_remove = [ifile for ifile in ifiles if ifile.url in remove_set]
        #print "to_remove:",to_remove
        install_set = install_set.difference(installed_set)
        to_install = [ifile for ifile in ifiles if ifile.url in install_set]
        #print "to_install:",to_install
        return to_install, to_remove

    def _build_ifiles(self, platform, cache_dir):
        """@brief determine what files to install and remove
@param platform The target platform. Eg, win32 or linux/i686/gcc/3.3
@param cache_dir The directory to cache downloads.
@return Returns the tuple (ifiles to install, ifiles to remove)"""
        ifiles = []
        for bin in self._binaries:
            ifiles.extend(self._binaries[bin].ifiles(bin, platform, cache_dir))
        return self._determine_install_set(ifiles)
        
    def _remove(self, to_remove):
        remove_file_list = []
        for ifile in to_remove:
            remove_file_list.extend(
                self._installed[ifile.pkgname].files_in(ifile.url))
            self._installed[ifile.pkgname].remove(ifile.url)
            self._installed_changed = True
        for filename in remove_file_list:
            print "rm",filename
            if not self._dryrun:
                os.remove(filename)

    def _install(self, to_install, install_dir):
        for ifile in to_install:
            tar = tarfile.open(ifile.filename, 'r')
            print "Extracting",ifile.filename,"to destination",install_dir
            if not self._dryrun:
                # *NOTE: try to call extractall, which first appears
                # in python 2.5. Phoenix 2008-01-28
                try:
                    tar.extractall(path=install_dir)
                except AttributeError:
                    _extractall(tar, path=install_dir)
            if self._installed.has_key(ifile.pkgname):
                self._installed[ifile.pkgname].add_files(ifile.url, tar.getnames())
            else:
                # *HACK: this understands the installed package syntax.
                definition = { ifile.url : tar.getnames() }
                self._installed[ifile.pkgname] = InstalledPackage(definition)
            self._installed_changed = True

    def do_install(self, platform, install_dir, cache_dir):
        """@brief Do the installation for for the platform.
@param platform The target platform. Eg, win32 or linux/i686/gcc/3.3
@param install_dir The root directory to install into. Created if missing.
@param cache_dir The directory to cache downloads. Created if missing."""
        if not self._binaries:
            raise RuntimeError("No binaries to install. Please add them.")
        _mkdir(install_dir)
        _mkdir(cache_dir)
        to_install, to_remove = self._build_ifiles(platform, cache_dir)

        # we do this in multiple steps reduce the likelyhood to have a
        # bad install.
        for ifile in to_install:
            ifile.fetch_local()
        self._remove(to_remove)
        self._install(to_install, install_dir)


#
# *NOTE: PULLED FROM PYTHON 2.5 tarfile.py Phoenix 2008-01-28
#
def _extractall(tar, path=".", members=None):
    """Extract all members from the archive to the current working
       directory and set owner, modification time and permissions on
       directories afterwards. `path' specifies a different directory
       to extract to. `members' is optional and must be a subset of the
       list returned by getmembers().
    """
    directories = []

    if members is None:
        members = tar

    for tarinfo in members:
        if tarinfo.isdir():
            # Extract directory with a safe mode, so that
            # all files below can be extracted as well.
            try:
                os.makedirs(os.path.join(path, tarinfo.name), 0777)
            except EnvironmentError:
                pass
            directories.append(tarinfo)
        else:
            tar.extract(tarinfo, path)

    # Reverse sort directories.
    directories.sort(lambda a, b: cmp(a.name, b.name))
    directories.reverse()

    # Set correct owner, mtime and filemode on directories.
    for tarinfo in directories:
        path = os.path.join(path, tarinfo.name)
        try:
            tar.chown(tarinfo, path)
            tar.utime(tarinfo, path)
            tar.chmod(tarinfo, path)
        except tarfile.ExtractError, e:
            if tar.errorlevel > 1:
                raise
            else:
                tar._dbg(1, "tarfile: %s" % e)


def _mkdir(directory):
    "Safe, repeatable way to make a directory."
    try:
        os.makedirs(directory)
    except OSError, e:
        if e[0] != errno.EEXIST:
            raise

def _get_platform():
    "Return appropriate platform packages for the environment."
    platform_map = {
        'darwin': 'darwin',
        'linux2': 'linux',
        'win32' : 'win32',
        'cygwin' : 'win32',
        'solaris' : 'solaris'
        }
    return platform_map[sys.platform]

def main():
    parser = optparse.OptionParser(
        usage="usage: %prog [options]",
        formatter = helpformatter.Formatter(),
        description="""This script fetches and installs binary packages.

The process is to open and read an install manifest file which specifies
what files should be installed. For each file in the manifest:
 * make sure it has a license
 * check the installed version
 ** if not installed and needs to be, download and install
 ** if installed version differs, download & install

When specifying a platform, you can specify 'all' to install all
packages, or any platform of the form:

OS[/arch[/compiler[/compiler_version]]]

Where the supported values for each are:
OS: darwin, linux, win32, solaris
arch: i686, x86_64, ppc, universal
compiler: vs, gcc
compiler_version: 2003, 2005, 2008, 3.3, 3.4, 4.0, etc.

No checks are made to ensure a valid combination of platform
parts. Some exmples of valid platforms:

win32
win32/i686/vs/2005
linux/x86_64/gcc/3.3
linux/x86_64/gcc/4.0
darwin/universal/gcc/4.0
""")
    parser.add_option(
        '--dry-run', 
        action='store_true',
        default=False,
        dest='dryrun',
        help='Do not actually install files. Downloads will still happen.')
    parser.add_option(
        '--install-manifest', 
        type='string',
        default=join(base_dir, 'install.xml'),
        dest='install_filename',
        help='The file used to describe what should be installed.')
    parser.add_option(
        '--installed-manifest', 
        type='string',
        default=join(base_dir, 'installed.xml'),
        dest='installed_filename',
        help='The file used to record what is installed.')
    parser.add_option(
        '-p', '--platform', 
        type='string',
        default=_get_platform(),
        dest='platform',
        help="""Override the automatically determined platform. \
You can specify 'all' to do a complete installation of all binaries.""")
    parser.add_option(
        '--cache-dir', 
        type='string',
        default=join(base_dir, '.install.cache'),
        dest='cache_dir',
        help='Where to download files.')
    parser.add_option(
        '--install-dir', 
        type='string',
        default=base_dir,
        dest='install_dir',
        help='Where to unpack the installed files.')
    parser.add_option(
        '--skip-license-check', 
        action='store_false',
        default=True,
        dest='check_license',
        help="Do not perform the license check.")
    parser.add_option(
        '--add-license', 
        type='string',
        default=None,
        dest='new_license',
        help="""Add a license to the install file. Argument is the name of \
license. Specify --license-url if the license is remote or specify \
--license-text, otherwse the license text will be read from standard \
input.""")
    parser.add_option(
        '--remove-license', 
        type='string',
        default=None,
        dest='remove_license',
        help="Remove a named license.")
    parser.add_option(
        '--license-url', 
        type='string',
        default=None,
        dest='license_url',
        help="""Put the specified url into an added license. \
Ignored if --add-license is not specified.""")
    parser.add_option(
        '--license-text', 
        type='string',
        default=None,
        dest='license_text',
        help="""Put the text into an added license. \
Ignored if --add-license is not specified.""")
    parser.add_option(
        '--orphan', 
        type='string',
        default=None,
        dest='orphan',
        help="Remove a binary from the install file.")
    parser.add_option(
        '--adopt', 
        type='string',
        default=None,
        dest='adopt',
        help="""Add a binary into the install file. Argument is the name of \
the binary to add.""")
    parser.add_option(
        '--list', 
        action='store_true',
        default=False,
        dest='list_binaries',
        help="List the binaries in the install manifest")
    parser.add_option(
        '--details', 
        type='string',
        default=None,
        dest='detail_binary',
        help="Get detailed information on specified binary.")
    options, args = parser.parse_args()


    installer = Installer(
        options.install_filename,
        options.installed_filename,
        options.dryrun)

    #
    # Handle the queries for information
    #
    if options.list_binaries:
        print "binary list:",installer._binaries.keys()
        return 0
    if options.detail_binary:
        detail = installer.detail_binary(options.detail_binary)
        if detail:
            print "Detail on binary",options.detail_binary+":"
            pprint.pprint(detail)
        else:
            print "Bianry '"+options.detail_binary+"' not found in",
            print "install file."
        return 0

    #
    # Handle updates -- can only do one of these
    # *TODO: should this change the command line syntax?
    #
    if options.new_license:
        if not installer.add_license(
            options.new_license,
            options.license_text,
            options.license_url):
            return 1
    elif options.remove_license:
        installer.remove_license(options.remove_license)
    elif options.orphan:
        installer.orphan_binary(options.orphan)
    elif options.adopt:
        if not installer.adopt_binary(options.adopt):
            return 1
    else:
        if options.check_license:
            if not installer.is_license_info_valid():
                print >>sys.stderr, 'Please add or correct the license',
                print >>sys.stderr, 'information in',
                print >>sys.stderr, options.install_filename + '.'
                print >>sys.stderr, "You can also use the --add-license",
                print >>sys.stderr, "option. See", sys.argv[0], "--help"
                return 1

        # *TODO: check against a list of 'known good' licenses.
        # *TODO: check for urls which conflict -- will lead to
        # problems.

        installer.do_install(
            options.platform,
            options.install_dir,
            options.cache_dir)

    # save out any changes
    installer.save()
    return 0

if __name__ == '__main__':
    sys.exit(main())
