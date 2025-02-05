#!/usr/bin/env python3
"""\
@file   watchdog.py
@author Nat Goodspeed
@date   2025-01-28
@brief  Monitor viewer termination; send crash data on crash.

$LicenseInfo:firstyear=2025&license=viewerlgpl$
Copyright (c) 2025, Linden Research, Inc.
$/LicenseInfo$
"""

import __main__
from datetime import datetime, timezone
import hashlib
import json
import llsd
import logging
import os
from pathlib import Path
import platform
import re
import subprocess
import sys
from tkinter.messagebox import showerror
from uuid import UUID

mydir = Path(__file__).parent
myname = Path(__main__.__file__).stem

class Error(Exception):
    pass

class JSONLLSDEncoder(json.JSONEncoder):
    """
    The Python `llsd` module returns most exotic LLSD data types as Python
    strings. But it does return:
    * LLSD dates as Python datetime.datetime objects
    * LLSD UUIDs as Python uuid.UUID objects
    which the json module, by default, does not handle. Extend the encoder to
    serialize these as string instead of crumping.
    """
    def default(self, obj):
        if isinstance(obj, (datetime, UUID)):
            return str(obj)
        return super().default(obj)

def main(logdir):
    errors = []

##  from argparse import ArgumentParser
##  parser = ArgumentParser(description="%(prog)s")
##  #parser.add_argument()
##  args = parser.parse_args(raw_args)

    logdir = Path(logdir)
    logdir.mkdir(parents=True, exist_ok=True)

    # When we first wake up, check if there's a backlog of pending crash
    # reports we couldn't previously send.
    crashes_json = logdir / 'crashes.json'
    try:
        inf = crashes_json.open()
    except FileNotFoundError:
        crashes = []
    else:
        with inf:
            crashes = json.load(inf)

        if not send_report('backlog', crashes):
            errors.append(f"Can't send backlog of {len(crashes)} crashes")
        else:
            # We were successful in clearing out the backlog, so start fresh.
            crashes = []
            crashes_json.unlink()

    # Our job is to notice when the viewer terminates. We spend most of our
    # lifetime hanging on a read of stdin. When the viewer terminates, in any
    # way, stdin will close and the read will complete. If the viewer writes
    # 'OK' first, it's intentional, a normal termination. If not -- well,
    # something went wrong.
    OK = sys.stdin.buffer.read(2)
    if OK == b'OK':
        # Viewer shut down successfully.
        return

    # All of what follows is when we believe the viewer crashed.
    # Get timestamp in case our setup takes any real time.
    viewerdown = datetime.now(timezone.utc)

    # Set up logging
    logging.basicConfig(filename=logdir / (myname + '.log'),
                        format='{asctime} {message}', style='{',
                        datefmt='%Y-%m-%d %H:%M:%S',
                        level=logging.DEBUG)
    logging.info(72*'=')
    logging.error('Viewer crashed!')

    info_log = logdir / 'static_debug_info.log'
    try:
        inf = open(info_log, 'rb')
    except FileNotFoundError:
        errors.append(f"Can't open {info_log}")
        logging.error(errors[-1])
        crash = {}
    else:
        # Start with the viewer's static_debug_info.log as the bulk of this
        # new crash report.
        with inf:
            crash = llsd.parse(inf)

    crashes.append(crash)

    # Supplement by recording the timestamp and the duration of the crashed
    # session. We use the timestamp of SecondLife.start_marker to determine
    # both.
    startfile = logdir / 'SecondLife.start_marker'
    try:
        stat = startfile.stat()
    except FileNotFoundError:
        # If the viewer didn't even write SecondLife.start_marker, we can't
        # report its timestamp. We may be fooled if a prior viewer run did
        # write SecondLife.start_marker, then the run that just terminated
        # didn't. We'll pick up on the previous run's marker file.
        errors.append(f"Can't stat {startfile}")
        logging.error(errors[-1])
    else:
        # st_birthtime should be the creation timestamp of that file. But the
        # viewer simply overwrites the SecondLife.start_marker file, so its
        # st_birthtime goes back to the first viewer run on this filesystem.
        # Same thing for st_ctime, the older "creation time" field. Have to
        # rely on st_mtime, the most recent file modification time.
        # Weirdly, at least on Windows, the st_birthtime and st_ctime of
        # SecondLife.log *also* go back to that first viewer run, even though
        # every viewer run renames SecondLife.log to SecondLife.old and
        # creates a new SecondLife.log.
        # We check SecondLife.start_marker instead of SecondLife.log because
        # the st_mtime of SecondLife.log reflects the *end* of the crashed
        # session, not the beginning.
        logstart = datetime.fromtimestamp(stat.st_mtime, timezone.utc)
        duration = viewerdown - logstart
        minutes, seconds = divmod(int(duration.total_seconds()), 60)
        hours,   minutes = divmod(minutes, 60)
        crash['Date'] = logstart.isoformat(timespec='seconds')
        crash['Duration'] = f'{hours:02}:{minutes:02}:{seconds:02}'

    try:
        machine_id = make_VVM_UUID_hash()
    except Error as err:
        err = str(err)
        logging.error(err)
        errors.append(err)
    else:
        crash['MachineID'] = machine_id

    if errors:
        crash['Errors'] = errors

    # Capture pending crashes in case we need to try again next time. Write
    # the file first, remove it if successful, to guard against crashing
    # during the attempt to send.
    with crashes_json.open('w') as outf:
        json.dump(crashes, outf, indent=4, ensure_ascii=False, cls=JSONLLSDEncoder)
    if send_report('new', crashes):
        crashes_json.unlink()

