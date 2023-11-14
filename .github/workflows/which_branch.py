#!/usr/bin/env python3
"""\
@file   which_branch.py
@author Nat Goodspeed
@date   2023-11-14
@brief  Discover which git branch(es) correspond to a given commit hash.

$LicenseInfo:firstyear=2023&license=viewerlgpl$
Copyright (c) 2023, Linden Research, Inc.
$/LicenseInfo$
"""

import github
import re
import sys
import subprocess

class Error(Exception):
    pass

def branches_for(token, commit, repo=None):
    """
    Use the GitHub REST API to discover which branch(es) correspond to the
    passed commit hash. The commit string can actually be any of the ways git
    permits to identify a commit:

    https://git-scm.com/docs/gitrevisions#_specifying_revisions

    branches_for() generates a (possibly empty) sequence of all the branches
    of the specified repo for which the specified commit is the tip.

    If repo is omitted or None, assume the current directory is a local clone
    whose 'origin' remote is the GitHub repository of interest.
    """
    if not repo:
        url = subprocess.check_output(['git', 'remote', 'get-url', 'origin'],
                                      text=True)
        parts = re.split(r'[:/]', url.rstrip())
        repo = '/'.join(parts[-2:]).removesuffix('.git')

    gh = github.MainClass.Github(token)
    grepo = gh.get_repo(repo)
    for branch in grepo.get_branches():
        try:
            delta = grepo.compare(base=commit, head=branch.name)
        except github.GithubException:
            continue

        if delta.ahead_by == 0 and delta.behind_by == 0:
            yield branch

def main(*raw_args):
    from argparse import ArgumentParser
    parser = ArgumentParser(description=
"%(prog)s reports the branch(es) for which the specified commit hash is the tip.",
                            epilog="""\
When GitHub Actions launches a tag build, it checks out the specific changeset
identified by the tag, and so 'git branch' reports detached HEAD. But we use
tag builds to build a GitHub 'release' of the tip of a particular branch, and
it's useful to be able to identify which branch that is.
""")
    parser.add_argument('-t', '--token', required=True,
                        help="""GitHub REST API access token""")
    parser.add_argument('-r', '--repo',
                        help="""GitHub repository name, in the form OWNER/REPOSITORY""")
    parser.add_argument('commit',
                        help="""commit hash at the tip of the sought branch""")

    args = parser.parse_args(raw_args)
    for branch in branches_for(token=args.token, commit=args.commit, repo=args.repo):
        print(branch.name)

if __name__ == "__main__":
    try:
        sys.exit(main(*sys.argv[1:]))
    except Error as err:
        sys.exit(str(err))
