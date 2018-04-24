#ifndef __HPP_LUA_TABLE_TO_TEXT
#define __HPP_LUA_TABLE_TO_TEXT

#include "../lua.hpp"

/*
 * no out put
 * input table tablename outputfilename or table outputfilename
*/
LUA_API int sstd_print_table_by_std_ofstream(lua_State *)/*环表输出不完整*/;
LUA_API int sstd_full_print_table_by_std_ofstream(lua_State *)/*环表输出完整*/;
/*
 * no out put
 * input table tablename or table
*/
LUA_API int sstd_print_table_by_std_cout(lua_State *)/*环表输出不完整*/;
LUA_API int sstd_full_print_table_by_std_cout(lua_State *)/*环表输出完整*/;
/*
 * out put string
 * input table tablename or table
*/
LUA_API int sstd_full_print_table_to_string(lua_State *L);
LUA_API int sstd_print_table_to_string(lua_State *L);

#endif

