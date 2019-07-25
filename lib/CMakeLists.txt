include(GNUInstallDirs)

add_library(kahypar SHARED libkahypar.cc)
target_link_libraries(kahypar ${Boost_LIBRARIES})

set_target_properties(kahypar PROPERTIES
    PUBLIC_HEADER ../include/libkahypar.h)

target_include_directories(kahypar PRIVATE ../include)

configure_file(libkahypar.pc.in libkahypar.pc @ONLY)

if(WIN32)
	install(TARGETS kahypar
		RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}
		PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
else()
	install(TARGETS kahypar
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
		PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif()

install(FILES ${CMAKE_BINARY_DIR}/lib/libkahypar.pc
       DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)

configure_file(cmake_uninstall.cmake.in cmake_uninstall.cmake IMMEDIATE @ONLY)

add_custom_target(uninstall-kahypar "${CMAKE_COMMAND}" -P cmake_uninstall.cmake)

add_custom_target(install.library 
    ${CMAKE_COMMAND} 
    -DBUILD_TYPE=${CMAKE_BUILD_TYPE} 
    -P ${CMAKE_BINARY_DIR}/cmake_install.cmake) 
ADD_DEPENDENCIES(install.library kahypar) 
