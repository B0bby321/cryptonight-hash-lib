cmake_minimum_required(VERSION 2.8)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_MODULE_PATH})
include(Cryptonite)
message(STATUS "Target architectures: ${CMAKE_TARGET_ARCHITECTURES}")

project(cryptonite_hash C ASM)

set(LINK_TYPE SHARED)
# Add a CMake parameter for choosing a desired Python version
option(MAKE_CLIB "Builds a shared library for C")
option(MAKE_CLIB_STATIC "Builds a static library for C")
if(MAKE_CLIB OR MAKE_CLIB_STATIC)
	add_definitions(-DCLIB)
	if(MAKE_CLIB_STATIC)
		set(MAKE_CLIB ON)
		set(LINK_TYPE STATIC)
		message(STATUS "Building static library for C Lib")
	else()
		message(STATUS "Building shared library for C Lib")
	endif()
endif()

# Set a default build configuration if none is specified. 'Release' produces the fastest binaries
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Release' as none was specified.")
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
    "MinSizeRel" "RelWithDebInfo")
endif()
string(TOUPPER "${CMAKE_BUILD_TYPE}" U_CMAKE_BUILD_TYPE)

# Try to autodetect Python (can be overridden manually if needed)
set(Python_ADDITIONAL_VERSIONS 3.4 3.5 3.6)
find_package(PythonLibs ${CRYPTONIGHT_PYTHON_VERSION} REQUIRED)

# Include path for Python header files
include_directories(${PYTHON_INCLUDE_DIR})

# include all subdirs at source directory
include_directories(${PROJECT_SOURCE_DIR}/.)
include_directories(${PROJECT_SOURCE_DIR}/crypto)

set(CRYPTO_HEADERS
	crypto/oaes_lib.h
	crypto/c_keccak.h
	crypto/c_groestl.h
	crypto/c_blake256.h
	crypto/c_jh.h
	crypto/c_skein.h
	crypto/int-util.h
	crypto/hash-ops.h
	crypto/hash.h
)

set(CRYPTO_SOURCES
	crypto/aesb.c
	crypto/oaes_lib.c
	crypto/c_keccak.c
	crypto/c_groestl.c
	crypto/c_blake256.c
	crypto/c_jh.c
	crypto/c_skein.c
	crypto/hash.c
)

include_directories(${PROJECT_SOURCE_DIR}/compat)

set(COMPAT_HEADERS
	compat/cpuminer-config.h
	compat/inttypes.h
	compat/stdbool.h
	compat/unistd.h
	compat/winansi.h
)

# Use Assembly file for AES-IN optimization only on x64 env 
# and not on Visual Studio
if( "${CMAKE_TARGET_ARCHITECTURES}" STREQUAL "x86_64" AND NOT MSVC )
    set(ASM
		asm/aesb-x64.S
	)
else()
    set(ASM
		#asm/aesb-x86.S
		#crypto/aesb-x86-impl.c
	)
endif()



if(MAKE_CLIB)
	# Create the binding library for C
	add_library(cryptonite_hash ${LINK_TYPE}
   #${PYBIND11_HEADERS}
   ${CRYPTO_HEADERS}
   ${COMPAT_HEADERS}
   ${CRYPTO_SOURCES}
   ${ASM}
   compat.h
   miner.h
   cryptonite_hash.h
   cryptonite_hash.c
   cryptolite_hash.c
   sysinfos.c
  # ... extra files go here ...
)
set_target_properties(cryptonite_hash PROPERTIES PREFIX "lib")
configure_file(cryptonite_hash_Capi.h cryptonite_hash_Capi.h COPYONLY)
else()
# Create the binding library for python
add_library(cryptonite_hash SHARED
   #${PYBIND11_HEADERS}
   ${CRYPTO_HEADERS}
   ${COMPAT_HEADERS}
   ${CRYPTO_SOURCES}
   ${ASM}
   compat.h
   miner.h
   cryptonite_hash.h
   cryptonite_hash.c
   cryptolite_hash.c
   sysinfos.c
   cryptonitehashmodule.c
  # ... extra files go here ...
)
# Don't add a 'lib' prefix to the shared library
set_target_properties(cryptonite_hash PROPERTIES PREFIX "")
endif()


