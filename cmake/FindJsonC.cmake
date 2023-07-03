include(FindPackageHandleStandardArgs)

find_package(PkgConfig)
if (PKG_CONFIG_FOUND)
    pkg_check_modules(JSON_C IMPORTED_TARGET GLOBAL json-c)
  endif ()
set(JSON_C_LIBRARIES ${JSON_C_LIBRARIES} CACHE STRING "The json-c libraries." FORCE)
add_library(JsonC ALIAS PkgConfig::JSON_C)

find_package_handle_standard_args(JsonC DEFAULT_MSG JSON_C_LIBRARIES)