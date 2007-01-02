#!/usr/bin/python
#
# Update all of the various files in the repository to a new version number,
# instead of having to figure it out by hand
#

import getopt, sys, os, re, commands

def _getstatusoutput(cmd):
    """Return Win32 (status, output) of executing cmd
in a shell."""
    pipe = os.popen(cmd, 'r')
    text = pipe.read()
    sts = pipe.close()
    if sts is None: sts = 0
    if text[-1:] == '\n': text = text[:-1]
    return sts, text

re_map = {}

#re_map['filename'] = (('pattern', 'replacement'),
#                      ('pattern', 'replacement')
re_map['indra/llcommon/llversion.h'] = \
	(('const S32 LL_VERSION_MAJOR = (\d+);',
	  'const S32 LL_VERSION_MAJOR = %(VER_MAJOR)s;'),
	 ('const S32 LL_VERSION_MINOR = (\d+);',
	  'const S32 LL_VERSION_MINOR = %(VER_MINOR)s;'),
	 ('const S32 LL_VERSION_PATCH = (\d+);',
	  'const S32 LL_VERSION_PATCH = %(VER_PATCH)s;'),
	 ('const S32 LL_VERSION_BUILD = (\d+);',
	  'const S32 LL_VERSION_BUILD = %(VER_BUILD)s;'))
re_map['indra/newview/res/newViewRes.rc'] = \
	(('FILEVERSION [0-9,]+',
	  'FILEVERSION %(VER_MAJOR)s,%(VER_MINOR)s,%(VER_PATCH)s,%(VER_BUILD)s'),
	 ('PRODUCTVERSION [0-9,]+',
	  'PRODUCTVERSION %(VER_MAJOR)s,%(VER_MINOR)s,%(VER_PATCH)s,%(VER_BUILD)s'),
	 ('VALUE "FileVersion", "[0-9.]+"',
	  'VALUE "FileVersion", "%(VER_MAJOR)s.%(VER_MINOR)s.%(VER_PATCH)s.%(VER_BUILD)s"'),
	 ('VALUE "ProductVersion", "[0-9.]+"',
	  'VALUE "ProductVersion", "%(VER_MAJOR)s.%(VER_MINOR)s.%(VER_PATCH)s.%(VER_BUILD)s"'))

# Trailing ',' in top level tuple is special form to avoid parsing issues with one element tuple
re_map['indra/newview/Info-SecondLife.plist'] = \
	(('<key>CFBundleVersion</key>\n\t<string>[0-9.]+</string>',
	  '<key>CFBundleVersion</key>\n\t<string>%(VER_MAJOR)s.%(VER_MINOR)s.%(VER_PATCH)s.%(VER_BUILD)s</string>'),)

# This will probably only work as long as InfoPlist.strings is NOT UTF16, which is should be...
re_map['indra/newview/English.lproj/InfoPlist.strings'] = \
	(('CFBundleShortVersionString = "Second Life version [0-9.]+";',
	  'CFBundleShortVersionString = "Second Life version %(VER_MAJOR)s.%(VER_MINOR)s.%(VER_PATCH)s.%(VER_BUILD)s";'),
	 ('CFBundleGetInfoString = "Second Life version [0-9.]+',
	  'CFBundleGetInfoString = "Second Life version %(VER_MAJOR)s.%(VER_MINOR)s.%(VER_PATCH)s.%(VER_BUILD)s'))


version_re = re.compile('(\d+).(\d+).(\d+).(\d+)')
svn_re = re.compile('Last Changed Rev: (\d+)')

def main():
	script_path = os.path.dirname(__file__)
	src_root = script_path + "/../"
	verbose = False

	# Get version number from llversion.h
	full_fn = src_root + '/' + 'indra/llcommon/llversion.h'
	file = open(full_fn,"r")
	file_str = file.read()
	file.close()
	
	m = re.search('const S32 LL_VERSION_MAJOR = (\d+);', file_str)
	VER_MAJOR = m.group(1)
	m = re.search('const S32 LL_VERSION_MINOR = (\d+);', file_str)
	VER_MINOR = m.group(1)
	m = re.search('const S32 LL_VERSION_PATCH = (\d+);', file_str)
	VER_PATCH = m.group(1)
	m = re.search('const S32 LL_VERSION_BUILD = (\d+);', file_str)
	VER_BUILD = m.group(1)

	opts, args = getopt.getopt(sys.argv[1:],
							   "",
							   ['version=', 'verbose'])

	version_string = None
	for o,a in opts:
		if o in ('--version'):
			version_string = a
		if o in ('--verbose'):
			verbose = True

	if verbose:
		print "Source Path:", src_root
		print "Current version: %(VER_MAJOR)s.%(VER_MINOR)s.%(VER_PATCH)s.%(VER_BUILD)s" % locals()

	if version_string:
		m = version_re.match(version_string)
		if not m:
			print "Invalid version string specified!"
			return -1
		VER_MAJOR = m.group(1)
		VER_MINOR = m.group(2)
		VER_PATCH = m.group(3)
		VER_BUILD = m.group(4)
	else:
		# Assume we're updating just the build number
		cl = 'svn info "%s"' % src_root
		status, output = _getstatusoutput(cl)
		#print
		#print "svn info output:"
		#print "----------------"
		#print output
		m = svn_re.search(output)
		if not m:
			print "Failed to execute svn info, output follows:"
			print output
			return -1
		VER_BUILD = m.group(1)

	if verbose:
		print "New version: %(VER_MAJOR)s.%(VER_MINOR)s.%(VER_PATCH)s.%(VER_BUILD)s" % locals()
		print

	# Grab the version numbers off the command line
	# Iterate through all of the files in the map, and apply the substitution filters
	for filename in re_map.keys():
		# Read the entire file into a string
		full_fn = src_root + '/' + filename
		file = open(full_fn,"r")
		file_str = file.read()
		file.close()

		if verbose:
			print "Processing file:",filename
		for rule in re_map[filename]:
			repl = rule[1] % locals()
			file_str = re.sub(rule[0], repl, file_str)

		file = open(full_fn,"w")
		file.write(file_str)
		file.close()
	return 0

main()
