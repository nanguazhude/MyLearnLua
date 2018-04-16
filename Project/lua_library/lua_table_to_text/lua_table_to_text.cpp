#define QUICK_CHECK_CODE

#include "../lua.hpp"
#include "../part_goole_v8/include/double-conversion/double-conversion.h"

#include <list>
#include <regex>
#include <array>
#include <memory>
#include <string>
#include <cstddef>
#include <utility>
#include <string_view>
#include <type_traits>
#include <memory_resource>

#if defined(QUICK_CHECK_CODE)
#include <iostream>
#endif

using namespace std::string_view_literals;

namespace {

	using string_view = std::string_view;
	using string = std::string;// std::pmr::string;
	template<typename T> using list = std::list<T>;// std::pmr::list;

	/* Lua栈区最小值 */
	constexpr int inline concept_stack_min_size() { return 16; }
	/* 临时表最小大小 */
	constexpr int inline concept_min_tmp_table_size() { return 128; }
	/* 临时缓存区大小 */
	constexpr int inline concept_tmp_buffer_size() { return 256; }
	/* 连续=的最大优化值 */
	constexpr int inline max_equal_size() { return 256; }
#if defined(QUICK_CHECK_CODE)
	class WType {
	public:
		void inline write(const string_view &a) { std::cout << a; }
	};
#else
	template<typename WType/* WType w; w.write(const string_view &); */>
#endif
	class LuaLock {
	public:
		lua_State * $L;
		int $TableIndex;
		int $TmpTableIndex;
		int $ErrorStringIndex;
		int $ReturnClearIndex;
		int $UserTableIndex;
		int $UserKeyIndex;
		int $UserValueIndex;
		int $CurrentTableIndex = 1;
		std::remove_cv_t<std::remove_reference_t<WType>/**/> $Writer;
		std::array<char, concept_tmp_buffer_size() > $TmpStringBuffer;
		class TableItem {
		public:
			TableItem * const $Parent;
			int const $IndexInTmpTable;
			int $LastIndex = 1/*用于判断Array是否连续*/;
			bool $TableArrayContinue = true;
			bool $IsCreate = true;

			TableItem(TableItem * P, const int & arg) :$Parent(P), $IndexInTmpTable(arg) {}

			inline void destory(const LuaLock & arg) {
				lua_pushnil(arg);
				lua_rawseti(arg, arg.$TmpTableIndex, $IndexInTmpTable);
			}

			inline void get_table_name(const LuaLock & arg) {
				lua_rawgeti(arg, arg.$TmpTableIndex, $IndexInTmpTable);
				lua_rawgeti(arg, -1, 3);
			}

			template<bool isCreate >
			inline void push(const LuaLock & arg) {

				if constexpr(isCreate) {
					/*创建索引表*/
					lua_createtable(arg, 3, 0);
					const auto varThisTableIndex = lua_gettop(arg);
					{
						/* value */
						lua_pushvalue(arg, varThisTableIndex);
						/* set tmp[IndexInTmpTable] = value */
						lua_rawseti(arg, arg.$TmpTableIndex, $IndexInTmpTable);
					}
					/*
					this table
					parent table key
					parent table
					*/
					{
						/* item.key */
						lua_pushnil(arg);
						lua_rawseti(arg, varThisTableIndex, 1);
					}
					{
						/* item.table */
						lua_pushvalue(arg, varThisTableIndex - 1);
						lua_rawseti(arg, varThisTableIndex, 2);
					}
					{
						/* table name */
						lua_pushvalue(arg, varThisTableIndex - 2);
						lua_rawseti(arg, varThisTableIndex, 3);
					}
					/*移除不用的值*/
					lua_settop(arg, arg.$UserKeyIndex);
				}
				else {
					$IsCreate = false;
					lua_rawgeti(arg, arg.$TmpTableIndex, $IndexInTmpTable);
					const auto varThisTableIndex = lua_gettop(arg);
					/*
					table key
					table
					*/
					{
						/* item.key */
						lua_pushvalue(arg, varThisTableIndex - 1);
						lua_rawseti(arg, varThisTableIndex, 1);
					}
				}

			}

