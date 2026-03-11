cmake_minimum_required(VERSION 3.16)

project(eolib_c LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

enable_testing()
set(CMAKE_CTEST_ARGUMENTS "--output-on-failure")

include(GNUInstallDirs)

set(EOLIB_VERSION "0.1.0")

find_package(PkgConfig)
if(PkgConfig_FOUND)
	pkg_check_modules(LIBXML2 QUIET libxml-2.0)
	pkg_check_modules(JSONC QUIET IMPORTED_TARGET json-c)
endif()

if(NOT LIBXML2_FOUND)
	find_package(LibXml2 REQUIRED)
	set(LIBXML2_INCLUDE_DIRS ${LIBXML2_INCLUDE_DIR})
	set(LIBXML2_LIBRARIES ${LIBXML2_LIBRARIES})
endif()

if(NOT JSONC_FOUND)
	find_package(json-c CONFIG REQUIRED)
	set(JSONC_LIBRARIES json-c::json-c)
endif()

add_executable(code_gen tools/code_gen.c)
target_include_directories(code_gen PRIVATE ${LIBXML2_INCLUDE_DIRS})
if(JSONC_FOUND)
	target_link_libraries(code_gen PRIVATE ${LIBXML2_LIBRARIES} PkgConfig::JSONC)
else()
	target_link_libraries(code_gen PRIVATE ${LIBXML2_LIBRARIES} ${JSONC_LIBRARIES})
endif()

set(EOLIB_PROTOCOL_C ${CMAKE_CURRENT_SOURCE_DIR}/src/protocol.c)
set(EOLIB_PROTOCOL_H ${CMAKE_CURRENT_SOURCE_DIR}/include/protocol.h)
set(EOLIB_PACKET_TESTS_C ${CMAKE_CURRENT_SOURCE_DIR}/tests/packet_tests.c)

add_custom_command(
	OUTPUT ${EOLIB_PROTOCOL_C} ${EOLIB_PROTOCOL_H} ${EOLIB_PACKET_TESTS_C}
	COMMAND code_gen
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	DEPENDS code_gen
	COMMENT "Generating protocol sources"
)

add_custom_target(generate_protocol ALL
	DEPENDS ${EOLIB_PROTOCOL_C} ${EOLIB_PROTOCOL_H} ${EOLIB_PACKET_TESTS_C}
)

add_library(eolib SHARED
    src/data.c
    src/encrypt.c
	src/sequencer.c
	${EOLIB_PROTOCOL_C}
)

set_source_files_properties(${EOLIB_PROTOCOL_C} PROPERTIES GENERATED TRUE)

target_include_directories(eolib PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
add_dependencies(eolib generate_protocol)
set_target_properties(eolib PROPERTIES OUTPUT_NAME "eo")
if(WIN32)
	set_target_properties(eolib PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON PREFIX "lib")
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

add_executable(eolib_encrypt_tests
	tests/encrypt_tests.c
	tests/test_utils.c
)
target_include_directories(eolib_encrypt_tests PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/include
	${CMAKE_CURRENT_SOURCE_DIR}/tests
)
target_link_libraries(eolib_encrypt_tests PRIVATE eolib)
set(EOLIB_ENCRYPT_TESTS
	encrypt_packet_known_vector
	decrypt_packet_known_vector
	server_verification_hash
	generate_swap_multiple
)
foreach(test_name IN LISTS EOLIB_ENCRYPT_TESTS)
	add_test(NAME eolib_encrypt_tests.${test_name} COMMAND eolib_encrypt_tests ${test_name})
endforeach()

add_executable(eolib_data_tests
	tests/data_tests.c
	tests/test_utils.c
)
target_include_directories(eolib_data_tests PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/include
	${CMAKE_CURRENT_SOURCE_DIR}/tests
)
target_link_libraries(eolib_data_tests PRIVATE eolib)
set(EOLIB_data_TESTS
	eo_writer_init_with_capacity
	eo_writer_ensure_capacity
	eo_encode_decode_number_roundtrip
	eo_encode_decode_string_roundtrip
	eo_encode_string_known_value
	eo_decode_string_known_value
	eo_writer_add_numbers
	eo_writer_add_strings_and_bytes
	eo_writer_string_sanitization_mode
	eo_reader_number_reads
	eo_reader_string_reads
	eo_reader_get_bytes
	eo_reader_chunked_mode
)
foreach(test_name IN LISTS EOLIB_data_TESTS)
	add_test(NAME eolib_data_tests.${test_name} COMMAND eolib_data_tests ${test_name})
endforeach()

add_executable(eolib_sequencer_tests
	tests/sequencer_tests.c
	tests/test_utils.c
)
target_include_directories(eolib_sequencer_tests PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/include
	${CMAKE_CURRENT_SOURCE_DIR}/tests
)
target_link_libraries(eolib_sequencer_tests PRIVATE eolib)
set(EOLIB_SEQUENCER_TESTS
	sequencer_next_wraps
	generate_sequence_start_range
	sequence_init_bytes_roundtrip
	sequence_ping_bytes_roundtrip
)
foreach(test_name IN LISTS EOLIB_SEQUENCER_TESTS)
	add_test(NAME eolib_sequencer_tests.${test_name} COMMAND eolib_sequencer_tests ${test_name})
endforeach()

set_source_files_properties(${EOLIB_PACKET_TESTS_C} PROPERTIES GENERATED TRUE)
add_executable(eolib_packet_tests
	${EOLIB_PACKET_TESTS_C}
	tests/test_utils.c
)
target_include_directories(eolib_packet_tests PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/include
	${CMAKE_CURRENT_SOURCE_DIR}/tests
)
target_link_libraries(eolib_packet_tests PRIVATE eolib)
add_dependencies(eolib_packet_tests generate_protocol)
set(PACKET_TEST_LIST_FILE "${CMAKE_CURRENT_SOURCE_DIR}/tests/packet_tests.list")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${PACKET_TEST_LIST_FILE}")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tests/packet_tests.c")
if(EXISTS "${PACKET_TEST_LIST_FILE}")
	file(STRINGS "${PACKET_TEST_LIST_FILE}" EOLIB_PACKET_TESTS)
	foreach(test_name IN LISTS EOLIB_PACKET_TESTS)
		if(NOT test_name STREQUAL "")
			add_test(NAME eolib_packet_tests.${test_name} COMMAND eolib_packet_tests ${test_name})
		endif()
	endforeach()
else()
	add_test(NAME eolib_packet_tests COMMAND eolib_packet_tests)
endif()

set(CPACK_PACKAGE_NAME "eolib")
set(CPACK_PACKAGE_VENDOR "sorokya")
set(CPACK_PACKAGE_VERSION ${EOLIB_VERSION})
set(CPACK_PACKAGE_CONTACT "https://github.com/sorokya/eolib-c")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt")
if(DEFINED EOLIB_PACKAGE_PLATFORM)
	set(CPACK_PACKAGE_FILE_NAME "eolib-${EOLIB_VERSION}-${EOLIB_PACKAGE_PLATFORM}")
else()
	set(CPACK_PACKAGE_FILE_NAME "eolib-${EOLIB_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
endif()
set(CPACK_GENERATOR "ZIP")

include(CPack)
