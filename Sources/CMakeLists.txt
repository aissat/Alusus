# This file is released under Alusus Public License, Version 1.0.
# For details on usage and copying conditions read the full license in the
# accompanying license file or at <https://alusus.org/license.html>.

project(Alusus)
cmake_minimum_required(VERSION 3.1)

# Read version info from version file.
execute_process(COMMAND python3 version_info.py
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../Tools/BuildSrc
                OUTPUT_VARIABLE VersionInfo)
string(REGEX MATCH "VERSION: ([0-9]+\\.[0-9]+\\.[0-9]+)" _dummy_ ${VersionInfo})
set(AlususVersion ${CMAKE_MATCH_1})
string(REGEX MATCH "^[0-9]+\\.[0-9]+" AlususSoVersion ${AlususVersion})
string(REGEX MATCH "REVISION: ([A-Za-z0-9]+)" _dummy_ ${VersionInfo})
set(AlususRevision ${CMAKE_MATCH_1})
if (NOT ${AlususRevision} STREQUAL "")
  set(AlususRevision "-${AlususRevision}")
endif()
string(REGEX MATCH "DATE: ([0-9]+-[0-9]+-[0-9]+)" _dummy_ ${VersionInfo})
set(AlususReleaseDate ${CMAKE_MATCH_1})
string(REGEX MATCH "HIJRI_DATE: ([0-9]+-[0-9]+-[0-9]+)" _dummy_ ${VersionInfo})
set(AlususHijriReleaseDate ${CMAKE_MATCH_1})
message(STATUS "Alusus Version: " ${AlususVersion})
message(STATUS "Alusus Revision: " ${AlususRevision})
message(STATUS "Alusus So Version: " ${AlususSoVersion})
message(STATUS "Alusus Release Date: " ${AlususReleaseDate})
message(STATUS "Alusus Hijri Release Date: " ${AlususHijriReleaseDate})

set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS ALUSUS_VERSION="${AlususVersion}"
                                                           ALUSUS_REVISION="${AlususRevision}"
                                                           ALUSUS_RELEASE_DATE="${AlususReleaseDate}"
                                                           ALUSUS_HIJRI_RELEASE_DATE="${AlususHijriReleaseDate}")
if ("${CMAKE_BUILD_TYPE_UPPER}" STREQUAL "RELEASE")
  set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS RELEASE)
else()
  set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS USE_LOGS)
endif()

# LLVM path
set(LLVM_PATH "" CACHE PATH "Path to LLVM root folder.")
if (NOT EXISTS "${LLVM_PATH}/include/llvm")
  message(SEND_ERROR "Please specify the path to LLVM root folder in LLVM_PATH.")
else()
  message(STATUS "The following LLVM path was provided: " ${LLVM_PATH} ".")
endif()

# Prepare compile and link flags.
STRING(TOUPPER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE_UPPER)
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  message(STATUS "Using Clang")
  # CLang options
  set(Alusus_COMPILE_FLAGS "${CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE_UPPER}} -Werror=return-type -Wunused-variable -Winit-self -Wuninitialized -Wunused-const-variable -Wno-unused-result -fno-rtti")
  set(Alusus_COMPILE_FLAGS "${Alusus_COMPILE_FLAGS} -std=c++17")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  # GCC options
  message(STATUS "Using GCC")
  set(Alusus_COMPILE_FLAGS "${CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE_UPPER}} -Werror=return-type -Wunused-variable -Winit-self -Wuninitialized -Wunused-but-set-parameter -Wunused-but-set-variable -Wno-unused-result -fno-rtti")
  set(Alusus_COMPILE_FLAGS "${Alusus_COMPILE_FLAGS} -std=c++17")
endif()

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  # macOS has a different mechanism for RPATH than Linux and requires special
  # handling. More information can be found here:
  #
  # https://gitlab.kitware.com/cmake/community/-/wikis/doc/cmake/RPATH-handling
  #
  # The following article also has a good explanation:
  #
  # https://blogs.oracle.com/dipol/dynamic-libraries,-rpath,-and-mac-os
  set(CMAKE_MACOSX_RPATH TRUE)
  set(CMAKE_EXE_LINKER_FLAGS "-Wl,-rpath,@executable_path")
else()
  set(CMAKE_SKIP_RPATH TRUE)
  set(CMAKE_EXE_LINKER_FLAGS "-Wl,-rpath,'$ORIGIN',--enable-new-dtags")
endif()

# Clear CMAKE_CXX_FLAGS_* because we are going to provide our own compilation
# flags here.
set(CMAKE_CXX_FLAGS_RELEASE "")
set(CMAKE_CXX_FLAGS_DEBUG "")
set(CMAKE_CXX_FLAGS "")

# Incorporate the CMake features provided by LLVM:
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" "${LLVM_PATH}/lib/cmake/llvm")
include(LLVMConfig)

# Macro for generating pre-compiled headers.
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" "${Alusus_SOURCE_DIR}")
include(precompiled_headers.cmake)

# Enable testing
enable_testing()

set (CUSTOM_OUTPUT_PATH "" CACHE PATH "Custom path to output all executable and library files. Useful during development.")
if (NOT CUSTOM_OUTPUT_PATH STREQUAL "")
  if (NOT EXISTS "${CUSTOM_OUTPUT_PATH}")
    message(SEND_ERROR "Invalid custom output path was provided: " ${CUSTOM_OUTPUT_PATH} ".")
  else()
    message(STATUS "The following custom output path was provided: " ${CUSTOM_OUTPUT_PATH} ".")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CUSTOM_OUTPUT_PATH}")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CUSTOM_OUTPUT_PATH}")
  endif()
endif()

# Add projects
add_subdirectory(Srt)
add_subdirectory(Core)
add_subdirectory(Spp)
add_subdirectory(Tests)
