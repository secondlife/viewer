# -*- cmake -*-
include(Prebuilt)

include_guard()
create_target( ll::hunspell )

use_prebuilt_binary(libhunspell)
if (WINDOWS)
  target_link_libraries( ll::hunspell INTERFACE libhunspell)
elseif(DARWIN)
  target_link_libraries( ll::hunspell INTERFACE hunspell-1.3)
elseif(LINUX)
  target_link_libraries( ll::hunspell INTERFACE hunspell-1.3)
endif()
set_target_include_dirs( ll::hunspell ${LIBS_PREBUILT_DIR}/include/hunspell)
use_prebuilt_binary(dictionaries)
