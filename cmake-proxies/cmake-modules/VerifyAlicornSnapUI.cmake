# SPDX-License-Identifier: GPL-2.0-or-later
execute_process(
   COMMAND "${GIT_EXECUTABLE}" -C "${ALICORN_SOURCE}" rev-parse HEAD
   RESULT_VARIABLE result OUTPUT_VARIABLE revision ERROR_VARIABLE error
   OUTPUT_STRIP_TRAILING_WHITESPACE )
if( NOT result EQUAL 0 OR NOT revision STREQUAL EXPECTED_REVISION )
   message( FATAL_ERROR "Alicorn source must be a Git checkout at ${EXPECTED_REVISION}; got '${revision}'. ${error}" )
endif()
execute_process(
   COMMAND "${GIT_EXECUTABLE}" -C "${ALICORN_SOURCE}" status --porcelain --untracked-files=all --
      runtime text_interaction third_party/Runa assets/fonts
   RESULT_VARIABLE result OUTPUT_VARIABLE changes ERROR_VARIABLE error )
if( NOT result EQUAL 0 OR changes )
   message( FATAL_ERROR "Alicorn build inputs must be clean at ${EXPECTED_REVISION}: ${changes} ${error}" )
endif()
