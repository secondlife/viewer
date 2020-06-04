# -*- cmake -*-
include(Prebuilt)

use_prebuilt_binary(llca)

set(CA_BUNDLE_MAX_AGE_DAYS 90)

execute_process(
    COMMAND bash bin/check-ca-bundle-age.sh --packages . --days ${CA_BUNDLE_MAX_AGE_DAYS}
    WORKING_DIRECTORY ${AUTOBUILD_INSTALL_DIR}
    RESULT_VARIABLE ca_bundle_check_result
    )

if (NOT ${ca_bundle_check_result} EQUAL 0)
  message(SEND_ERROR "Certificate Authority Bundle is out of date (rebuild and update the llca package)")
else (NOT ${ca_bundle_check_result} EQUAL 0)
  message(STATUS "Certificate Authority Bundle is recent enough.")
endif (NOT ${ca_bundle_check_result} EQUAL 0)


