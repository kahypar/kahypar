################################################################################
# python/CMakeLists.txt
#
# Copyright (c) 2019 Timo Bingmann
#
# All rights reserved. Published under the MIT License in the LICENSE file.
################################################################################


#file(GLOB_RECURSE KAHYPAR_SOURCES RELATIVE ${PROJECT_SOURCE_DIR} *.h)


add_subdirectory(pybind11)
include_directories(${PROJECT_SOURCE_DIR})
pybind11_add_module(kahypar_python module.cpp)
target_link_libraries(kahypar_python PRIVATE ${Boost_LIBRARIES})

# rename kahypar_python target output to kahypar
set_target_properties(kahypar_python PROPERTIES OUTPUT_NAME kahypar)

################################################################################
