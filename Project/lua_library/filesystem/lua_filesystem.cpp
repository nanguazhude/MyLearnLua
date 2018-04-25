#ifndef LUA_LIB
#define LUA_LIB std::size_t(1)
#endif

#include "lua_filesystem.hpp"
using namespace std::string_view_literals;

namespace {
	namespace fs {
		using namespace filesystem;
	}/**/
	[[noreturn]]void __push_lua_error(lua_State*L, const std::string_view &e) {
		lua_pushlstring(L, e.data(), e.size());
		lua_error(L);
	}

	inline void pushlstring(lua_State*L, const std::string_view &e) {
		lua_pushlstring(L, e.data(), e.size());
	}
}/**/

LUA_API LuaFileSystemPath::LuaFileSystemPath(const std::string_view&d) :
	_m_path(fs::u8path(d.cbegin(), d.cend())) {
}

namespace {
	constexpr const char * meta_table_name() {
		return "sstd::filesystem::path";
	}
}

LUA_API int LuaFileSystemPath::register_this(lua_State*L) {
	luaL_newmetatable(L, meta_table_name());
	const auto varTableIndex = lua_gettop(L);

	pushlstring(L, u8R"(new)"sv);
	lua_pushcfunction(L, &LuaFileSystemPath::create_path);

}

namespace {
	void create_path_make_mtable(lua_State*L, int varInput) {
		luaL_getmetatable(L, meta_table_name());
		lua_setmetatable(L, varInput);
	}
}/*namespace*/

LUA_API int LuaFileSystemPath::create_path(lua_State*L) {
	const auto varInput = lua_gettop(L);
	const auto varType = lua_type(L, varInput);
	if (varType == LUA_TSTRING) {
		std::size_t n = 0;
		const char * d = nullptr;
		d = lua_tolstring(L, varInput, &n);
		if (n > 0) {
			::new(lua_newuserdata(L, sizeof(LuaFileSystemPath)))
				LuaFileSystemPath(d, n);
		}
		else {
			::new(lua_newuserdata(L, sizeof(LuaFileSystemPath)))
				LuaFileSystemPath();
		}
		create_path_make_mtable(L, lua_gettop(L));
		return 1;
	}
	else if (varType == LUA_TUSERDATA) {
		auto varP = reinterpret_cast<LuaFileSystemPath *>(
			luaL_checkudata(L, varInput, meta_table_name()));
		::new(lua_newuserdata(L, sizeof(LuaFileSystemPath)))
			LuaFileSystemPath(*varP);
		create_path_make_mtable(L, lua_gettop(L));
		return 1;
	}
	__push_lua_error(L, u8R"(can not convert input to filesystempath)"sv);
	return 0;
}
/*******************************************/
