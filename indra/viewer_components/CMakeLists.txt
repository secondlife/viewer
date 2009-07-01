# -*- cmake -*-

include(Externals)

add_subdirectory(login)
add_subdirectory(eventhost)

use_svn_external(eventlet ${CMAKE_CURRENT_SOURCE_DIR}/../lib/python/ http://svn.secondlife.com/svn/eventlet/trunk/eventlet 164)

