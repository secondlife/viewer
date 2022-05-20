# -*- cmake -*-

include(Linking)
include(Prebuilt)

use_prebuilt_binary(meshoptimizer)

if (WINDOWS)
  set(MESHOPTIMIZER_LIBRARIES meshoptimizer.lib)
elseif (LINUX)
  set(MESHOPTIMIZER_LIBRARIES meshoptimizer.o)
elseif (DARWIN)
  set(MESHOPTIMIZER_LIBRARIES libmeshoptimizer.a)
endif (WINDOWS)

set(MESHOPTIMIZER_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/include/meshoptimizer)
