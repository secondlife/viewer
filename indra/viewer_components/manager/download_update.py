#!/usr/bin/env python

# $LicenseInfo:firstyear=2016&license=internal$
# 
# Copyright (c) 2016, Linden Research, Inc.
# 
# The following source code is PROPRIETARY AND CONFIDENTIAL. Use of
# this source code is governed by the Linden Lab Source Code Disclosure
# Agreement ("Agreement") previously entered between you and Linden
# Lab. By accessing, using, copying, modifying or distributing this
# software, you acknowledge that you have been informed of your
# obligations under the Agreement and agree to abide by those obligations.
# 
# ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
# WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
# COMPLETENESS OR PERFORMANCE.
# $LicenseInfo:firstyear=2013&license=viewerlgpl$
# Copyright (c) 2013, Linden Research, Inc.
# $/LicenseInfo$

"""
@file   download_update.py
@author coyot
@date   2016-06-23
"""

"""
Performs a download of an update.  In a separate script from update_manager so that we can
call it with subprocess.
"""

import argparse
import InstallerUserMessage as IUM
import os
import Queue
import requests
import threading


def download_update(url = None, download_dir = None, size = None, progressbar = False, chunk_size = 1024):
    #url to download from
    #download_dir to download to
    #total size (for progressbar) of download
    #progressbar: whether to display one (not used for background downloads)
    #chunk_size is in bytes
    #in_queue is to communicate between download_update and the thread
    #out_queue is to communicate between the thread and the progressbar
    #if progressbar:
        #out_queue = Queue.Queue()
        #buffer_thread = ThreadedBuffer(size, in_queue, out_queue)
        #buffer_thread.start()
        #pb_thread = ThreadedBar(size, out_queue)
        #pb_thread.start()
    
    queue = Queue.Queue()
    filename = os.path.join(download_dir, url.split('/')[-1])
    req = requests.get(url, stream=True)
    down_thread = ThreadedDownload(req, filename, chunk_size, progressbar, queue)
    down_thread.start()
    
    if progressbar:
        frame = IUM.InstallerUserMessage(title = "Second Life Downloader", icon_name="head-sl-logo.gif")
        frame.progress_bar(message = "Download Progress", size = size, pb_queue = queue)
        frame.mainloop()

class ThreadedDownload(threading.Thread):
    def __init__(self, req, filename, chunk_size, progressbar, in_queue):
        threading.Thread.__init__(self)
        self.req = req
        self.filename = filename
        self.chunk_size = int(chunk_size)
        self.progressbar = progressbar
        self.in_queue = in_queue
        
    def run(self):
        with open(self.filename, 'wb') as fd:
            for chunk in self.req.iter_content(self.chunk_size):
                fd.write(chunk)
                if self.progressbar:
                    self.in_queue.put(len(chunk))                       
            self.in_queue.put(-1)

def main():
    parser = argparse.ArgumentParser("Download URI to directory")
    parser.add_argument('--url', dest='url', help='URL of file to be downloaded', required=True)
    parser.add_argument('--dir', dest='download_dir', help='directory to be downloaded to', required=True)
    parser.add_argument('--pb', dest='progressbar', help='whether or not to show a progressbar', action="store_true", default = False)
    parser.add_argument('--size', dest='size', help='size of download for progressbar')
    parser.add_argument('--chunk_size', dest='chunk_size', help='max portion size of download to be loaded in memory in bytes.')
    args = parser.parse_args()

    download_update(url = args.url, download_dir = args.download_dir, size = args.size, progressbar = args.progressbar, chunk_size = args.chunk_size)


if __name__ == "__main__":
    main()
