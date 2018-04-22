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

namespace {

	static constexpr const luaL_Reg sstd_libs[] = {
	{ u8R"(tofile)" ,&sstd_print_table_by_std_ofstream },
	{ u8R"(tcout)" ,&sstd_print_table_by_std_cout },
	{ u8R"(ftfile)" ,&sstd_full_print_table_by_std_ofstream },
	{ u8R"(ftcout)" ,&sstd_full_print_table_by_std_cout },
	{ nullptr, nullptr },
	};

}/*namespace*/

LUAMOD_API int sstd_open(lua_State *L) {
	luaL_newlib(L, sstd_libs);
	return 1;
}


