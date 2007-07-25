"""
@file webdav.py
@brief Classes to make manipulation of a webdav store easier.

Copyright (c) 2006-2007, Linden Research, Inc.
$License$
"""

import sys, os, httplib, urlparse
import socket, time
import xml.dom.minidom
import syslog
# import signal

__revision__ = '0'

dav_debug = False


# def urlsafe_b64decode (enc):
#     return base64.decodestring (enc.replace ('_', '/').replace ('-', '+'))

# def urlsafe_b64encode (str):
#     return base64.encodestring (str).replace ('+', '-').replace ('/', '_')


class DAVError (Exception):
    """ Base class for exceptions in this module. """
    def __init__ (self, status=0, message='', body='', details=''):
        self.status = status
        self.message = message
        self.body = body
        self.details = details
        Exception.__init__ (self, '%d:%s:%s%s' % (self.status, self.message,
                                                   self.body, self.details))

    def print_to_stderr (self):
        """ print_to_stderr docstring """
        print >> sys.stderr, str (self.status) + ' ' + self.message
        print >> sys.stderr, str (self.details)


class Timeout (Exception):
    """ Timeout docstring """
    def __init__ (self, arg=''):
        Exception.__init__ (self, arg)


def alarm_handler (signum, frame):
    """ alarm_handler docstring """
    raise Timeout ('caught alarm')


