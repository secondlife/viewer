# -*- cmake -*-

include(MySQL)

set(LLDATABASE_INCLUDE_DIRS
    ${LIBS_SERVER_DIR}/lldatabase
    ${MYSQL_INCLUDE_DIR}
    )

set(LLDATABASE_LIBRARIES
    lldatabase
    ${MYSQL_LIBRARIES}
    )
