#!/usr/bin/python

# ----------------------------------------------------
# Utility to extract log messages from *.<pid>.llsd
# files that contain performance statistics.

# ----------------------------------------------------
import sys, os

if os.path.exists("setup-path.py"):
    execfile("setup-path.py")

from indra.base import llsd

DEFAULT_PATH="/dev/shm/simperf/"


# ----------------------------------------------------
# Pull out the stats and return a single document
def parse_logfile(filename, target_column=None, verbose=False):
    full_doc = []
    # Open source temp log file.  Let exceptions percolate up.
    sourcefile = open( filename,'r')
        
    if verbose:
        print "Reading " + filename  
    
    # Parse and output all lines from the temp file
    for line in sourcefile.xreadlines():
        partial_doc = llsd.parse(line)
        if partial_doc is not None:
            if target_column is None:
                full_doc.append(partial_doc)
            else:
                trim_doc = { target_column: partial_doc[target_column] }
                if target_column != "fps":
                    trim_doc[ 'fps' ] = partial_doc[ 'fps' ]
                trim_doc[ '/total_time' ] = partial_doc[ '/total_time' ]
                trim_doc[ 'utc_time' ] = partial_doc[ 'utc_time' ]
                full_doc.append(trim_doc)

    sourcefile.close()
    return full_doc

# Extract just the meta info line, and the timestamp of the first/last frame entry.
def parse_logfile_info(filename, verbose=False):
    # Open source temp log file.  Let exceptions percolate up.
    sourcefile = open(filename, 'rU') # U is to open with Universal newline support
        
    if verbose:
        print "Reading " + filename  

    # The first line is the meta info line.
    info_line = sourcefile.readline()
    if not info_line:
        sourcefile.close()
        return None

    # The rest of the lines are frames.  Read the first and last to get the time range.
    info = llsd.parse( info_line )
    info['start_time'] = None
    info['end_time'] = None
    first_frame = sourcefile.readline()
    if first_frame:
        try:
            info['start_time'] = int(llsd.parse(first_frame)['timestamp'])
        except:
            pass

    # Read the file backwards to find the last two lines.
    sourcefile.seek(0, 2)
    file_size = sourcefile.tell()
    offset = 1024
    num_attempts = 0
    end_time = None
    if file_size < offset:
        offset = file_size
    while 1:
        sourcefile.seek(-1*offset, 2)
        read_str = sourcefile.read(offset)
        # Remove newline at the end
        if read_str[offset - 1] == '\n':
            read_str = read_str[0:-1]
        lines = read_str.split('\n')
        full_line = None
        if len(lines) > 2:  # Got two line
            try:
                end_time = llsd.parse(lines[-1])['timestamp']
            except:
                # We couldn't parse this line.  Try once more.
                try:
                    end_time = llsd.parse(lines[-2])['timestamp']
                except:
                    # Nope.  Just move on.
                    pass
            break
        if len(read_str) == file_size:   # Reached the beginning
            break
        offset += 1024

    info['end_time'] = int(end_time)

    sourcefile.close()
    return info
    

def parse_proc_filename(filename):
    try:
        name_as_list = filename.split(".")
        cur_stat_type = name_as_list[0].split("_")[0]
        cur_pid = name_as_list[1]
    except IndexError, ValueError:
        return (None, None)
    return (cur_pid, cur_stat_type)

# ----------------------------------------------------
def get_simstats_list(path=None):
    """ Return stats (pid, type) listed in <type>_proc.<pid>.llsd """
    if path is None:
        path = DEFAULT_PATH
    simstats_list = []
    for file_name in os.listdir(path):
        if file_name.endswith(".llsd") and file_name != "simperf_proc_config.llsd":
            simstats_info = parse_logfile_info(path + file_name)
            if simstats_info is not None:
                simstats_list.append(simstats_info)
    return simstats_list

def get_log_info_list(pid=None, stat_type=None, path=None, target_column=None, verbose=False):
    """ Return data from all llsd files matching the pid and stat type """
    if path is None:
        path = DEFAULT_PATH
    log_info_list = {}
    for file_name in os.listdir ( path ):
        if file_name.endswith(".llsd") and file_name != "simperf_proc_config.llsd":
            (cur_pid, cur_stat_type) = parse_proc_filename(file_name)
            if cur_pid is None:
                continue
            if pid is not None and pid != cur_pid:
                continue
            if stat_type is not None and stat_type != cur_stat_type:
                continue
            log_info_list[cur_pid] = parse_logfile(path + file_name, target_column, verbose)
    return log_info_list

def delete_simstats_files(pid=None, stat_type=None, path=None):
    """ Delete *.<pid>.llsd files """
    if path is None:
        path = DEFAULT_PATH
    del_list = []
    for file_name in os.listdir(path):
        if file_name.endswith(".llsd") and file_name != "simperf_proc_config.llsd":
            (cur_pid, cur_stat_type) = parse_proc_filename(file_name)
            if cur_pid is None:
                continue
            if pid is not None and pid != cur_pid:
                continue
            if stat_type is not None and stat_type != cur_stat_type:
                continue
            del_list.append(cur_pid)
            # Allow delete related exceptions to percolate up if this fails.
            os.unlink(os.path.join(DEFAULT_PATH, file_name))
    return del_list

