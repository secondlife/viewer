#!/usr/bin/env python3
"""\
@file   flatten_files.py
@author Nat Goodspeed
@date   2023-08-18
@brief  From an input directory tree, populate a single flat output directory.

$LicenseInfo:firstyear=2023&license=viewerlgpl$
Copyright (c) 2023, Linden Research, Inc.
$/LicenseInfo$
"""

DESCRIPTION = """\
From an input directory tree, populate a single flat output directory.

For files with colliding names, rename them to unambiguous names derived from
their relative pathname within the input tree.

This is useful when downloading GitHub build artifacts from multiple platforms
to post them all as release assets without collisions.
"""

from collections import defaultdict
from contextlib import suppress
import filecmp
import os
from pathlib import Path
import sys

class Error(Exception):
    pass

def flatten(output, input='.', dry_run=False):
    try:
        in_stat = os.stat(input)
    except FileNotFoundError as err:
        raise Error(f'{input} does not exist') from err

    try:
        out_stat = os.stat(output)
    except FileNotFoundError:
        # output doesn't yet exist - at this point that's okay
        out_stat = None

    # use samestat() to avoid being fooled by different ways of expressing the
    # same path
    if out_stat and os.path.samestat(out_stat, in_stat):
        # output directory same as input: in this case, don't prune output
        # directory from input tree walk because we'd prune everything
        out_stat = None
    elif out_stat:
        # distinct existing output directory (potentially containing, or
        # contained by, the input directory)
        outfiles = [f for f in Path(output).rglob('*') if f.is_file()]
        if outfiles:
            print(f'Warning: {output} already contains {len(outfiles)} files:', file=sys.stderr)
            for f in sorted(outfiles):
                print('  ', f.relative_to(output), file=sys.stderr)

    # Use os.walk() instead of Path.rglob() so we can prune unwanted
    # directories.
    infiles = []
    for parent, dirs, files in os.walk(input):
        infiles.extend(Path(parent, f) for f in files)
        # Prune directories: because we must modify the dirs list in-place,
        # and because we're using indexes, traverse backwards so deletion
        # won't affect subsequent iterations. Yes we really must subtract 1
        # that many times.
        for idx in range(len(dirs)-1, -1, -1):
            if dirs[idx].startswith('.'):
                # skip dot-named directories
                print(f'ignoring {dirs[idx]}', file=sys.stderr)
                del dirs[idx]
            elif out_stat and os.path.samestat(os.stat(os.path.join(parent, dirs[idx])), out_stat):
                # output directory lives under input directory: ignore any
                # previous contents
                print(f'ignoring nested output directory {os.path.join(parent, dirs[idx])}',
                      file=sys.stderr)
                del dirs[idx]

    # Now that we've traversed the input tree, create the output directory if
    # needed.
    output = Path(output)
    output.mkdir(parents=True, exist_ok=True)

    # group files by basename to identify collisions
    basenames = defaultdict(list)
    for f in infiles:
        basenames[f.name].append(f)

    # output names: populate it right away with unique basenames
    outnames = { name: files[0] for name, files in basenames.items()
                 if len(files) == 1 }

    # now focus on the collisions
    for name, files in basenames.items():
        if len(files) <= 1:
            continue

        # Special case: are these colliding files equal? e.g. viewer_version.txt
        # Pass shallow=False so we actually read the files in question. Even
        # if they're identical, they've been downloaded from different
        # artifacts and have different timestamps (which would fool the default
        # shallow=True). This could be time-consuming if we were comparing two
        # of our very large files, but (a) our very large files already have
        # distinct names and so don't reach this call and (b) if we somehow do
        # wind up in that situation, it would be even more important to post
        # only a single copy.
        if all(filecmp.cmp(files[0], f, shallow=False) for f in files[1:]):
            # pick only one of them and use its simple basename
            outnames[name] = files[0]
            continue

        # Because of our intended use for GitHub Actions build artifacts, we
        # assume the top-level artifact names are descriptive. We'd still like
        # to eliminate mid-level directory names that don't help disambiguate,
        # so for instance, given:
        # Windows metadata/newview/viewer_version.txt
        # macOS metadata/newview/viewer_version.txt
        # we see no reason to retain the 'newview' pathname component. Try
        # longer and longer prefixes of the pathname parents. (But don't
        # forget to trim off the original input directory pathname.)
        filepairs = [(f, f.relative_to(input)) for f in files]
        partslen = max(len(rel.parts) for f, rel in filepairs)
        # skip the basename itself, we'll append that explicitly
        for prefixlen in range(partslen - 1):
            # Consider these relative names (shouldn't occur, but...):
            # parent/autobuild-package.xml
            # parent/newview/autobuild-package.xml
            # Unless these are in fact identical, they'll collide, meaning
            # we'll see them here. But beware their unequal numbers of parts.
            # partslen will be 3, so prefixlen will be 0, 1 -- but unless we
            # constrain it with min(), for prefixlen == 1 we'd construct:
            # ('parent', 'autobuild-package.xml', 'autobuild-package.xml')
            # ('parent', 'newview', 'autobuild-package.xml')
            # whereas of course the correct answer would be:
            # ('parent', 'autobuild-package.xml')
            # ('parent', 'newview', 'autobuild-package.xml')
            # Since we already know the basename is identical for every f in
            # files, though, we can omit it from our uniqueness testing.
            trynames = { rel.parts[:min(prefixlen+1, len(rel.parts)-1)]: f
                         for f, rel in filepairs }
            if len(trynames) == len(files):
                # Found a prefix without collisions -- note that we're
                # guaranteed to get here eventually since the full paths are
                # distinct in the filesystem, we just want to try to shorten.
                # Path.parts is specifically documented to be a tuple. Join
                # the key tuple with some delimiter acceptable to the
                # filesystem.
                outnames.update(('-'.join(nametuple + (name,)), f)
                                for nametuple, f in trynames.items())
                # stop considering longer prefixlens
                break

    # at this point outnames should have distinct keys -- move to the output
    # directory
    for name, f in outnames.items():
        newname = output / name
        if (not dry_run) and newname != f:
            newname = f.rename(newname)
        print(f'{f} => {newname}')

def main(*raw_args):
    from argparse import ArgumentParser
    parser = ArgumentParser(description=DESCRIPTION)
    parser.add_argument('-n', '--dry-run', action='store_true', default=False,
                        help="""show what would happen without moving files""")
    parser.add_argument('output', metavar='OUTDIR',
                        help="""populate OUTDIR with (possibly renamed) files""")
    parser.add_argument('input', metavar='INDIR', nargs='?', default='.',
                        help="""recursively read files under INDIR tree""")

    args = parser.parse_args(raw_args)
    flatten(args.output, args.input, dry_run=args.dry_run)

if __name__ == "__main__":
    try:
        sys.exit(main(*sys.argv[1:]))
    except Error as err:
        sys.exit(str(err))