def send_report(desc, report):
    # For diagnostic purposes, dump the report to our log file.
    logging.debug(f'Sending {desc}:')
    streport = json.dumps(report, indent=4, ensure_ascii=False, cls=JSONLLSDEncoder)
    for line in streport.splitlines():
        logging.debug(line)
    # pretend we successfully sent
    return True

## Adapted from viewer-manager:
## https://github.com/secondlife/viewer-manager/blob/master/src/update_manager.py#L263-L313
## There's no compelling reason to pass the same MachineID as the machine ID
## passed to the Viewer Version Manager, but we already have this code.
def make_VVM_UUID_hash():
    #NOTE: There is no python library support for a persistent machine
    #      specific UUID (MUUID), AND all three platforms do this a different
    #      way, so exec'ing out is really the best we can do
    #for env without stdin, such as pythonw and pyinstaller, provide a legit empty handle, not the broken
    #thing we get from the env.
    if (platform.system() == 'Linux'):
        muuid = get_output('/usr/bin/hostid')
        logging.debug("result of subprocess call to get linux MUUID: %r" % muuid)

    elif (platform.system() == 'Darwin'):
        #this is absurdly baroque
        #/usr/sbin/system_profiler SPHardwareDataType | fgrep 'Serial' | awk '{print $NF}'
        #also note that this causes spurious messages about X86PlatformPlugin in the log from stderr
        # ignoring per https://tickets.puppetlabs.com/browse/FACT-724, stdout is correct, stderr is noise
        profiler_cmd=[]
        muuid = get_output("/usr/sbin/system_profiler", "SPHardwareDataType")
        #findall[0] does the grep for the value we are looking for: "Serial Number (system): XXXXXXXX"
        #split(:)[1] gets us the XXXXXXX part
        #lstrip shaves off the leading space that was after the colon
        muuid = re.split(b":", re.findall(rb'Serial Number \(system\): \S*', muuid)[0])[1].lstrip()
        logging.debug("result of subprocess call to get mac MUUID: %r" % muuid)

    elif (platform.system() == 'Windows'):
        # powershell has a canonical pathname that might or might not be on the PATH.
        pshell = Path("C:/Windows/System32/WindowsPowerShell/v1.0/powershell.exe")
        if not pshell.is_file():
            # powershell not in usual place -- better hope it's on PATH!
            pshell = "powershell"
        # pshell csproduct get UUID | grep -v UUID
        muuid = get_output(pshell, '-Command',
                           r'CimCmdlets\Get-CimInstance -ClassName Win32_ComputerSystemProduct | Select-Object -ExpandProperty UUID')
        #outputs row:
        #XXXXXXX-XXXX...
        # but splitlines() produces a whole lot of empty strings.
        muuid = [line for line in muuid.splitlines() if line][-1].rstrip()
        logging.debug("result of subprocess call to get win MUUID: %r" % muuid)

    else:
        #fake it
        logging.info(f"Unable to get {platform.system()} system unique id; constructing a dummy")
        muuid = str(uuid.uuid1()).encode('utf8')

    # hashlib requires a bytes object, not a str
    return hashlib.md5(muuid).hexdigest()

def get_output(*command):
    try:
        # For this use case we want bytes, not str.
        return subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                              check=True, text=False).stdout.rstrip()
    except FileNotFoundError as err:
        raise Error(str(err))
    except subprocess.CalledProcessError as err:
        raise Error(f'{err}:\n{err.stderr.decode("utf8")}')

if __name__ == "__main__":
    try:
        sys.exit(main(*sys.argv[1:]))
    except Error as err:
        err = str(err)
        logging.error(err)
        showerror(myname, err)
        sys.exit(err)
