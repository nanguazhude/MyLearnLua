#ifndef __FILE_STSTEM_HPP_LUA
#define __FILE_STSTEM_HPP_LUA (1L)

#include "../lua.hpp"
#include <filesystem>
#include <string_view>

/*using */namespace filesystem = std::experimental::filesystem;

class LuaFileSystemPath {
	filesystem::path _m_path;
public:
	inline LuaFileSystemPath()  = default;
	inline ~LuaFileSystemPath() = default;
 
	inline operator filesystem::path &() & { return _m_path; }
	inline operator const filesystem::path &() const & { return _m_path; }
	inline operator filesystem::path &&() && { return std::move(_m_path); }

	inline LuaFileSystemPath(const LuaFileSystemPath&)=default;
	inline LuaFileSystemPath(LuaFileSystemPath&&)=default;
	inline LuaFileSystemPath&operator=(LuaFileSystemPath&)  = default;
	inline LuaFileSystemPath&operator=(LuaFileSystemPath&&) = default;
	/*this will alwarys create from utf8*/
	LUA_API LuaFileSystemPath(const std::string_view&) ;
	inline LuaFileSystemPath(const char *a,std::size_t b)
		:LuaFileSystemPath(std::string_view{a,b}) {}
public:
	LUA_API static int register_this(lua_State*);
	LUA_API static int create_path(lua_State*);
	LUA_API static int destory(lua_State*);
	LUA_API static int get_meta_table_name(lua_State*);
};

#endif