if (WIN32)
  if (MSVC)
    # Enforce fastcode generation /O2 /Ot
	# /bigobj is needed for bigger binding projects due to the limit to 64k addressable sections
    # /MP enables multithreaded builds (relevant when there are many files).
    set_target_properties(cryptonite_hash PROPERTIES COMPILE_FLAGS "/O2 /Ot /GL /MP /bigobj /D _CRT_SECURE_NO_WARNINGS")
    set_target_properties(cryptonite_hash PROPERTIES LINK_FLAGS "/LTCG")
  elseif(MINGW)
    set_target_properties(cryptonite_hash PROPERTIES COMPILE_FLAGS "-O2 -D_REENTRANT -fmerge-all-constants")
  endif()
  
  if(NOT MAKE_CLIB)
	  # Strip unnecessary sections of the binary on MINGW
	  if (MINGW)
		if(LINK_TYPE EQUAL STATIC)
			set_target_properties(cryptonite_hash PROPERTIES SUFFIX ".a")
		endif()
		if (NOT ${U_CMAKE_BUILD_TYPE} MATCHES DEBUG)
		  message(STATUS "Add strip post-build command")
		  add_custom_command(TARGET cryptonite_hash POST_BUILD COMMAND strip -p --strip-debug --strip-unneeded ${PROJECT_BINARY_DIR}/cryptonite_hash.pyd)
		endif()
	  endif()
	endif()
elseif (UNIX)
  # It's quite common to have multiple copies of the same Python version
  # installed on one's system. E.g.: one copy from the OS and another copy
  # that's statically linked into an application like Blender or Maya.
  # If we link our plugin library against the OS Python here and import it
  # into Blender or Maya later on, this will cause segfaults when multiple
  # conflicting Python instances are active at the same time.

  # Windows is not affected by this issue since it handles DLL imports
  # differently. The solution for Linux and Mac OS is simple: we just don't
  # link against the Python library. The resulting shared library will have
  # missing symbols, but that's perfectly fine -- they will be resolved at
  # import time.

  set_target_properties(cryptonite_hash PROPERTIES COMPILE_FLAGS "-O2 -Wno-pointer-sign -Wno-pointer-to-int-cast")
	
  if(${LINK_TYPE} MATCHES SHARED)
  	set_target_properties(cryptonite_hash PROPERTIES SUFFIX ".so")
  else()
	set_target_properties(cryptonite_hash PROPERTIES SUFFIX ".a")
  endif()
  if(NOT MAKE_CLIB)
  	# Strip unnecessary sections of the binary on Linux/Mac OS
	  message(STATUS "Add strip post-build command")
	  if(APPLE)
		set_target_properties(cryptonite_hash PROPERTIES MACOSX_RPATH ".")
		set_target_properties(cryptonite_hash PROPERTIES LINK_FLAGS "-undefined dynamic_lookup -dead_strip")
		if (NOT ${U_CMAKE_BUILD_TYPE} MATCHES DEBUG)
		  add_custom_command(TARGET cryptonite_hash POST_BUILD COMMAND strip -u -r ${PROJECT_BINARY_DIR}/cryptonite_hash.so)
		endif()
	  else()
		if (NOT ${U_CMAKE_BUILD_TYPE} MATCHES DEBUG)
		  add_custom_command(TARGET cryptonite_hash POST_BUILD COMMAND strip ${PROJECT_BINARY_DIR}/cryptonite_hash.so)
		endif()
	  endif()
	endif()
endif()


if(MAKE_CLIB)
	add_executable(LibTestOut Libtest.c)
	target_link_libraries(LibTestOut cryptonite_hash)
endif()