			inline void pop(const LuaLock & arg) {
				lua_rawgeti(arg, arg.$TmpTableIndex, $IndexInTmpTable);
				const auto varThisTableIndex = lua_gettop(arg);
				/*key*/
				lua_rawgeti(arg, varThisTableIndex, 1);
				lua_copy(arg, -1, arg.$UserKeyIndex);
				/*table*/
				lua_rawgeti(arg, varThisTableIndex, 2);
				lua_copy(arg, -1, arg.$UserTableIndex);
				/*移除不用的值*/
				lua_settop(arg, arg.$UserKeyIndex);
			}
		};

		list< TableItem > $ItemTables;
	public:

		/* int to string */
		std::string_view int_to_string(lua_Integer data) {

			/*加速运算结果*/
			switch (data) {
			case 0: return u8R"(0)"sv;
			case 1: return u8R"(1)"sv;
			case 2: return u8R"(2)"sv;
			case 3: return u8R"(3)"sv;
			case 4: return u8R"(4)"sv;
			case 5: return u8R"(5)"sv;
			case 6: return u8R"(6)"sv;
			case 7: return u8R"(7)"sv;
			case 8: return u8R"(8)"sv;
			case 9: return u8R"(9)"sv;
			case 10: return u8R"(10)"sv;
			}

			auto varPointerEnd = &(*$TmpStringBuffer.rbegin());
			auto varPointer = varPointerEnd;
			const bool isN = data < 0 ? (data = -data, true) : false;

			{
				lldiv_t varTmp;
				varTmp.quot = data;
				do {
					varTmp = std::lldiv(varTmp.quot, 10);
					switch (varTmp.rem) {
					case 0:*varPointer = '0'; break;
					case 1:*varPointer = '1'; break;
					case 2:*varPointer = '2'; break;
					case 3:*varPointer = '3'; break;
					case 4:*varPointer = '4'; break;
					case 5:*varPointer = '5'; break;
					case 6:*varPointer = '6'; break;
					case 7:*varPointer = '7'; break;
					case 8:*varPointer = '8'; break;
					case 9:*varPointer = '9'; break;
					}
					--varPointer;
				} while (varTmp.quot);
			}

			if (isN) {
				*varPointer = '-';
				--varPointer;
			}

			return { varPointer + 1, static_cast<std::size_t>(varPointer - varPointerEnd) };
		}

		/*double to string*/
		std::string_view double_to_string(lua_Number data) {
			const static auto & varCV = double_conversion::DoubleToStringConverter::EcmaScriptConverter();
			double_conversion::StringBuilder varSB{ $TmpStringBuffer.data(),static_cast<int>($TmpStringBuffer.size()) };
			varCV.ToShortest(data, &varSB);
			auto ansData = varSB.Finalize();
			return { ansData,static_cast<std::size_t>(varSB.position()) };
		}

		/* 将字符串加载到Lua栈 */
		inline void push_string(const string_view & s) {
			lua_pushlstring($L, s.data(), s.size());
		}

		template<typename U>
		inline LuaLock(lua_State * arg, U&&w) :$L(arg), $Writer(std::forward<U>(w)) {
			/* 保证最小栈区 */
			lua_checkstack($L, concept_stack_min_size());
			$TableIndex = lua_gettop($L);
			$ReturnClearIndex = $TableIndex;
			if (lua_istable($L, $TableIndex)) {
				/* 当出现错误时用于存放字符串 */
				//lua_pushnil($L);
				$ErrorStringIndex = $TableIndex + 1;
				/* 临时表,用于存放表索引 */
				//lua_createtable($L, concept_min_tmp_table_size(), 0);
				$TmpTableIndex = $TableIndex + 2;
				/*用于存放当前表*/
				//lua_pushvalue($L, $TableIndex);
				$UserTableIndex = $TableIndex + 3;
				/*用于存放当前key*/
				//lua_pushnil($L);
				$UserKeyIndex = $TableIndex + 4;
				/*用于存放当前value*/
				$UserValueIndex = $TableIndex + 5;
			}
			else {
				push_string(u8R"(the first item must be a table)"sv);
				lua_error($L);
			}
		}

