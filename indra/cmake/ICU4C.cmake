# -*- cmake -*-
include(Prebuilt)

include_guard()

add_library( ll::icu4c INTERFACE IMPORTED )


use_system_binary(icu4c)
use_prebuilt_binary(icu4c)
if (WINDOWS)
  target_link_libraries( ll::icu4c INTERFACE  icuuc)
elseif(DARWIN)
  target_link_libraries( ll::icu4c INTERFACE  icuuc)
elseif(LINUX)
  target_link_libraries( ll::icu4c INTERFACE  icuuc)
else()
  message(FATAL_ERROR "Invalid platform")
endif()

target_include_directories( ll::icu4c SYSTEM INTERFACE  ${LIBS_PREBUILT_DIR}/include/unicode )

use_prebuilt_binary(dictionaries)
