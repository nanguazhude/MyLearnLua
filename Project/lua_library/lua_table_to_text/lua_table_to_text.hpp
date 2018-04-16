#ifndef __HPP_LUA_TABLE_TO_TEXT
#define __HPP_LUA_TABLE_TO_TEXT

#include "../lua.hpp"

/*
 * no out put
 * input table tablename outputfilename or table outputfilename
*/
LUA_API int print_table_by_std_ofstream(lua_State *);

/*
 * no out put
 * input table tablename or table
*/
LUA_API int print_table_by_std_cout(lua_State *);

#endif

