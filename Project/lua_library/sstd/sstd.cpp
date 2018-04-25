/*sstd lua library*/
#ifndef LUA_LIB
#define LUA_LIB std::size_t(1)
#endif

#include "../lua.hpp"
#include "../lauxlib.hpp"
#include "../lualib.hpp"

LUA_API int sstd_print_table_by_std_ofstream(lua_State *L);
LUA_API int sstd_print_table_by_std_cout(lua_State *L);
LUA_API int sstd_full_print_table_by_std_ofstream(lua_State *L);
LUA_API int sstd_full_print_table_by_std_cout(lua_State *L);
LUA_API int sstd_full_print_table_to_string(lua_State *L);
LUA_API int sstd_print_table_to_string(lua_State *L);
LUA_API int sstd_utf8_to_local(lua_State*);
LUA_API int sstd_local_to_utf8(lua_State*);

namespace {

	static constexpr const luaL_Reg sstd_libs[] = {
	{ u8R"(tfile)" ,&sstd_print_table_by_std_ofstream },
	{ u8R"(tcout)" ,&sstd_print_table_by_std_cout },
	{ u8R"(ftfile)" ,&sstd_full_print_table_by_std_ofstream },
	{ u8R"(ftcout)" ,&sstd_full_print_table_by_std_cout },
	{ u8R"(ftstring)" ,&sstd_full_print_table_to_string },
	{ u8R"(tstring)" ,&sstd_print_table_to_string },
	{ u8R"(utf8tolocal)",&sstd_utf8_to_local },
	{ u8R"(localtoutf8)",&sstd_utf8_to_local },
	{ nullptr, nullptr },
	};

}/*namespace*/

LUAMOD_API int sstd_open(lua_State *L) {
	luaL_newlib(L, sstd_libs);
	return 1;
}


