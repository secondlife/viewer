# -*- cmake -*-

include(CARes)
include(CURL)
include(OpenSSL)
include(XmlRpcEpi)

set(LLMESSAGE_INCLUDE_DIRS
    ${LIBS_OPEN_DIR}/llmessage
    ${CARES_INCLUDE_DIRS}
    ${CURL_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIRS}
    )

set(LLMESSAGE_LIBRARIES llmessage)
