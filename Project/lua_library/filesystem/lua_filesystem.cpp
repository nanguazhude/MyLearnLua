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
	if (luaL_newmetatable(L, meta_table_name())) {
		const auto varTableIndex = lua_gettop(L);

		pushlstring(L, u8R"(__index)"sv);
		lua_pushvalue(L, varTableIndex);
		lua_settable(L, varTableIndex);

		pushlstring(L, u8R"(new)"sv);
		lua_pushcfunction(L, &LuaFileSystemPath::create_path);
		lua_settable(L, varTableIndex);

		pushlstring(L, u8R"(get_meta_table_name)"sv);
		lua_pushcfunction(L, &LuaFileSystemPath::get_meta_table_name);
		lua_settable(L, varTableIndex);

		pushlstring(L, u8R"(__gc)"sv);
		lua_pushcfunction(L, &LuaFileSystemPath::destory);
		lua_settable(L, varTableIndex);

	}
	return 1;
}

namespace {
	void create_path_make_mtable(lua_State*L, int varInput) {
		LuaFileSystemPath::register_this(L);
		lua_setmetatable(L, varInput);
	}
}/*namespace*/

LUA_API int LuaFileSystemPath::get_meta_table_name(lua_State*L) {
	auto varP = reinterpret_cast<LuaFileSystemPath *>(
		*reinterpret_cast<void **>(
			luaL_checkudata(L, lua_gettop(L), meta_table_name())));
	pushlstring(L, meta_table_name());
	(void)varP;
	return 1;
}

LUA_API int LuaFileSystemPath::destory(lua_State*L) {
	auto varP = reinterpret_cast<LuaFileSystemPath *>(
		*reinterpret_cast<void **>(
			luaL_checkudata(L, lua_gettop(L), meta_table_name())));
	delete varP;
	return 0;
}

LUA_API int LuaFileSystemPath::create_path(lua_State*L) {
	const auto varInput = lua_gettop(L);
	const auto varType = lua_type(L, varInput);
	if (varType == LUA_TSTRING) {/*create*/
		std::size_t n = 0;
		const char * d = nullptr;
		d = lua_tolstring(L, varInput, &n);
		if (n > 0) {
			::new(lua_newuserdata(L, sizeof(LuaFileSystemPath*)))
				LuaFileSystemPath*{ new LuaFileSystemPath(d, n) };
		}
		else {
			::new(lua_newuserdata(L, sizeof(LuaFileSystemPath*)))
				LuaFileSystemPath*{ new LuaFileSystemPath() };
		}
		create_path_make_mtable(L, lua_gettop(L));
		return 1;
	}
	else if (varType == LUA_TUSERDATA) {/*copy create*/
		auto varP = reinterpret_cast<LuaFileSystemPath *>(
			*reinterpret_cast<void **>(
				luaL_testudata(L, varInput, meta_table_name())));
		if (varP) {
			::new(lua_newuserdata(L, sizeof(LuaFileSystemPath*)))
				LuaFileSystemPath*{ new LuaFileSystemPath(*varP) };
			create_path_make_mtable(L, lua_gettop(L));
			return 1;
		}
	}

	if ((varInput == 0) || ((varInput == 1) && (varType == LUA_TTABLE))) {
		/*if have no input ,or it called as table:new() then create it*/
		::new(lua_newuserdata(L, sizeof(LuaFileSystemPath*)))
			LuaFileSystemPath*{ new LuaFileSystemPath() };
		create_path_make_mtable(L, lua_gettop(L));
		return 1;
	}

	__push_lua_error(L, u8R"(can not convert input to filesystempath)"sv);
	return 0;
}
/*******************************************/
