#ifndef TEXT_CONVERT_HPP
#define TEXT_CONVERT_HPP

#include "../lua.hpp"

LUA_API int sstd_utf8_to_local(lua_State*);
LUA_API int sstd_local_to_utf8(lua_State*);

#endif


