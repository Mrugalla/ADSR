#----------------------------------------------------------------
# Generated CMake target import file for configuration "MinSizeRel".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "juce::juce_lv2_helper" for configuration "MinSizeRel"
set_property(TARGET juce::juce_lv2_helper APPEND PROPERTY IMPORTED_CONFIGURATIONS MINSIZEREL)
set_target_properties(juce::juce_lv2_helper PROPERTIES
  IMPORTED_LOCATION_MINSIZEREL "${_IMPORT_PREFIX}/bin/JUCE-7.0.2/juce_lv2_helper.exe"
  )

list(APPEND _cmake_import_check_targets juce::juce_lv2_helper )
list(APPEND _cmake_import_check_files_for_juce::juce_lv2_helper "${_IMPORT_PREFIX}/bin/JUCE-7.0.2/juce_lv2_helper.exe" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