		void error(const string_view & argErrorString) {
			/* 清除不使用的值 */
			$ReturnClearIndex = $ErrorStringIndex;
			lua_settop($L, $ReturnClearIndex);
			/* 抛出异常 */
			push_string(argErrorString);
			lua_replace($L, $ErrorStringIndex);
			lua_error($L);
		}

		~LuaLock() {
			if ($ReturnClearIndex == $ErrorStringIndex) { return; }
			lua_settop($L, $ReturnClearIndex);
		}

		void clear(const string_view & argTableName) {
			$CurrentTableIndex = 0;
			lua_settop(*this, $TableIndex);
			lua_settop(*this, $UserKeyIndex);
			{
				/*设置TmpTable*/
				lua_createtable($L, concept_min_tmp_table_size(), 0);
				lua_replace(*this, $TmpTableIndex);
			}
			/*初始化$ItemTables*/
			$ItemTables.clear();
			lua_pushvalue($L, $TableIndex);
			push_string(argTableName);
			lua_pushvalue($L, $TableIndex);
			$ItemTables.emplace_back(nullptr, ++$CurrentTableIndex).push<true>(*this);
		}

		/*just print value name*/
		bool print_name(TableItem*arg) {

			const auto varType = lua_type(*this, $UserKeyIndex);

			if (LUA_TNUMBER == varType) {
				int n;
				if (lua_isinteger(*this, $UserKeyIndex)) {
					auto d = lua_tointegerx(*this, $UserKeyIndex, &n);
					if (arg->$TableArrayContinue) {
						if (d == arg->$LastIndex) {
							++arg->$LastIndex;
							return/*连续table不输出变量名*/true;
						}
						else {
							arg->$TableArrayContinue = false;
						}
					}
					$Writer.write(u8R"([)"sv);
					return ($Writer.write(int_to_string(d)), $Writer.write(u8R"(])"sv), true);
				}
				else {
					arg->$TableArrayContinue = false;
					auto d = lua_tonumberx(*this, $UserKeyIndex, &n);
					$Writer.write(u8R"([)"sv);
					return ($Writer.write(double_to_string(d)), $Writer.write(u8R"(])"sv), true);
				}
			}

			arg->$TableArrayContinue = false;
			std::size_t n;
			auto d = lua_tolstring(*this, $UserKeyIndex, &n);
			if (n > 0) {
				return ($Writer.write({ d,n }), true);
			}
			else {
				return false;
			}

		}
		void print_equal() {
			return $Writer.write(u8R"( = )"sv);
		}

		void print_value_none() {
			return $Writer.write(u8R"(nil)"sv);
		}

		void print_value_nil() {
			return $Writer.write(u8R"(nil)"sv);
		}

		void print_value_bool() {
			if (lua_toboolean(*this, $UserValueIndex)) {
				return $Writer.write(u8R"(true)"sv);
			}
			else {
				return $Writer.write(u8R"(false)"sv);
			}
		}

		void print_value_lightuserdata() {
			return $Writer.write(u8R"("lightuserdata")"sv);
		}

		void print_value_number() {
			int n;
			if (lua_isinteger(*this, $UserValueIndex)) {
				auto d = lua_tointegerx(*this, $UserValueIndex, &n);
				return $Writer.write(int_to_string(d));
			}
			else {
				auto d = lua_tonumberx(*this, $UserValueIndex, &n);
				return $Writer.write(double_to_string(d));
			}
		}

