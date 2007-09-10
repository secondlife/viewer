"""\
@file named_query.py
@author Ryan Williams, Phoenix
@date 2007-07-31
@brief An API for running named queries.

Copyright (c) 2007, Linden Research, Inc.
$License$
"""

import MySQLdb
import os
import os.path
import time

from indra.base import llsd
from indra.base import config
from indra.ipc import russ

_g_named_manager = None

# this function is entirely intended for testing purposes,
# because it's tricky to control the config from inside a test
def _init_g_named_manager(sql_dir = None):
    if sql_dir is None:
        sql_dir = config.get('named-query-base-dir')
    global _g_named_manager
    _g_named_manager = NamedQueryManager(
        os.path.abspath(os.path.realpath(sql_dir)))

def get(name):
    "@brief get the named query object to be used to perform queries"
    if _g_named_manager is None:
        _init_g_named_manager()
    return _g_named_manager.get(name)

def sql(name, params):
    # use module-global NamedQuery object to perform default substitution
    return get(name).sql(params)

def run(connection, name, params, expect_rows = None):
    """\
@brief given a connection, run a named query with the params

Note that this function will fetch ALL rows.
@param connection The connection to use
@param name The name of the query to run
@param params The parameters passed into the query
@param expect_rows The number of rows expected. Set to 1 if return_as_map is true.  Raises ExpectationFailed if the number of returned rows doesn't exactly match.  Kind of a hack.
@return Returns the result set as a list of dicts.
"""
    return get(name).run(connection, params, expect_rows)

class ExpectationFailed(Exception):
    def __init__(self, message):
        self.message = message

class NamedQuery(object):
    def __init__(self, filename):
        self._stat_interval = 5000  # 5 seconds
        self._location = filename
        self.load_contents()

    def get_modtime(self):
        return os.path.getmtime(self._location)

    def load_contents(self):
        self._contents = llsd.parse(open(self._location).read())
        self._ttl = int(self._contents.get('ttl', 0))
        self._return_as_map = bool(self._contents.get('return_as_map', False))
        self._legacy_dbname = self._contents.get('legacy_dbname', None)
        self._legacy_query = self._contents.get('legacy_query', None)
        self._options = self._contents.get('options', {})
        self._base_query = self._contents['base_query']

        self._last_mod_time = self.get_modtime()
        self._last_check_time = time.time()

    def ttl(self):
        return self._ttl

    def legacy_dbname(self):
        return self._legacy_dbname

    def legacy_query(self):
        return self._legacy_query

    def return_as_map(self):
        return self._return_as_map

    def run(self, connection, params, expect_rows = None, use_dictcursor = True):
        """\
@brief given a connection, run a named query with the params

Note that this function will fetch ALL rows. We do this because it
opens and closes the cursor to generate the values, and this isn't a generator so the
cursor has no life beyond the method call.
@param cursor The connection to use (this generates its own cursor for the query)
@param name The name of the query to run
@param params The parameters passed into the query
@param expect_rows The number of rows expected. Set to 1 if return_as_map is true.  Raises ExpectationFailed if the number of returned rows doesn't exactly match.  Kind of a hack.
@param use_dictcursor Set to false to use a normal cursor and manually convert the rows to dicts.
@return Returns the result set as a list of dicts, or, if the named query has return_as_map set to true, returns a single dict.
        """
        if use_dictcursor:
            cursor = connection.cursor(MySQLdb.cursors.DictCursor)
        else:
            cursor = connection.cursor()
        
        statement = self.sql(params)
        #print "SQL:", statement
        rows = cursor.execute(statement)
        
        # *NOTE: the expect_rows argument is a very cheesy way to get some
        # validation on the result set.  If you want to add more expectation
        # logic, do something more object-oriented and flexible.  Or use an ORM.
        if(self._return_as_map):
            expect_rows = 1
        if expect_rows is not None and rows != expect_rows:
            cursor.close()
            raise ExpectationFailed("Statement expected %s rows, got %s.  Sql: %s" % (
                expect_rows, rows, statement))

        # convert to dicts manually if we're not using a dictcursor
        if use_dictcursor:
            result_set = cursor.fetchall()
        else:
            if cursor.description is None:
                # an insert or something
                x = cursor.fetchall()
                cursor.close()
                return x

            names = [x[0] for x in cursor.description]

            result_set = []
            for row in cursor.fetchall():
                converted_row = {}
                for idx, col_name in enumerate(names):
                    converted_row[col_name] = row[idx]
                result_set.append(converted_row)

        cursor.close()
        if self._return_as_map:
            return result_set[0]
        return result_set

    def sql(self, params):
        self.refresh()

        # build the query from the options available and the params
        base_query = []
        base_query.append(self._base_query)
        for opt, extra_where in self._options.items():
            if opt in params and (params[opt] == 0 or params[opt]):
                if type(extra_where) in (dict, list, tuple):
                    base_query.append(extra_where[params[opt]])
                else:
                    base_query.append(extra_where)

        full_query = '\n'.join(base_query)
        
        # do substitution
        sql = russ.format(full_query, params)
        return sql

    def refresh(self):
        # only stat the file every so often
        now = time.time()
        if(now - self._last_check_time > self._stat_interval):
            self._last_check_time = now
            modtime = self.get_modtime()
            if(modtime > self._last_mod_time):
                self.load_contents()

class NamedQueryManager(object):
    def __init__(self, named_queries_dir):
        self._dir = os.path.abspath(os.path.realpath(named_queries_dir))
        self._cached_queries = {}

    def sql(self, name, params):
        nq = self.get(name)
        return nq.sql(params)
        
    def get(self, name):
        # new up/refresh a NamedQuery based on the name
        nq = self._cached_queries.get(name)
        if nq is None:
            nq = NamedQuery(os.path.join(self._dir, name))
            self._cached_queries[name] = nq
        return nq
