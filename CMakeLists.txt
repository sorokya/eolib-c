cmake_minimum_required(VERSION 3.16)

project(eolib_c LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

include(GNUInstallDirs)

set(EOLIB_VERSION "0.1.0")

find_package(PkgConfig)
if(PkgConfig_FOUND)
	pkg_check_modules(LIBXML2 QUIET libxml-2.0)
endif()

if(NOT LIBXML2_FOUND)
	find_package(LibXml2 REQUIRED)
	set(LIBXML2_INCLUDE_DIRS ${LIBXML2_INCLUDE_DIR})
	set(LIBXML2_LIBRARIES ${LIBXML2_LIBRARIES})
endif()

add_executable(code_gen tools/code_gen.c)
target_include_directories(code_gen PRIVATE ${LIBXML2_INCLUDE_DIRS})
target_link_libraries(code_gen PRIVATE ${LIBXML2_LIBRARIES})

add_custom_command(
	OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/codegen.stamp
	COMMAND code_gen
	COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/codegen.stamp
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	DEPENDS code_gen
	COMMENT "Generating protocol sources"
)

add_custom_target(generate_protocol ALL
	DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/codegen.stamp
)

add_library(eolib SHARED
    src/eo_data.c
    src/encrypt.c
    src/protocol.c
)

target_include_directories(eolib PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
add_dependencies(eolib generate_protocol)
set_target_properties(eolib PROPERTIES OUTPUT_NAME "eolib" PREFIX "")
if(WIN32)
	set_target_properties(eolib PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

install(TARGETS eolib
	EXPORT eolibTargets
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

include(CMakePackageConfigHelpers)
set(EOLIB_CONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/eolib")

configure_package_config_file(
	${CMAKE_CURRENT_SOURCE_DIR}/cmake/eolibConfig.cmake.in
	${CMAKE_CURRENT_BINARY_DIR}/eolibConfig.cmake
	INSTALL_DESTINATION ${EOLIB_CONFIG_INSTALL_DIR}
)

write_basic_package_version_file(
	${CMAKE_CURRENT_BINARY_DIR}/eolibConfigVersion.cmake
	VERSION ${EOLIB_VERSION}
	COMPATIBILITY AnyNewerVersion
)

install(FILES
	${CMAKE_CURRENT_BINARY_DIR}/eolibConfig.cmake
	${CMAKE_CURRENT_BINARY_DIR}/eolibConfigVersion.cmake
	DESTINATION ${EOLIB_CONFIG_INSTALL_DIR}
)

install(EXPORT eolibTargets
	NAMESPACE eolib::
	FILE eolibTargets.cmake
	DESTINATION ${EOLIB_CONFIG_INSTALL_DIR}
)
