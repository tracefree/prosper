#----------------------------------------------------------------
# Generated CMake target import file.
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "fastgltf::fastgltf" for configuration ""
set_property(TARGET fastgltf::fastgltf APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(fastgltf::fastgltf PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_NOCONFIG "CXX"
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib/libfastgltf.a"
  )

list(APPEND _cmake_import_check_targets fastgltf::fastgltf )
list(APPEND _cmake_import_check_files_for_fastgltf::fastgltf "${_IMPORT_PREFIX}/lib/libfastgltf.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
