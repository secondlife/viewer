#!/usr/bin/env python3
"""\
@file   profile_cmp.py
@author Nat Goodspeed
@date   2024-09-13
@brief  Compare a frame profile stats file with a similar baseline file.

$LicenseInfo:firstyear=2024&license=viewerlgpl$
Copyright (c) 2024, Linden Research, Inc.
$/LicenseInfo$
"""

from datetime import datetime
import json
from logsdir import Error, latest_file, logsdir
from pathlib import Path
import sys

# variance that's ignorable
DEFAULT_EPSILON = 0.03          # 3%

def compare(baseline, test, epsilon=DEFAULT_EPSILON):
    if Path(baseline).samefile(test):
        print(f'{baseline} same as\n{test}\nAnalysis moot.')
        return

    with open(baseline) as inf:
        bdata = json.load(inf)
    with open(test) as inf:
        tdata = json.load(inf)
    print(f'baseline {baseline}\ntestfile {test}')

    for k, tv in tdata['context'].items():
        bv = bdata['context'].get(k)
        if bv != tv:
            print(f'baseline {k}={bv} vs.\ntestfile {k}={tv}')

    btime = bdata['context'].get('time')
    ttime = tdata['context'].get('time')
    if btime and ttime:
        print('testfile newer by',
              datetime.fromisoformat(ttime) - datetime.fromisoformat(btime))

    # The following ignores totals and unused shaders, except to the extent
    # that some shaders were used in the baseline but not in the recent test
    # or vice-versa. While the viewer considers that a shader has been used if
    # 'binds' is nonzero, we exclude any whose 'time' is zero to avoid zero
    # division.
    bshaders = {s['name']: s for s in bdata['shaders'] if s['time'] and s['samples']}
    tshaders = {s['name']: s for s in tdata['shaders'] if s['time']}

    bothshaders = set(bshaders).intersection(tshaders)
    deltas = []
    for shader in bothshaders:
        bshader = bshaders[shader]
        tshader = tshaders[shader]
        bthruput = bshader['samples']/bshader['time']
        tthruput = tshader['samples']/tshader['time']
        delta = (tthruput - bthruput)/bthruput
        if abs(delta) > epsilon:
            deltas.append((delta, shader, bthruput, tthruput))

    # descending order of performance gain
    deltas.sort(reverse=True)
    print(f'{len(deltas)} shaders showed nontrivial performance differences '
          '(millon samples/sec):')
    namelen = max(len(s[1]) for s in deltas) if deltas else 0
    for delta, shader, bthruput, tthruput in deltas:
        print(f'  {shader.rjust(namelen)} {delta*100:6.1f}% '
              f'{bthruput/1000000:8.2f} -> {tthruput/1000000:8.2f}')

    tunused = set(bshaders).difference(tshaders)
    print(f'{len(tunused)} baseline shaders not used in test:')
    for s in tunused:
        print(f'  {s}')
    bunused = set(tshaders).difference(bshaders)
    print(f'{len(bunused)} shaders newly used in test:')
    for s in bunused:
        print(f'  {s}')

def main(*raw_args):
    from argparse import ArgumentParser
    parser = ArgumentParser(description="""
%(prog)s compares a baseline JSON file from Develop -> Render Tests -> Frame
Profile to another such file from a more recent test. It identifies shaders
that have gained and lost in throughput.
""")
    parser.add_argument('-e', '--epsilon', type=float, default=int(DEFAULT_EPSILON*100),
                        help="""percent variance considered ignorable (default %(default)s%%)""")
    parser.add_argument('baseline',
                        help="""baseline profile filename to compare against""")
    parser.add_argument('test', nargs='?',
                        help="""test profile filename to compare
                        (default is most recent)""")
    args = parser.parse_args(raw_args)
    compare(args.baseline,
            args.test or latest_file(logsdir(), 'profile.*.json'),
            epsilon=(args.epsilon / 100.))

if __name__ == "__main__":
    try:
        sys.exit(main(*sys.argv[1:]))
    except (Error, OSError, json.JSONDecodeError) as err:
        sys.exit(str(err))
