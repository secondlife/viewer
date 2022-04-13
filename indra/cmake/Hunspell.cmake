# -*- cmake -*-
include(Prebuilt)

include_guard()
create_target( ll::hunspell )

use_prebuilt_binary(libhunspell)
if (WINDOWS)
  set_target_libraries( ll::hunspell libhunspell)
elseif(DARWIN)
  set_target_libraries( ll::hunspell hunspell-1.3)
elseif(LINUX)
  set_target_libraries( ll::hunspell hunspell-1.3)
endif()
set_target_include_dirs( ll::hunspell ${LIBS_PREBUILT_DIR}/include/hunspell)
use_prebuilt_binary(dictionaries)