		static inline const int _p_get_equal_count(const string_view & arg) {
			int ans = 0/*min value*/;
			int count = 0;
			auto p = arg.cbegin();
			auto e = arg.cend();
			for (; p != e; ++p) {
				if (*p == '=') {
					count = 1;
					for (++p; p != e; ++p) {
						if (*p == '=') { ++count; }
						else { ++p; break; }
					}
					if (count > ans) { ans = count; }
				}
			}
			return ans;
		}
		static inline const string_view * _p_get_equal(int n) {
			const static constexpr string_view data_all =
				"================"
				"================"
				"================"
				"================"
				"================"
				"================"
				"================"
				"================"
				"================"
				"================"
				"================"
				"================"
				"================"
				"================"
				"================"
				"================"sv;
			const static constexpr string_view data[] = {
				data_all.substr(0,1) ,
				data_all.substr(0,2) ,
				data_all.substr(0,3) ,
				data_all.substr(0,4) ,
				data_all.substr(0,5) ,
				data_all.substr(0,6) ,
				data_all.substr(0,7) ,
				data_all.substr(0,8) ,
				data_all.substr(0,9) ,
				data_all.substr(0,10) ,
				data_all.substr(0,11) ,
				data_all.substr(0,12) ,
				data_all.substr(0,13) ,
				data_all.substr(0,14) ,
				data_all.substr(0,15) ,
				data_all.substr(0,16) ,
				data_all.substr(0,17) ,
				data_all.substr(0,18) ,
				data_all.substr(0,19) ,
				data_all.substr(0,20) ,
				data_all.substr(0,21) ,
				data_all.substr(0,22) ,
				data_all.substr(0,23) ,
				data_all.substr(0,24) ,
				data_all.substr(0,25) ,
				data_all.substr(0,26) ,
				data_all.substr(0,27) ,
				data_all.substr(0,28) ,
				data_all.substr(0,29) ,
				data_all.substr(0,30) ,
				data_all.substr(0,31) ,
				data_all.substr(0,32) ,
				data_all.substr(0,33) ,
				data_all.substr(0,34) ,
				data_all.substr(0,35) ,
				data_all.substr(0,36) ,
				data_all.substr(0,37) ,
				data_all.substr(0,38) ,
				data_all.substr(0,39) ,
				data_all.substr(0,40) ,
				data_all.substr(0,41) ,
				data_all.substr(0,42) ,
				data_all.substr(0,43) ,
				data_all.substr(0,44) ,
				data_all.substr(0,45) ,
				data_all.substr(0,46) ,
				data_all.substr(0,47) ,
				data_all.substr(0,48) ,
				data_all.substr(0,49) ,
				data_all.substr(0,50) ,
				data_all.substr(0,51) ,
				data_all.substr(0,52) ,
				data_all.substr(0,53) ,
				data_all.substr(0,54) ,
				data_all.substr(0,55) ,
				data_all.substr(0,56) ,
				data_all.substr(0,57) ,
				data_all.substr(0,58) ,
				data_all.substr(0,59) ,
				data_all.substr(0,60) ,
				data_all.substr(0,61) ,
				data_all.substr(0,62) ,
				data_all.substr(0,63) ,
				data_all.substr(0,64) ,
				data_all.substr(0,65) ,
				data_all.substr(0,66) ,
				data_all.substr(0,67) ,
				data_all.substr(0,68) ,
				data_all.substr(0,69) ,
				data_all.substr(0,70) ,
				data_all.substr(0,71) ,
				data_all.substr(0,72) ,
				data_all.substr(0,73) ,
				data_all.substr(0,74) ,
				data_all.substr(0,75) ,
				data_all.substr(0,76) ,
				data_all.substr(0,77) ,
				data_all.substr(0,78) ,
				data_all.substr(0,79) ,
				data_all.substr(0,80) ,
				data_all.substr(0,81) ,
				data_all.substr(0,82) ,
				data_all.substr(0,83) ,
				data_all.substr(0,84) ,
				data_all.substr(0,85) ,
				data_all.substr(0,86) ,
				data_all.substr(0,87) ,
				data_all.substr(0,88) ,
				data_all.substr(0,89) ,
				data_all.substr(0,90) ,
				data_all.substr(0,91) ,
				data_all.substr(0,92) ,
				data_all.substr(0,93) ,
				data_all.substr(0,94) ,
				data_all.substr(0,95) ,
				data_all.substr(0,96) ,
				data_all.substr(0,97) ,
				data_all.substr(0,98) ,
				data_all.substr(0,99) ,
				data_all.substr(0,100) ,
				data_all.substr(0,101) ,
				data_all.substr(0,102) ,
				data_all.substr(0,103) ,
				data_all.substr(0,104) ,
				data_all.substr(0,105) ,
				data_all.substr(0,106) ,
				data_all.substr(0,107) ,
				data_all.substr(0,108) ,
				data_all.substr(0,109) ,
				data_all.substr(0,110) ,
				data_all.substr(0,111) ,
				data_all.substr(0,112) ,
				data_all.substr(0,113) ,
				data_all.substr(0,114) ,
				data_all.substr(0,115) ,
				data_all.substr(0,116) ,
				data_all.substr(0,117) ,
				data_all.substr(0,118) ,
				data_all.substr(0,119) ,
				data_all.substr(0,120) ,
				data_all.substr(0,121) ,
				data_all.substr(0,122) ,
				data_all.substr(0,123) ,
				data_all.substr(0,124) ,
				data_all.substr(0,125) ,
				data_all.substr(0,126) ,
				data_all.substr(0,127) ,
				data_all.substr(0,128) ,
				data_all.substr(0,129) ,
				data_all.substr(0,130) ,
				data_all.substr(0,131) ,
				data_all.substr(0,132) ,
				data_all.substr(0,133) ,
				data_all.substr(0,134) ,
				data_all.substr(0,135) ,
				data_all.substr(0,136) ,
				data_all.substr(0,137) ,
				data_all.substr(0,138) ,
				data_all.substr(0,139) ,
				data_all.substr(0,140) ,
				data_all.substr(0,141) ,
				data_all.substr(0,142) ,
				data_all.substr(0,143) ,
				data_all.substr(0,144) ,
				data_all.substr(0,145) ,
				data_all.substr(0,146) ,
				data_all.substr(0,147) ,
				data_all.substr(0,148) ,
				data_all.substr(0,149) ,
				data_all.substr(0,150) ,
				data_all.substr(0,151) ,
				data_all.substr(0,152) ,
				data_all.substr(0,153) ,
				data_all.substr(0,154) ,
				data_all.substr(0,155) ,
				data_all.substr(0,156) ,
				data_all.substr(0,157) ,
				data_all.substr(0,158) ,
				data_all.substr(0,159) ,
				data_all.substr(0,160) ,
				data_all.substr(0,161) ,
				data_all.substr(0,162) ,
				data_all.substr(0,163) ,
				data_all.substr(0,164) ,
				data_all.substr(0,165) ,
				data_all.substr(0,166) ,
				data_all.substr(0,167) ,
				data_all.substr(0,168) ,
				data_all.substr(0,169) ,
				data_all.substr(0,170) ,
				data_all.substr(0,171) ,
				data_all.substr(0,172) ,
				data_all.substr(0,173) ,
				data_all.substr(0,174) ,
				data_all.substr(0,175) ,
				data_all.substr(0,176) ,
				data_all.substr(0,177) ,
				data_all.substr(0,178) ,
				data_all.substr(0,179) ,
				data_all.substr(0,180) ,
				data_all.substr(0,181) ,
				data_all.substr(0,182) ,
				data_all.substr(0,183) ,
				data_all.substr(0,184) ,
				data_all.substr(0,185) ,
				data_all.substr(0,186) ,
				data_all.substr(0,187) ,
				data_all.substr(0,188) ,
				data_all.substr(0,189) ,
				data_all.substr(0,190) ,
				data_all.substr(0,191) ,
				data_all.substr(0,192) ,
				data_all.substr(0,193) ,
				data_all.substr(0,194) ,
				data_all.substr(0,195) ,
				data_all.substr(0,196) ,
				data_all.substr(0,197) ,
				data_all.substr(0,198) ,
				data_all.substr(0,199) ,
				data_all.substr(0,200) ,
				data_all.substr(0,201) ,
				data_all.substr(0,202) ,
				data_all.substr(0,203) ,
				data_all.substr(0,204) ,
				data_all.substr(0,205) ,
				data_all.substr(0,206) ,
				data_all.substr(0,207) ,
				data_all.substr(0,208) ,
				data_all.substr(0,209) ,
				data_all.substr(0,210) ,
				data_all.substr(0,211) ,
				data_all.substr(0,212) ,
				data_all.substr(0,213) ,
				data_all.substr(0,214) ,
				data_all.substr(0,215) ,
				data_all.substr(0,216) ,
				data_all.substr(0,217) ,
				data_all.substr(0,218) ,
				data_all.substr(0,219) ,
				data_all.substr(0,220) ,
				data_all.substr(0,221) ,
				data_all.substr(0,222) ,
				data_all.substr(0,223) ,
				data_all.substr(0,224) ,
				data_all.substr(0,225) ,
				data_all.substr(0,226) ,
				data_all.substr(0,227) ,
				data_all.substr(0,228) ,
				data_all.substr(0,229) ,
				data_all.substr(0,230) ,
				data_all.substr(0,231) ,
				data_all.substr(0,232) ,
				data_all.substr(0,233) ,
				data_all.substr(0,234) ,
				data_all.substr(0,235) ,
				data_all.substr(0,236) ,
				data_all.substr(0,237) ,
				data_all.substr(0,238) ,
				data_all.substr(0,239) ,
				data_all.substr(0,240) ,
				data_all.substr(0,241) ,
				data_all.substr(0,242) ,
				data_all.substr(0,243) ,
				data_all.substr(0,244) ,
				data_all.substr(0,245) ,
				data_all.substr(0,246) ,
				data_all.substr(0,247) ,
				data_all.substr(0,248) ,
				data_all.substr(0,249) ,
				data_all.substr(0,250) ,
				data_all.substr(0,251) ,
				data_all.substr(0,252) ,
				data_all.substr(0,253) ,
				data_all.substr(0,254) ,
				data_all.substr(0,255) ,
				data_all.substr(0,256) ,
			};

			static_assert(std::size(data) >= max_equal_size(),
				"max_equal_size must less than data size");

			if ((--n) < max_equal_size()) {
				return &data[n];
			}
			return nullptr;
		}

