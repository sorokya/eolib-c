cmake_minimum_required(VERSION 3.16)

find_package(Lua REQUIRED)

set_source_files_properties(${EOLIB_LUA_PROTOCOL_C} ${EOLIB_LUA_PROTOCOL_H}
	PROPERTIES GENERATED TRUE)

add_library(lua_eolib MODULE
	src/eolib_lua.c
	src/lua_data.c
	src/lua_encrypt.c
	src/lua_sequencer.c
	${EOLIB_LUA_PROTOCOL_C}
)

set_target_properties(lua_eolib PROPERTIES
	OUTPUT_NAME "eolib"
	PREFIX ""
)

if(APPLE)
	set_target_properties(lua_eolib PROPERTIES SUFFIX ".so")
elseif(WIN32)
	set_target_properties(lua_eolib PROPERTIES SUFFIX ".dll")
endif()

target_include_directories(lua_eolib PRIVATE
	${LUA_INCLUDE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/src
	${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(lua_eolib PRIVATE eolib ${LUA_LIBRARIES})

add_dependencies(lua_eolib generate_protocol)

install(TARGETS lua_eolib
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

find_program(LUA_INTERPRETER NAMES lua lua5.4 lua5.3 lua5.2 lua5.1 luajit)

if(LUA_INTERPRETER)
	file(GLOB LUA_TEST_FILES "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_*.lua")
	if(WIN32)
		set(LUA_MODULE_EXT "dll")
	else()
		set(LUA_MODULE_EXT "so")
	endif()
	set(LUA_ENV "LUA_CPATH=${CMAKE_CURRENT_BINARY_DIR}/?.${LUA_MODULE_EXT}")
	set(lua_test_commands "")
	foreach(test_file ${LUA_TEST_FILES})
		list(APPEND lua_test_commands
			COMMAND ${CMAKE_COMMAND} -E env ${LUA_ENV}
				${LUA_INTERPRETER} ${test_file}
		)
	endforeach()
	add_custom_target(lua-test
		${lua_test_commands}
		DEPENDS lua_eolib
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		COMMENT "Running Lua tests"
	)
endif()
