# -*- cmake -*-
include(Prebuilt)
use_prebuilt_binary(libatmosphere)
set(LIBATMOSPHERE_LIBRARIES atmosphere)
set(LIBATMOSPHERE_INCLUDE_DIR ${LIBS_PREBUILT_DIR}/include/atmosphere)