		bool static inline is_simple_string(const string_view & arg) {
			/* a-z A-Z 0-9 _ */
			const static std::regex varR(u8R"!([a-zA-Z0-9_]*)!");
			return std::regex_match(arg.begin(), arg.end(), varR);
		}

		void _p_print_value_string(const string_view varData) {
			if (is_simple_string(varData)) {
				$Writer.write(u8R"(")"sv);
				$Writer.write(varData);
				$Writer.write(u8R"(")"sv);
			}
			else {
				auto varEC = _p_get_equal_count(varData) + 1;
				auto varQE = _p_get_equal(varEC);
				if (varQE) {
					$Writer.write(u8R"([)"sv);
					$Writer.write(*varQE);
					$Writer.write(u8R"([)"sv);
					$Writer.write(varData);
					$Writer.write(u8R"(])"sv);
					$Writer.write(*varQE);
					$Writer.write(u8R"(])"sv);
				}
				else {
					const auto varN = std::lldiv(varEC, max_equal_size());
					auto varQE1 = _p_get_equal(static_cast<int>(varN.rem));
					varQE = _p_get_equal(max_equal_size());
					$Writer.write(u8R"([)"sv);
					$Writer.write(*varQE1);
					for (long long i = 0; i < varN.quot; ++i) {
						$Writer.write(*varQE);
					}
					$Writer.write(u8R"([)"sv);
					$Writer.write(varData);
					$Writer.write(u8R"(])"sv);
					$Writer.write(*varQE1);
					for (long long i = 0; i < varN.quot; ++i) {
						$Writer.write(*varQE);
					}
					$Writer.write(u8R"(])"sv);
				}
			}
		}
		void print_value_string() {
			constexpr const string_view empty_string = u8R"("")"sv;
			if (lua_isstring(*this, $UserValueIndex)) {
				std::size_t n;
				auto d = lua_tolstring(*this, $UserValueIndex, &n);
				if (n > 0) {
					string_view varData{ d ,n };
					return _p_print_value_string(varData);
				}
				else {
					return $Writer.write(empty_string);
				}
			}

			/*write empty sring in any case*/
			return $Writer.write(empty_string);
		}

