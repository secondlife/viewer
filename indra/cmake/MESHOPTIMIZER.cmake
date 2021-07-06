# -*- cmake -*-

include(Prebuilt)

use_prebuilt_binary(meshoptimizer)

if (WINDOWS)
  set(MESHOPTIMIZER_LIBRARIES
    debug meshoptimizer
    optimized meshoptimizer)
elseif (LINUX)
  set(MESHOPTIMIZER_LIBRARIES meshoptimizer)
elseif (DARWIN)
  set(MESHOPTIMIZER_LIBRARIES libmeshoptimizer.o)
endif (WINDOWS)
set(MESHOPTIMIZER_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/include/meshoptimizer)
