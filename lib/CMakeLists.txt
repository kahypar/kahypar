include(GNUInstallDirs)

add_library(kahypar SHARED libkahypar.cc)
target_link_libraries(kahypar ${Boost_LIBRARIES})

set_target_properties(kahypar PROPERTIES
    PUBLIC_HEADER ../include/libkahypar.h)

target_include_directories(kahypar PRIVATE ../include)

configure_file(libkahypar.pc.in libkahypar.pc @ONLY)

install(TARGETS kahypar
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(FILES ${CMAKE_BINARY_DIR}/lib/libkahypar.pc
       DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)

configure_file(cmake_uninstall.cmake.in cmake_uninstall.cmake IMMEDIATE @ONLY)

add_custom_target(uninstall-kahypar "${CMAKE_COMMAND}" -P cmake_uninstall.cmake)