class WebDAV:
    """ WebDAV docstring """
    def __init__ (self, url, proxy=None, retries_before_fail=6):
        self.init_url = url
        self.init_proxy = proxy
        self.retries_before_fail = retries_before_fail
        url_parsed = urlparse.urlsplit (url)

        self.top_path = url_parsed[ 2 ]
        # make sure top_path has a trailing /
        if self.top_path == None or self.top_path == '':
            self.top_path = '/'
        elif len (self.top_path) > 1 and self.top_path[-1:] != '/':
            self.top_path += '/'

        if dav_debug:
            syslog.syslog ('new WebDAV %s : %s' % (str (url), str (proxy)))

        if proxy:
            proxy_parsed = urlparse.urlsplit (proxy)
            self.host_header = url_parsed[ 1 ]
            host_and_port = proxy_parsed[ 1 ].split (':')
            self.host = host_and_port[ 0 ]
            if len (host_and_port) > 1:
                self.port = int(host_and_port[ 1 ])
            else:
                self.port = 80
        else: # no proxy
            host_and_port = url_parsed[ 1 ].split (':')
            self.host_header = None
            self.host = host_and_port[ 0 ]
            if len (host_and_port) > 1:
                self.port = int(host_and_port[ 1 ])
            else:
                self.port = 80

        self.connection = False
        self.connect ()


    def log (self, msg, depth=0):
        """ log docstring """
        if dav_debug and depth == 0:
            host = str (self.init_url)
            if host == 'http://int.tuco.lindenlab.com:80/asset/':
                host = 'tuco'
            if host == 'http://harriet.lindenlab.com/asset-keep/':
                host = 'harriet/asset-keep'
            if host == 'http://harriet.lindenlab.com/asset-flag/':
                host = 'harriet/asset-flag'
            if host == 'http://harriet.lindenlab.com/asset/':
                host = 'harriet/asset'
            if host == 'http://ozzy.lindenlab.com/asset/':
                host = 'ozzy/asset'
            if host == 'http://station11.lindenlab.com:12041/:':
                host = 'station11:12041'
            proxy = str (self.init_proxy)
            if proxy == 'None':
                proxy = ''
            if proxy == 'http://int.tuco.lindenlab.com:3128/':
                proxy = 'tuco'
            syslog.syslog ('WebDAV (%s:%s) %s' % (host, proxy, str (msg)))


    def connect (self):
        """ connect docstring """
        self.log ('connect')
        self.connection = httplib.HTTPConnection (self.host, self.port)

    def __err (self, response, details):
        """ __err docstring """
        raise DAVError (response.status, response.reason, response.read (),
                        str (self.init_url) + ':' + \
                        str (self.init_proxy) + ':' + str (details))

    def request (self, method, path, body=None, headers=None,
                 read_all=True, body_hook = None, recurse=0, allow_cache=True):
        """ request docstring """
        # self.log ('request %s %s' % (method, path))
        if headers == None:
            headers = {}
        if not allow_cache:
            headers['Pragma'] = 'no-cache'
            headers['cache-control'] = 'no-cache'
        try:
            if method.lower () != 'purge':
                if path.startswith ('/'):
                    path = path[1:]
                if self.host_header: # use proxy
                    headers[ 'host' ] = self.host_header
                    fullpath = 'http://%s%s%s' % (self.host_header,
                                                  self.top_path, path)
                else: # no proxy
                    fullpath = self.top_path + path
            else:
                fullpath = path

            self.connection.request (method, fullpath, body, headers)
            if body_hook:
                body_hook ()

            # signal.signal (signal.SIGALRM, alarm_handler)
            # try:
            #     signal.alarm (120)
            #     signal.alarm (0)
            # except Timeout, e:
            #     if recurse < 6:
            #         return self.retry_request (method, path, body, headers,
            #                                    read_all, body_hook, recurse)
            #     else:
            #         raise DAVError (0, 'timeout', self.host,
            #                         (method, path, body, headers, recurse))

            response = self.connection.getresponse ()

            if read_all:
                while len (response.read (1024)) > 0:
                    pass
            if (response.status == 500 or \
                response.status == 503 or \
                response.status == 403) and \
                recurse < self.retries_before_fail:
                return self.retry_request (method, path, body, headers,
                                           read_all, body_hook, recurse)
            return response
        except (httplib.ResponseNotReady,
                httplib.BadStatusLine,
                socket.error):
            # if the server hangs up on us (keepalive off, broken pipe),
            # we need to reconnect and try again.
            if recurse < self.retries_before_fail:
                return self.retry_request (method, path, body, headers,
                                           read_all, body_hook, recurse)
            raise DAVError (0, 'reconnect failed', self.host,
                            (method, path, body, headers, recurse))


    def retry_request (self, method, path, body, headers,
                       read_all, body_hook, recurse):
        """ retry_request docstring """
        time.sleep (10.0 * recurse)
        self.connect ()
        return self.request (method, path, body, headers,
                             read_all, body_hook, recurse+1)



    def propfind (self, path, body=None, depth=1):
        """ propfind docstring """
        # self.log ('propfind %s' % path)
        headers = {'Content-Type':'text/xml; charset="utf-8"',
                   'Depth':str(depth)}
        response = self.request ('PROPFIND', path, body, headers, False)
        if response.status == 207:
            return response # Multi-Status
        self.__err (response, ('PROPFIND', path, body, headers, 0))


    def purge (self, path):
        """ issue a squid purge command """
        headers = {'Accept':'*/*'}
        response = self.request ('PURGE', path, None, headers)
        if response.status == 200 or response.status == 404:
            # 200 if it was purge, 404 if it wasn't there.
            return response
        self.__err (response, ('PURGE', path, None, headers))


    def get_file_size (self, path):
        """
        Use propfind to ask a webdav server what the size of
        a file is.  If used on a directory (collection) return 0
        """
        self.log ('get_file_size %s' % path)
        # "getcontentlength" property
        # 8.1.1 Example - Retrieving Named Properties
        # http://docs.python.org/lib/module-xml.dom.html
        nsurl = 'http://apache.org/dav/props/'
        doc = xml.dom.minidom.Document ()
        propfind_element = doc.createElementNS (nsurl, "D:propfind")
        propfind_element.setAttributeNS (nsurl, 'xmlns:D', 'DAV:')
        doc.appendChild (propfind_element)
        prop_element = doc.createElementNS (nsurl, "D:prop")
        propfind_element.appendChild (prop_element)
        con_len_element = doc.createElementNS (nsurl, "D:getcontentlength")
        prop_element.appendChild (con_len_element)

        response = self.propfind (path, doc.toxml ())
        doc.unlink ()

        resp_doc = xml.dom.minidom.parseString (response.read ())
        cln = resp_doc.getElementsByTagNameNS ('DAV:','getcontentlength')[ 0 ]
        try:
            content_length = int (cln.childNodes[ 0 ].nodeValue)
        except IndexError:
            return 0
        resp_doc.unlink ()
        return content_length


    def file_exists (self, path):
        """
        do an http head on the given file.  return True if it succeeds
        """
        self.log ('file_exists %s' % path)
        expect_gzip = path.endswith ('.gz')
        response = self.request ('HEAD', path)
        got_gzip = response.getheader ('Content-Encoding', '').strip ()
        if got_gzip.lower () == 'x-gzip' and expect_gzip == False:
            # the asset server fakes us out if we ask for the non-gzipped
            # version of an asset, but the server has the gzipped version.
            return False
        return response.status == 200


    def mkdir (self, path):
        """ mkdir docstring """
        self.log ('mkdir %s' % path)
        headers = {}
        response = self.request ('MKCOL', path, None, headers)
        if response.status == 201:
            return # success
        if response.status == 405:
            return # directory already existed?
        self.__err (response, ('MKCOL', path, None, headers, 0))


    def delete (self, path):
        """ delete docstring """
        self.log ('delete %s' % path)
        headers = {'Depth':'infinity'} # collections require infinity
        response = self.request ('DELETE', path, None, headers)
        if response.status == 204:
            return # no content
        if response.status == 404:
            return # hmm
        self.__err (response, ('DELETE', path, None, headers, 0))


    def list_directory (self, path, dir_filter=None, allow_cache=True,
                        minimum_cache_time=False):
        """
        Request an http directory listing and parse the filenames out of lines
        like: '<LI><A HREF="X"> X</A>'. If a filter function is provided,
        only return filenames that the filter returns True for.

        This is sort of grody, but it seems faster than other ways of getting
        this information from an isilon.
        """
        self.log ('list_directory %s' % path)

        def try_match (lline, before, after):
            """ try_match docstring """
            try:
                blen = len (before)
                asset_start_index = lline.index (before)
                asset_end_index = lline.index (after, asset_start_index + blen)
                asset = line[ asset_start_index + blen : asset_end_index ]

                if not dir_filter or dir_filter (asset):
                    return [ asset ]
                return []
            except ValueError:
                return []

        if len (path) > 0 and path[-1:] != '/':
            path += '/'

        response = self.request ('GET', path, None, {}, False,
                                 allow_cache=allow_cache)

        if allow_cache and minimum_cache_time: # XXX
            print response.getheader ('Date')
            # s = "2005-12-06T12:13:14"
            # from datetime import datetime
            # from time import strptime
            # datetime(*strptime(s, "%Y-%m-%dT%H:%M:%S")[0:6])
            # datetime.datetime(2005, 12, 6, 12, 13, 14)

        if response.status != 200:
            self.__err (response, ('GET', path, None, {}, 0))
        assets = []
        for line in response.read ().split ('\n'):
            lline = line.lower ()
            if lline.find ("parent directory") == -1:
                # isilon file
                assets += try_match (lline, '<li><a href="', '"> ')
                # apache dir
                assets += try_match (lline, 'alt="[dir]"> <a href="', '/">')
                # apache file
                assets += try_match (lline, 'alt="[   ]"> <a href="', '">')
        return assets


    def __tmp_filename (self, path_and_file):
        """ __tmp_filename docstring """
        head, tail = os.path.split (path_and_file)
        if head != '':
            return head + '/.' + tail + '.' + str (os.getpid ())
        else:
            return head + '.' + tail + '.' + str (os.getpid ())


    def __put__ (self, filesize, body_hook, remotefile):
        """ __put__ docstring """
        headers = {'Content-Length' : str (filesize)}
        remotefile_tmp = self.__tmp_filename (remotefile)
        response = self.request ('PUT', remotefile_tmp, None,
                                 headers, True, body_hook)
        if not response.status in (201, 204): # created, no content
            self.__err (response, ('PUT', remotefile, None, headers, 0))
        if filesize != self.get_file_size (remotefile_tmp):
            try:
                self.delete (remotefile_tmp)
            except:
                pass
            raise DAVError (0, 'tmp upload error', remotefile_tmp)
        # move the file to its final location
        try:
            self.rename (remotefile_tmp, remotefile)
        except DAVError, exc:
            if exc.status == 403: # try to clean up the tmp file
                try:
                    self.delete (remotefile_tmp)
                except:
                    pass
            raise
        if filesize != self.get_file_size (remotefile):
            raise DAVError (0, 'file upload error', str (remotefile_tmp))


    def put_string (self, strng, remotefile):
        """ put_string docstring """
        self.log ('put_string %d -> %s' % (len (strng), remotefile))
        filesize = len (strng)
        def body_hook ():
            """ body_hook docstring """
            self.connection.send (strng)
        self.__put__ (filesize, body_hook, remotefile)


    def put_file (self, localfile, remotefile):
        """
        Send a local file to a remote webdav store.  First, upload to
        a temporary filename.  Next make sure the file is the size we
        expected.  Next, move the file to its final location.  Next,
        check the file size at the final location.
        """
        self.log ('put_file %s -> %s' % (localfile, remotefile))
        filesize = os.path.getsize (localfile)
        def body_hook ():
            """ body_hook docstring """
            handle = open (localfile)
            while True:
                data = handle.read (1300)
                if len (data) == 0:
                    break
                self.connection.send (data)
            handle.close ()
        self.__put__ (filesize, body_hook, remotefile)


    def create_empty_file (self, remotefile):
        """ create an empty file """
        self.log ('touch_file %s' % (remotefile))
        headers = {'Content-Length' : '0'}
        response = self.request ('PUT', remotefile, None, headers)
        if not response.status in (201, 204): # created, no content
            self.__err (response, ('PUT', remotefile, None, headers, 0))
        if self.get_file_size (remotefile) != 0:
            raise DAVError (0, 'file upload error', str (remotefile))


    def __get_file_setup (self, remotefile, check_size=True):
        """ __get_file_setup docstring """
        if check_size:
            remotesize = self.get_file_size (remotefile)
        response = self.request ('GET', remotefile, None, {}, False)
        if response.status != 200:
            self.__err (response, ('GET', remotefile, None, {}, 0))
        try:
            content_length = int (response.getheader ("Content-Length"))
        except TypeError:
            content_length = None
        if check_size:
            if content_length != remotesize:
                raise DAVError (0, 'file DL size error', remotefile)
        return (response, content_length)


    def __get_file_read (self, writehandle, response, content_length):
        """ __get_file_read docstring """
        if content_length != None:
            so_far_length = 0
            while so_far_length < content_length:
                data = response.read (content_length - so_far_length)
                if len (data) == 0:
                    raise DAVError (0, 'short file download')
                so_far_length += len (data)
                writehandle.write (data)
            while len (response.read ()) > 0:
                pass
        else:
            while True:
                data = response.read ()
                if (len (data) < 1):
                    break
                writehandle.write (data)


    def get_file (self, remotefile, localfile, check_size=True):
        """
        Get a remote file from a webdav server.  Download to a local
        tmp file, then move into place.  Sanity check file sizes as
        we go.
        """
        self.log ('get_file %s -> %s' % (remotefile, localfile))
        (response, content_length) = \
                   self.__get_file_setup (remotefile, check_size)
        localfile_tmp = self.__tmp_filename (localfile)
        handle = open (localfile_tmp, 'w')
        self.__get_file_read (handle, response, content_length)
        handle.close ()
        if check_size:
            if content_length != os.path.getsize (localfile_tmp):
                raise DAVError (0, 'file DL size error',
                                remotefile+','+localfile)
        os.rename (localfile_tmp, localfile)


    def get_file_as_string (self, remotefile, check_size=True):
        """
        download a file from a webdav server and return it as a string.
        """
        self.log ('get_file_as_string %s' % remotefile)
        (response, content_length) = \
                   self.__get_file_setup (remotefile, check_size)
        # (tmp_handle, tmp_filename) = tempfile.mkstemp ()
        tmp_handle = os.tmpfile ()
        self.__get_file_read (tmp_handle, response, content_length)
        tmp_handle.seek (0)
        ret = tmp_handle.read ()
        tmp_handle.close ()
        # os.unlink (tmp_filename)
        return ret


    def get_post_as_string (self, remotefile, body):
        """
        Do an http POST, send body, get response and return it.
        """
        self.log ('get_post_as_string %s' % remotefile)
        # headers = {'Content-Type':'application/x-www-form-urlencoded'}
        headers = {'Content-Type':'text/xml; charset="utf-8"'}
        # b64body = urlsafe_b64encode (asset_url)
        response = self.request ('POST', remotefile, body, headers, False)
        if response.status != 200:
            self.__err (response, ('POST', remotefile, body, headers, 0))
        try:
            content_length = int (response.getheader ('Content-Length'))
        except TypeError:
            content_length = None
        tmp_handle = os.tmpfile ()
        self.__get_file_read (tmp_handle, response, content_length)
        tmp_handle.seek (0)
        ret = tmp_handle.read ()
        tmp_handle.close ()
        return ret


    def __destination_command (self, verb, remotesrc, dstdav, remotedst):
        """
        self and dstdav should point to the same http server.
        """
        if len (remotedst) > 0 and remotedst[ 0 ] == '/':
            remotedst = remotedst[1:]
        headers = {'Destination': 'http://%s:%d%s%s' % (dstdav.host,
                                                        dstdav.port,
                                                        dstdav.top_path,
                                                        remotedst)}
        response = self.request (verb, remotesrc, None, headers)
        if response.status == 201:
            return # created
        if response.status == 204:
            return # no content
        self.__err (response, (verb, remotesrc, None, headers, 0))


    def rename (self, remotesrc, remotedst):
        """ rename a file on a webdav server """
        self.log ('rename %s -> %s' % (remotesrc, remotedst))
        self.__destination_command ('MOVE', remotesrc, self, remotedst)
    def xrename (self, remotesrc, dstdav, remotedst):
        """ rename a file on a webdav server """
        self.log ('xrename %s -> %s' % (remotesrc, remotedst))
        self.__destination_command ('MOVE', remotesrc, dstdav, remotedst)


    def copy (self, remotesrc, remotedst):
        """ copy a file on a webdav server """
        self.log ('copy %s -> %s' % (remotesrc, remotedst))
        self.__destination_command ('COPY', remotesrc, self, remotedst)
    def xcopy (self, remotesrc, dstdav, remotedst):
        """ copy a file on a webdav server """
        self.log ('xcopy %s -> %s' % (remotesrc, remotedst))
        self.__destination_command ('COPY', remotesrc, dstdav, remotedst)


def put_string (data, url):
    """
    upload string s to a url
    """
    url_parsed = urlparse.urlsplit (url)
    dav = WebDAV ('%s://%s/' % (url_parsed[ 0 ], url_parsed[ 1 ]))
    dav.put_string (data, url_parsed[ 2 ])


def get_string (url, check_size=True):
    """
    return the contents of a url as a string
    """
    url_parsed = urlparse.urlsplit (url)
    dav = WebDAV ('%s://%s/' % (url_parsed[ 0 ], url_parsed[ 1 ]))
    return dav.get_file_as_string (url_parsed[ 2 ], check_size)
