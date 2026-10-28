set(LUA_SOURCES
	src/lapi.c
	src/lcode.c
	src/ldebug.c
	src/ldo.c
	src/ldump.c
	src/lfunc.c
	src/lgc.c
	src/llex.c
	src/lmem.c
	src/lobject.c
	src/lopcodes.c
	src/lparser.c
	src/lstate.c
	src/lstring.c
	src/ltable.c
	src/ltests.c
	src/ltm.c
	src/lundump.c
	src/lvm.c
	src/lzio.c
	src/lib/lauxlib.c
	src/lib/lbaselib.c
	src/lib/ldblib.c
	src/lib/liolib.c
	src/lib/lmathlib.c
	src/lib/loadlib.c
	src/lib/lstrlib.c
	src/lib/ltablib.c
)

add_library(liblua STATIC ${LUA_SOURCES})

# MSVC's optimizer miscompiles Lua 5.0's table rehash (resize() receives nasize=INT_MIN),
# crashing qc and game at quest load. Debug and clang-cl builds are unaffected.
if (MSVC AND NOT CMAKE_C_COMPILER_ID STREQUAL "Clang")
	target_compile_options(liblua PRIVATE /Od)
endif()

target_include_directories(liblua PUBLIC "include" .)