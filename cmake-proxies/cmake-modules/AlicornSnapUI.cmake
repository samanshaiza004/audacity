# SPDX-License-Identifier: GPL-2.0-or-later
include_guard( GLOBAL )
include( "${CMAKE_CURRENT_LIST_DIR}/AlicornSnapUIPin.cmake" )
get_filename_component( AUDACITY_ALICORN_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE )
set( AUDACITY_ALICORN_SOURCE "" CACHE PATH "Clean pinned Alicorn source checkout" )
if( NOT EXISTS "${AUDACITY_ALICORN_SOURCE}/runtime/runtime.odin" )
   message( FATAL_ERROR "Set AUDACITY_ALICORN_SOURCE to the pinned Alicorn Git checkout" )
endif()

find_package( Git REQUIRED )
find_program( AUDACITY_ALICORN_ODIN odin )
if( NOT AUDACITY_ALICORN_ODIN )
   message( FATAL_ERROR "Alicorn Snap UI requires the Odin compiler on PATH" )
endif()
if( NOT CMAKE_SIZEOF_VOID_P EQUAL 8 )
   message( FATAL_ERROR "Alicorn Snap UI currently requires a 64-bit build" )
endif()

if( APPLE )
   if( CMAKE_OSX_ARCHITECTURES STREQUAL "arm64" OR
       ( NOT CMAKE_OSX_ARCHITECTURES AND CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm64|aarch64)$" ) )
      set( AUDACITY_ALICORN_ODIN_TARGET darwin_arm64 )
   elseif( CMAKE_OSX_ARCHITECTURES STREQUAL "x86_64" OR
           ( NOT CMAKE_OSX_ARCHITECTURES AND CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64|amd64)$" ) )
      set( AUDACITY_ALICORN_ODIN_TARGET darwin_amd64 )
   else()
      message( FATAL_ERROR "Alicorn Snap UI requires a single supported macOS architecture" )
   endif()
elseif( WIN32 )
   if( CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64|amd64)$" )
      set( AUDACITY_ALICORN_ODIN_TARGET windows_amd64 )
   else()
      message( FATAL_ERROR "Alicorn Snap UI currently supports Windows x64 only" )
   endif()
elseif( CMAKE_SYSTEM_NAME STREQUAL "Linux" )
   if( CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64)$" )
      set( AUDACITY_ALICORN_ODIN_TARGET linux_arm64 )
   elseif( CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64|amd64)$" )
      set( AUDACITY_ALICORN_ODIN_TARGET linux_amd64 )
   else()
      message( FATAL_ERROR "Alicorn Snap UI currently supports Linux x64 and arm64 only" )
   endif()
else()
   message( FATAL_ERROR "Alicorn Snap UI does not support ${CMAKE_SYSTEM_NAME}" )
endif()

execute_process( COMMAND "${AUDACITY_ALICORN_ODIN}" version
   OUTPUT_VARIABLE odin_version RESULT_VARIABLE odin_version_result )
if( NOT odin_version_result EQUAL 0 )
   message( FATAL_ERROR "Could not query Odin compiler version: ${odin_version}" )
endif()

set( alicorn_verify_command "${CMAKE_COMMAND}"
   "-DGIT_EXECUTABLE=${GIT_EXECUTABLE}" "-DALICORN_SOURCE=${AUDACITY_ALICORN_SOURCE}"
   "-DEXPECTED_REVISION=${AUDACITY_ALICORN_REVISION}"
   -P "${CMAKE_CURRENT_LIST_DIR}/VerifyAlicornSnapUI.cmake" )
execute_process( COMMAND ${alicorn_verify_command} RESULT_VARIABLE verify_result )
if( NOT verify_result EQUAL 0 )
   message( FATAL_ERROR "Alicorn source verification failed" )
endif()

set( alicorn_odin_dir "${CMAKE_BINARY_DIR}/alicorn-snap-ui-odin" )
set( alicorn_odin_archive "${alicorn_odin_dir}/${CMAKE_STATIC_LIBRARY_PREFIX}audacity_alicorn_snap_ui${CMAKE_STATIC_LIBRARY_SUFFIX}" )
file( GLOB_RECURSE alicorn_runtime_inputs CONFIGURE_DEPENDS
   "${AUDACITY_ALICORN_SOURCE}/runtime/*.odin"
   "${AUDACITY_ALICORN_SOURCE}/text_interaction/*.odin"
   "${AUDACITY_ALICORN_SOURCE}/third_party/Runa/*.odin"
   "${AUDACITY_ALICORN_SOURCE}/assets/fonts/*.ttf" )
add_custom_command(
   OUTPUT "${alicorn_odin_archive}"
   COMMAND "${CMAKE_COMMAND}" -E make_directory "${alicorn_odin_dir}"
   COMMAND "${AUDACITY_ALICORN_ODIN}" build "${AUDACITY_ALICORN_ROOT}/src/alicorn"
      "-collection:alicorn=${AUDACITY_ALICORN_SOURCE}"
      -build-mode:lib -no-entry-point "-target:${AUDACITY_ALICORN_ODIN_TARGET}"
      "-out:${alicorn_odin_archive}"
   DEPENDS
      "${AUDACITY_ALICORN_ROOT}/src/alicorn/AudacitySnapView.odin"
      "${AUDACITY_ALICORN_ROOT}/src/alicorn/AudacitySnapView.h"
      ${alicorn_runtime_inputs}
   WORKING_DIRECTORY "${AUDACITY_ALICORN_ROOT}"
   VERBATIM )
add_custom_target( audacity-alicorn-snap-ui-odin-build DEPENDS "${alicorn_odin_archive}" )
add_library( audacity-alicorn-snap-ui-odin STATIC IMPORTED GLOBAL )
set_target_properties( audacity-alicorn-snap-ui-odin PROPERTIES IMPORTED_LOCATION "${alicorn_odin_archive}" )

find_package( Threads REQUIRED )
add_library( audacity-alicorn-snap-ui INTERFACE )
target_include_directories( audacity-alicorn-snap-ui INTERFACE "${AUDACITY_ALICORN_ROOT}/src/alicorn" )
target_link_libraries( audacity-alicorn-snap-ui INTERFACE Threads::Threads ${CMAKE_DL_LIBS} )
if( CMAKE_SYSTEM_NAME STREQUAL "Linux" )
   target_link_libraries( audacity-alicorn-snap-ui INTERFACE m )
endif()