		void print_value_function() {
			return $Writer.write(u8R"(""function"")"sv);
		}

		void print_value_userdata() {
		}

		void print_value_thread() {
			return $Writer.write(u8R"(""thread"")"sv);
		}

		void print_value_numtags() {
			return $Writer.write(u8R"("numtags")"sv);
		}

		void print_endl() {
			$Writer.write(u8R"( ;
)"sv);
		}

		void print_table_begin() {
			$Writer.write(u8R"( {
)"sv);
		}
		void print_table_end() {
			$Writer.write(u8R"( } ;
)"sv);
		}

		void print_table(const string_view & argTableName) try {
			this->clear(argTableName);

		next_table:
			while (false == $ItemTables.empty()) {
				auto & varCurrent = *$ItemTables.rbegin();

				/*begin of a table*/
				if (varCurrent.$IsCreate) {
					print_table_begin();
				}

				varCurrent.pop(*this);
				while (lua_next(*this, $UserTableIndex)) {
					const auto varType = lua_type(*this, $UserValueIndex);
					switch (varType) {
					case  LUA_TNONE: {/*跳过none*/ break; }break;
					case  LUA_TNIL: { /*跳过nil*/break; } break;
					case  LUA_TBOOLEAN: {
						if (false == this->print_name(&varCurrent))break;
						if (false == varCurrent.$TableArrayContinue) this->print_equal();
						this->print_value_bool();
						this->print_endl();
					}break;
					case  LUA_TLIGHTUSERDATA: {
						/*跳过 light user data*/ break;
						if (false == this->print_name(&varCurrent))break;
						if (false == varCurrent.$TableArrayContinue)this->print_equal();
						this->print_value_lightuserdata();
						this->print_endl();
					}break;
					case  LUA_TNUMBER: {
						if (false == this->print_name(&varCurrent))break;
						if (false == varCurrent.$TableArrayContinue)this->print_equal();
						this->print_value_number();
						this->print_endl();
					}break;
					case  LUA_TSTRING: {
						if (false == this->print_name(&varCurrent))break;
						if (false == varCurrent.$TableArrayContinue)this->print_equal();
						this->print_value_string();
						this->print_endl();
					}break;
					case  LUA_TTABLE: {
						$ItemTables.emplace_back(&varCurrent, ++$CurrentTableIndex).push<true>(*this);
						varCurrent.push<false>(*this);
						goto next_table;
					}break;
					case  LUA_TFUNCTION: {
						/*跳过 function*/ break;
						if (false == this->print_name(&varCurrent))break;
						if (false == varCurrent.$TableArrayContinue)this->print_equal();
						this->print_value_function();
						this->print_endl();
					}break;
					case  LUA_TUSERDATA: {
						/*跳过 user data*/ break;
						if (false == this->print_name(&varCurrent))break;
						if (false == varCurrent.$TableArrayContinue)this->print_equal();
						this->print_value_userdata();
						this->print_endl();
					}break;
					case  LUA_TTHREAD: {
						/*跳过 thread*/ break;
						if (false == this->print_name(&varCurrent))break;
						if (false == varCurrent.$TableArrayContinue)this->print_equal();
						this->print_value_thread();
						this->print_endl();
					}break;
					case  LUA_NUMTAGS: {
						/*跳过 numtags*/ break;
						if (false == this->print_name(&varCurrent))break;
						if (false == varCurrent.$TableArrayContinue)this->print_equal();
						this->print_value_numtags();
						this->print_endl();
					}break;
					}/*switch*/
					lua_settop(*this, $UserKeyIndex);
				}/*whihle*/

				{/*End of a table*/
					print_table_end();
					--$CurrentTableIndex;
					$ItemTables.rbegin()->destory(*this);
					$ItemTables.pop_back();
				}

			}/*while*/

		}
		catch (const LuaCplusplusException &e) {
			throw e;
		}
		catch (...) {/*意外的异常转化为lua异常*/
			this->error(u8R"(a unlua exception catched @ lua_table_to_text)"sv);
		}

		operator lua_State * () const { return $L; }
		lua_State * operator->() const { return $L; }
		lua_State & operator*() const { return *$L; }

		LuaLock(const LuaLock &) = delete;
		LuaLock(LuaLock &&) = delete;
		LuaLock&operator=(const LuaLock &) = delete;
		LuaLock&operator=(LuaLock &&) = delete;
	};

	static inline void lua_table_to_text(lua_State * argL) {
	}

}/*namespace*/




