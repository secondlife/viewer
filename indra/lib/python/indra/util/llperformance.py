#!/usr/bin/python

# ------------------------------------------------
# Sim metrics utility functions.

import glob, os, time, sys, stat, exceptions

from indra.base import llsd

gBlockMap = {}              #Map of performance metric data with function hierarchy information.
gCurrentStatPath = ""

gIsLoggingEnabled=False

class LLPerfStat:
    def __init__(self,key):
        self.mTotalTime = 0
        self.mNumRuns = 0
        self.mName=key
        self.mTimeStamp = int(time.time()*1000)
        self.mUTCTime = time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime())

    def __str__(self):
        return "%f" %  self.mTotalTime

    def start(self):
        self.mStartTime = int(time.time() * 1000000)
        self.mNumRuns += 1

    def stop(self):
        execution_time = int(time.time() * 1000000) - self.mStartTime
        self.mTotalTime += execution_time

    def get_map(self):
        results={}
        results['name']=self.mName
        results['utc_time']=self.mUTCTime
        results['timestamp']=self.mTimeStamp
        results['us']=self.mTotalTime
        results['count']=self.mNumRuns
        return results

class PerfError(exceptions.Exception):
    def __init__(self):
        return

    def __Str__(self):
        print "","Unfinished LLPerfBlock"

class LLPerfBlock:
    def __init__( self, key ):
        global gBlockMap
        global gCurrentStatPath
        global gIsLoggingEnabled

        #Check to see if we're running metrics right now.
        if gIsLoggingEnabled:
            self.mRunning = True        #Mark myself as running.
    
            self.mPreviousStatPath = gCurrentStatPath
            gCurrentStatPath += "/" + key
            if gCurrentStatPath not in gBlockMap:
                gBlockMap[gCurrentStatPath] = LLPerfStat(key)

            self.mStat = gBlockMap[gCurrentStatPath]
            self.mStat.start()
    
    def finish( self ):
        global gBlockMap
        global gIsLoggingEnabled

        if gIsLoggingEnabled:
            self.mStat.stop()
            self.mRunning = False
            gCurrentStatPath = self.mPreviousStatPath

#    def __del__( self ):
#        if self.mRunning:
#            #SPATTERS FIXME
#            raise PerfError

class LLPerformance:
    #--------------------------------------------------
    # Determine whether or not we want to log statistics

    def __init__( self, process_name = "python" ):
        self.process_name = process_name
        self.init_testing()
        self.mTimeStamp = int(time.time()*1000)
        self.mUTCTime = time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime())

    def init_testing( self ):
        global gIsLoggingEnabled

        host_performance_file = "/dev/shm/simperf/simperf_proc_config.llsd"
    
        #If file exists, open
        if os.path.exists(host_performance_file):
            file = open (host_performance_file,'r')
    
            #Read serialized LLSD from file.
            body = llsd.parse(file.read())
    
            #Calculate time since file last modified.
            stats = os.stat(host_performance_file)
            now = time.time()
            mod = stats[stat.ST_MTIME]
            age = now - mod
    
            if age < ( body['duration'] ):
                gIsLoggingEnabled = True
    

    def get ( self ):
        global gIsLoggingEnabled
        return gIsLoggingEnabled

    #def output(self,ptr,path):
    #    if 'stats' in ptr:
    #        stats = ptr['stats']
    #        self.mOutputPtr[path] = stats.get_map()

    #    if 'children' in ptr:
    #        children=ptr['children']

    #        curptr = self.mOutputPtr
    #        curchildren={}
    #        curptr['children'] = curchildren

    #        for key in children:
    #            curchildren[key]={}
    #            self.mOutputPtr = curchildren[key]
    #            self.output(children[key],path + '/' + key)
    
    def done(self):
        global gBlockMap

        if not self.get():
            return

        output_name = "/dev/shm/simperf/%s_proc.%d.llsd" % (self.process_name, os.getpid())
        output_file = open(output_name, 'w')
        process_info = {
            "name"  :   self.process_name,
            "pid"   :   os.getpid(),
            "ppid"  :   os.getppid(),
            "timestamp" :   self.mTimeStamp,
            "utc_time"  :   self.mUTCTime,
            }
        output_file.write(llsd.format_notation(process_info))
        output_file.write('\n')

        for key in gBlockMap.keys():
            gBlockMap[key] = gBlockMap[key].get_map()
        output_file.write(llsd.format_notation(gBlockMap))
        output_file.write('\n')
        output_file.close()

