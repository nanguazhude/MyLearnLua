#ifndef LUA_LIB
#define LUA_LIB std::size_t(1)
#endif

#include <memory>
#include "text_convert.hpp"

#include <string_view>
using namespace std::string_view_literals;

#if defined( _WIN32 )||defined(WIN32)
#define Windows_Like_Value (1)
#else
#define Windows_Like_Value (0)
#endif

namespace {

	class TempBuffer {
	public:
		constexpr static int size = 1024;
		wchar_t WBuffer[TempBuffer::size];
	};
	thread_local TempBuffer tmpBuffer;
	using WLock = std::unique_ptr<wchar_t, void(*)(wchar_t*)>;
	using Lock = std::unique_ptr<char, void(*)(char*)>;

	[[noreturn]] inline void __push__error(lua_State *L, const std::string_view&e) {
		lua_pushlstring(L, e.data(), e.size());
		lua_error(L);
	}

#if Windows_Like_Value
#include <windows.h>
	bool need_convert() {
		static bool ans = []()->bool {
			//CPINFOEX info;
			//GetCPInfoEx(GetACP(), 0, &info);
			//return info.CodePageName;
			/*如果当前代码页不是UTF8,则需要转换*/
			return GetACP() != /*utf8*/65001;
		}();
		return ans;
	}
	//local转UTF-8
	int _p_LocalToUtf8(lua_State *L, const char * d, std::size_t n) {
		if (n < 1) {
			lua_pushlstring(L, nullptr, 0);
			return 1;
		}

		WLock wcLock{ nullptr, nullptr };

		//获得Length
		int len = MultiByteToWideChar(CP_ACP, 0, d, static_cast<int>(n), nullptr, 0);
		if (len < 1) {
			__push__error(L, u8R"(error on text convert)"sv);
		}
		//获得临时内存
		wchar_t *strUnicode = (len <= TempBuffer::size) ? tmpBuffer.WBuffer :
			(wcLock = WLock{ new wchar_t[len],[](wchar_t*d) {delete[]d; } }).get();
		//wmemset(strUnicode, 0, len)/*set them to zero*/;
		//进行字符转换
		len = MultiByteToWideChar(CP_ACP, 0, d, static_cast<int>(n), strUnicode, len);
		const auto wideLen = len;
		if (len < 1) {
			__push__error(L, u8R"(error on text convert)"sv);
		}

		//unicode to UTF-8
		//get the length 
		len = WideCharToMultiByte(CP_UTF8, 0, strUnicode, wideLen, nullptr, 0, nullptr, nullptr);
		if (len < 1) {
			__push__error(L, u8R"(error on text convert)"sv);
		}

		luaL_Buffer varBuffer;
		auto strUtf8 = luaL_buffinitsize(L, &varBuffer, len);
		len = WideCharToMultiByte(CP_UTF8, 0, strUnicode, wideLen, strUtf8, len, nullptr, nullptr);
		if (len < 1) {
			/*push the result and then remove the data*/
			luaL_pushresult(&varBuffer);
			lua_pop(L, 1);
			__push__error(L, u8R"(error on text convert)"sv);
		}

		luaL_pushresultsize(&varBuffer, len);

		return 1;
	}

	//UTF-8转gbk
	int _p_Utf8ToLocal(lua_State *L, const char * d, std::size_t n)
	{
		if (n < 1) {
			lua_pushlstring(L, nullptr, 0);
			return 1;
		}
		WLock wcLock{ nullptr, nullptr };
		//UTF-8转unicode
		int len = MultiByteToWideChar(CP_UTF8, 0, d, static_cast<int>(n), nullptr, 0);
		if (len < 1) {
			__push__error(L, u8R"(error on text convert)"sv);
		}
		wchar_t * strUnicode = (len <= TempBuffer::size) ? tmpBuffer.WBuffer :
			(wcLock = WLock{ new wchar_t[len],[](wchar_t*d) {delete[]d; } }).get();
		//wmemset(strUnicode, 0, len);
		len = MultiByteToWideChar(CP_UTF8, 0, d, static_cast<int>(n), strUnicode, len);
		if (len < 1) {
			__push__error(L, u8R"(error on text convert)"sv);
		}
		const auto wLen = len;

		//unicode转local
		len = WideCharToMultiByte(CP_ACP, 0, strUnicode, wLen, nullptr, 0, nullptr, nullptr);
		if (len < 1) {
			__push__error(L, u8R"(error on text convert)"sv);
		}

		luaL_Buffer varBuffer;
		auto strLocal = luaL_buffinitsize(L, &varBuffer, len);
		//memset(strLocal, 0, len);
		len = WideCharToMultiByte(CP_ACP, 0, strUnicode, wLen, strLocal, len, nullptr, nullptr);
		if (len < 1) {
			/*push the result and then remove the data*/
			luaL_pushresult(&varBuffer);
			lua_pop(L, 1);
			__push__error(L, u8R"(error on text convert)"sv);
		}

		luaL_pushresultsize(&varBuffer, len);
		return 1;
	}

#else
	/*just do nothing*/
#endif

}/*namespace*/

#if Windows_Like_Value
LUA_API int sstd_utf8_to_local(lua_State*L) {
	const auto varTop = lua_gettop(L);
	if (varTop < 1) { __push__error(L, "no input"sv); }
	if (false == need_convert()) { return 1; }
	const char *d = nullptr;
	std::size_t n = 0;
	if (lua_type(L, varTop) == LUA_TSTRING) {
		d = lua_tolstring(L, varTop, &n);
	}
	else {
		d = luaL_tolstring(L, varTop, &n);
	}
	return _p_Utf8ToLocal(L, d, n);
}

LUA_API int sstd_local_to_utf8(lua_State*L) {
	const auto varTop = lua_gettop(L);
	if (varTop < 1) { __push__error(L, "no input"sv); }
	if (false == need_convert()) { return 1; }
	const char *d = nullptr;
	std::size_t n = 0;
	if (lua_type(L, varTop) == LUA_TSTRING) {
		d = lua_tolstring(L, varTop, &n);
	}
	else {
		d = luaL_tolstring(L, varTop, &n);
	}
	return _p_LocalToUtf8(L, d, n);
}
#else

/*如果不是Windows,那么本地编码本身就是UTF8...*/
LUA_API int sstd_utf8_to_local(lua_State*L) {
	const auto varTop = lua_gettop(L);
	if (varTop < 1) { __push__error(L, "no input"sv); }
	return 1;
}

LUA_API int sstd_local_to_utf8(lua_State*L) {
	const auto varTop = lua_gettop(L);
	if (varTop < 1) { __push__error(L, "no input"sv); }
	return 1;
}

#endif
