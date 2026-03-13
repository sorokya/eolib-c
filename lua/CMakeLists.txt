cmake_minimum_required(VERSION 3.16)

find_package(Lua REQUIRED)

set_source_files_properties(${EOLIB_LUA_PROTOCOL_C} ${EOLIB_LUA_PROTOCOL_H}
	PROPERTIES GENERATED TRUE)

add_library(lua_eolib MODULE
	src/eolib_lua.c
	src/lua_data.c
	src/lua_encrypt.c
	src/lua_rng.c
	src/lua_sequencer.c
	${EOLIB_LUA_PROTOCOL_C}
)

set_target_properties(lua_eolib PROPERTIES
	OUTPUT_NAME "eolib"
	PREFIX ""
)

# Lua on macOS expects .so, not the default .dylib for MODULE libraries.
if(APPLE)
	set_target_properties(lua_eolib PROPERTIES SUFFIX ".so")
endif()

target_include_directories(lua_eolib PRIVATE
	${LUA_INCLUDE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/src
	${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(lua_eolib PRIVATE eolib ${LUA_LIBRARIES})

add_dependencies(lua_eolib generate_protocol)

# Copy libeolib next to eolib.so in the build tree so tests can load it.
add_custom_command(TARGET lua_eolib POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
		$<TARGET_FILE:eolib>
		$<TARGET_FILE_DIR:lua_eolib>
	COMMENT "Copying eolib shared library for Lua tests"
)

# Resolve libeolib relative to eolib.so at runtime so the drop-in bundle works:
#   Linux:  $ORIGIN         - side-by-side
#           $ORIGIN/../..   - system install (lib/lua/5.x/ -> lib/)
#   macOS:  @loader_path      - side-by-side
#           @loader_path/../.. - system install
if(APPLE)
	set_target_properties(lua_eolib PROPERTIES
		INSTALL_RPATH "@loader_path;@loader_path/../.."
		BUILD_WITH_INSTALL_RPATH FALSE
	)
elseif(UNIX)
	set_target_properties(lua_eolib PROPERTIES
		INSTALL_RPATH "$ORIGIN;$ORIGIN/../.."
		BUILD_WITH_INSTALL_RPATH FALSE
	)
endif()

if(LUA_VERSION_STRING)
	string(REGEX MATCH "^[0-9]+\\.[0-9]+" LUA_VERSION_MAJOR_MINOR "${LUA_VERSION_STRING}")
	set(_lua_cmod "${CMAKE_INSTALL_LIBDIR}/lua/${LUA_VERSION_MAJOR_MINOR}")
	set(_lua_lmod "${CMAKE_INSTALL_DATADIR}/lua/${LUA_VERSION_MAJOR_MINOR}")
else()
	set(_lua_cmod "${CMAKE_INSTALL_LIBDIR}/lua")
	set(_lua_lmod "${CMAKE_INSTALL_DATADIR}/lua")
endif()

install(TARGETS lua_eolib
	LIBRARY DESTINATION ${_lua_cmod}
	RUNTIME DESTINATION ${_lua_cmod}
)

install(FILES "${CMAKE_SOURCE_DIR}/lua/eolib.d.lua"
	DESTINATION ${_lua_lmod}
)

# On Windows, libeolib.dll must be alongside eolib.dll (no rpath on Windows).
if(WIN32)
	install(FILES "$<TARGET_FILE:eolib>"
		DESTINATION ${_lua_cmod}
	)
endif()

# Standalone portable bundle (e.g. for CPack or local drop-in use)
install(TARGETS lua_eolib
	LIBRARY DESTINATION . COMPONENT lua_bindings EXCLUDE_FROM_ALL
	RUNTIME DESTINATION . COMPONENT lua_bindings EXCLUDE_FROM_ALL
)

install(FILES
		"${CMAKE_SOURCE_DIR}/lua/eolib.d.lua"
		"${CMAKE_SOURCE_DIR}/LICENSE.txt"
	DESTINATION .
	COMPONENT lua_bindings
	EXCLUDE_FROM_ALL
)

install(FILES "$<TARGET_FILE:eolib>"
	DESTINATION .
	COMPONENT lua_bindings
	EXCLUDE_FROM_ALL
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
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		COMMENT "Running Lua tests"
	)
endif()
