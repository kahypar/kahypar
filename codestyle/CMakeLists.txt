# Enables static code checks by replacing the C/C++ compiler with the wrapper script

# Pass current compiler to script via parameter
SET(CMAKE_CXX_FLAGS "-DWRAPPED_COMPILER=${CMAKE_CXX_COMPILER}")
SET(CMAKE_C_FLAGS   "-DWRAPPED_COMPILER=${CMAKE_C_COMPILER}")

# Find wrapper script and echo message
GET_FILENAME_COMPONENT( CODESTYLE_DIRECTORY ${CMAKE_CURRENT_LIST_FILE} PATH )
SET( CODESTYLE_COMPILER "${CODESTYLE_DIRECTORY}/compile_wrapper.sh" )
MESSAGE( STATUS "Replacing compiler command with ${CODESTYLE_COMPILER}." )

# Replace compiler
INCLUDE (CMakeForceCompiler)
   SET(CMAKE_SYSTEM_NAME Generic)
   CMAKE_FORCE_C_COMPILER   (${CODESTYLE_COMPILER} StandardCompilerWithChecks)
   CMAKE_FORCE_CXX_COMPILER (${CODESTYLE_COMPILER} StandardCompilerWithChecks)
