if(EXISTS ${PROJECT_SOURCE_DIR}/.git)
  find_package(Git)
  if(GIT_FOUND)
    execute_process(
      COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
      WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
      OUTPUT_VARIABLE "KaHyPar_BUILD_VERSION"
      ERROR_QUIET
      OUTPUT_STRIP_TRAILING_WHITESPACE)
    message( STATUS "Git version: ${KaHyPar_BUILD_VERSION}" )
  else(GIT_FOUND)
    set(KaHyPar_BUILD_VERSION 0)
  endif(GIT_FOUND)
endif(EXISTS ${PROJECT_SOURCE_DIR}/.git)

configure_file(${PROJECT_SOURCE_DIR}/src/lib/GitRevision.h.in
               ${PROJECT_BINARY_DIR}/src/lib/GitRevision.h)

# add the binary tree to the search path for include files so that we will find GitRevision.h
include_directories(${PROJECT_BINARY_DIR}/src)