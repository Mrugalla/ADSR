# Install script for directory: F:/PluginDevelopment/JUCE

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/Program Files (x86)/ADSR")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("F:/PluginDevelopment/ADSR/build-clap/clap_juce_juce/modules/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("F:/PluginDevelopment/ADSR/build-clap/clap_juce_juce/extras/Build/cmake_install.cmake")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/JUCE-7.0.2" TYPE FILE FILES
    "F:/PluginDevelopment/ADSR/build-clap/clap_juce_juce/JUCEConfigVersion.cmake"
    "F:/PluginDevelopment/ADSR/build-clap/clap_juce_juce/JUCEConfig.cmake"
    "F:/PluginDevelopment/JUCE/extras/Build/CMake/JUCECheckAtomic.cmake"
    "F:/PluginDevelopment/JUCE/extras/Build/CMake/JUCEHelperTargets.cmake"
    "F:/PluginDevelopment/JUCE/extras/Build/CMake/JUCEModuleSupport.cmake"
    "F:/PluginDevelopment/JUCE/extras/Build/CMake/JUCEUtils.cmake"
    "F:/PluginDevelopment/JUCE/extras/Build/CMake/JuceLV2Defines.h.in"
    "F:/PluginDevelopment/JUCE/extras/Build/CMake/LaunchScreen.storyboard"
    "F:/PluginDevelopment/JUCE/extras/Build/CMake/PIPAudioProcessor.cpp.in"
    "F:/PluginDevelopment/JUCE/extras/Build/CMake/PIPAudioProcessorWithARA.cpp.in"
    "F:/PluginDevelopment/JUCE/extras/Build/CMake/PIPComponent.cpp.in"
    "F:/PluginDevelopment/JUCE/extras/Build/CMake/PIPConsole.cpp.in"
    "F:/PluginDevelopment/JUCE/extras/Build/CMake/RecentFilesMenuTemplate.nib"
    "F:/PluginDevelopment/JUCE/extras/Build/CMake/UnityPluginGUIScript.cs.in"
    "F:/PluginDevelopment/JUCE/extras/Build/CMake/checkBundleSigning.cmake"
    "F:/PluginDevelopment/JUCE/extras/Build/CMake/copyDir.cmake"
    "F:/PluginDevelopment/JUCE/extras/Build/CMake/juce_runtime_arch_detection.cpp"
    )
endif()

