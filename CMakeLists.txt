# CacusLib CMake definitions
cmake_minimum_required (VERSION 3.12)
project ("CacusLib")

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(CheckCCompilerFlag)

# Same build rules as UT99
if(WIN32 AND NOT WINDOWS)
  set(WINDOWS TRUE)
  set(PLATFORM_PREFIX "Win")
endif()
if(APPLE AND NOT MACOSX)
  set(MACOSX TRUE)
  set(PLATFORM_PREFIX "Mac")
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(LINUX TRUE)
  set(PLATFORM_PREFIX "Linux")
endif()


# See what CPU we appear to be targeting... 
macro(check_defs _CDEFS _VALUE)
  check_c_source_compiles(
    "
    #if ( ${_CDEFS} )
      int main() { return 0; }
    #else
      #error
    #endif
    "
  ${_VALUE}
  )

  if(${_VALUE})
    message(STATUS "Check success for ${_VALUE}")
  endif()
endmacro(check_defs)

# CPU Target detection
check_defs( 
  "defined(__i386__) || defined(__i686__) || defined(_M_IX86) || defined(i386)"
  CACUSLIB_X86
)
check_defs(
  "defined(__x86_64__) || defined(_M_X64)"
  CACUSLIB_AMD64
)
check_defs(
  "defined(__arm__)"
  CACUSLIB_ARM
)
check_defs(
  "defined(__arm64__)"
  CACUSLIB_ARM64
)
#TODO: NEON SUPPORT CHECK

if(CACUSLIB_AMD64)
  set(PLATFORM_PREFIX "${PLATFORM_PREFIX}64")
elseif(CACUSLIB_X86)
  set(PLATFORM_PREFIX "${PLATFORM_PREFIX}32")
elseif(CACUSLIB_ARM64)
  set(PLATFORM_PREFIX "${PLATFORM_PREFIX}Arm64")
elseif(CACUSLIB_ARM)
  set(PLATFORM_PREFIX "${PLATFORM_PREFIX}Arm")
else()
  message(FATAL_ERROR "Unrecognized platform")
endif()
message(STATUS "Selected output: ${PLATFORM_PREFIX}")


# Build options
option(CACUSLIB_BUILD_XCGE, "Set to true to build for XC_Engine." FALSE)
option(CACUSLIB_STATIC_BUILD, "Set to true to create static library." FALSE)

if(CACUSLIB_STATIC_BUILD)
  set(CACUSLIB_LINKAGE STATIC)
else()
  set(CACUSLIB_LINKAGE SHARED)
endif()



if(WINDOWS)
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS=1)
elseif(LINUX)
  add_definitions(-fPIC)
  add_definitions(-lpthread)
  add_definitions(-fvisibility=hidden)
  add_compile_definitions(_UNIX=1)
endif()

if(CACUSLIB_BUILD_XCGE)
  add_compile_definitions(CACUSLIB_DISABLE_FIELD=1)
  add_compile_definitions(CACUSLIB_DISABLE_OUTPUTDEVICE=1)
endif()

include_directories(Cacus)
include_directories("./")

file(GLOB CACUS_HEADERS Cacus/*.h)

add_library(
  Cacus
  ${CACUSLIB_LINKAGE}
  "Private/BaseDir.cpp"
  "Private/Cacus.cpp"
  "Private/CacusOutputDevice.cpp"
  "Private/CacusString.cpp"
  "Private/DebugCallback.cpp"
  "Private/Thread.cpp"
  "Private/Time.cpp"
  "Private/Ticker.cpp"
  "Private/CacusField.cpp"
  "Private/COutputDevicePrintf.cpp"
  "Private/Parsers.cpp"
  "Private/StackUnwinder.cpp"
  "Private/Unicode.cpp"
  "Private/URI.cpp"
  "Private/NetworkSocket.cpp"
  "Private/IPv6.cpp"

  "Private/IGD.cpp"
  "Private/CacusMem.cpp"

   ${CACUS_HEADERS}
)

if (LINUX)

  target_link_libraries(
    Cacus
    pthread
  )

endif()


# Move to Dir
install(
  TARGETS
    Cacus
  DESTINATION ${_PLATFORM_PREFIX}
)

# Include sub-projects.
add_subdirectory ("Experimental")

message("TARGETS IS ${TARGETS}")
message("INSTALLATION AT ${CMAKE_INSTALL_PREFIX}")