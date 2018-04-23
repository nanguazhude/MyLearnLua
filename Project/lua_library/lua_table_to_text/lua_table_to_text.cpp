#ifndef LUA_LIB
#define LUA_LIB std::size_t(1)
#endif

//#define QUICK_CHECK_CODE std::size_t(1)
//#define DEBUG_THIS_LIB
#ifndef NDEBUG
#define NDEBUG
#endif

#include "../lua.hpp"
#include "../part_google_v8/include/double-conversion/double-conversion.h"

#include <set>
#include <map>
#include <list>
#include <vector>
#include <array>
#include <memory>
#include <string>
#include <cstddef>
#include <utility>
#include <optional>
#include <string_view>
#include <type_traits>
#include <memory_resource>
/***************************/
#include <fstream>
#include <iostream>
/**************************/
#include <cassert>

using namespace std::string_view_literals;

namespace {

	using string_view = std::string_view;
	/**************************************************************/
	/* change here to change how to use memory */
	using string = std::string;// std::pmr::string;
	template<typename T> using vector = std::vector<T>;
	template<typename T> using list = std::list<T>;// std::pmr::list;
	template<typename T> using unique_ptr = std::unique_ptr<T>;
	template<typename T, typename C = std::less<void>/**/> using set = std::set<T, C>;
	template<typename T, typename U, typename C = std::less<void>/**/> using map = std::map<T, U, C>;
	using std::make_unique;
	/**************************************************************/

	/* Lua栈区最小值 */
	constexpr int inline concept_stack_min_size() { return 16; }
	/* 临时表最小大小 */
	constexpr int inline concept_min_tmp_table_size() { return 128; }
	/* 临时缓存区大小 */
	constexpr int inline concept_tmp_buffer_size() { return 256; }
	/* 连续'='的最大优化值 */
	constexpr int inline max_equal_size() { return 256; }

	template<bool> class CheckCircleTableData;
#if defined(QUICK_CHECK_CODE)
	class LuaLock;
#else
	/*else*/
#endif

#if defined(DEBUG_THIS_LIB)
	inline string_view debug_type_name(int i) {
		switch (i) {
		case LUA_TNONE:return "LUA_TNONE"sv;
		case LUA_TNIL:return "LUA_TNIL"sv;
		case LUA_TBOOLEAN:return "LUA_TBOOLEAN"sv;
		case LUA_TLIGHTUSERDATA:return "LUA_TLIGHTUSERDATA"sv;
		case LUA_TNUMBER:return "LUA_TNUMBER"sv;
		case LUA_TSTRING:return "LUA_TSTRING"sv;
		case LUA_TTABLE:return "LUA_TTABLE"sv;
		case LUA_TFUNCTION:return "LUA_TFUNCTION"sv;
		case LUA_TUSERDATA:return "LUA_TUSERDATA"sv;
		case LUA_TTHREAD:return "LUA_TTHREAD"sv;
		}
		return "unknow"sv;
	}

	static string debug_string;
	inline string_view debug_type_value(lua_State * L, int i) {
		const auto t = lua_type(L, i);
		debug_string = "[===["sv;
		debug_string += debug_type_name(t);
		debug_string += " : "sv;
		switch (t) {
		case LUA_TNONE:debug_string += "LUA_TNONE"sv; break;
		case LUA_TNIL:debug_string += "LUA_TNIL"sv; break;
		case LUA_TBOOLEAN: debug_string += (lua_toboolean(L, i) ? "true"sv : "false"sv); break;
		case LUA_TLIGHTUSERDATA:debug_string += "LUA_TLIGHTUSERDATA"sv; break;
		case LUA_TNUMBER: {
			if (lua_isinteger(L, i)) {
				debug_string += std::to_string(lua_tointeger(L, i));
			}
			else {
				debug_string += std::to_string(lua_tonumber(L, i));
			}
		}break;
		case LUA_TSTRING: {
			std::size_t n = 0;
			const auto d = luaL_tolstring(L, i, &n);
			debug_string += string_view(d, n);
		}break;
		case LUA_TTABLE:debug_string += "LUA_TTABLE"sv; break;
		case LUA_TFUNCTION:debug_string += "LUA_TFUNCTION"sv; break;
		case LUA_TUSERDATA:debug_string += "LUA_TUSERDATA"sv; break;
		case LUA_TTHREAD:debug_string += "LUA_TTHREAD"sv; break;
		}
		debug_string += "]===]"sv;
		return debug_string;
	}
#endif

	template<>
	class CheckCircleTableData<true> {
	public:
		const constexpr static bool value = true;
		string_view $real_root_table_name;
		list<string> $catch_print_able_name/*缓冲所有名字*/;
		set<const void *> $about_circle_tables;
		list<std::pair<const string_view, const string_view>/**/> $string_about_to_write;
		class TableDetail {
		public:
			string_view print_able_name;
		};
		map<const void *, TableDetail> $Tables;
		const string_view $this_table_name = u8R"(tmp)"sv;
		inline const string_view & get_this_table_name() const {
			return $this_table_name;
		}
		inline bool hasTable(const void * const arg) const { return $Tables.count(arg) > 0; }
		inline auto find(const void * const arg) const {
			auto varPos = $Tables.find(arg);
			return std::make_pair(varPos, !(varPos == $Tables.end()));
		}
		inline void parent_clear() {
			$catch_print_able_name.clear();
			$Tables.clear();
			$about_circle_tables.clear();
		}
#if defined(QUICK_CHECK_CODE)
		template<typename TableItem>
		inline void insert(LuaLock*, TableItem *);
		void begin(const string_view &, LuaLock*);
		void end(LuaLock*);
#else
		template<typename LuaLock, typename TableItem>
		inline void insert(LuaLock*, TableItem *);
		template<typename LuaLock>
		void begin(const string_view &, LuaLock*);
		template<typename LuaLock>
		void end(LuaLock*);
#endif

	};

	template<>
	class CheckCircleTableData<false> {
	public:
		const constexpr static bool value = false;
		set<const void *> $Tables;
		inline bool hasTable(const void * const arg) const { return $Tables.count(arg) > 0; }
		inline auto find(const void * const arg) const {
			auto varPos = $Tables.find(arg);
			return std::make_pair(varPos, !(varPos == $Tables.end()));
		}
		inline void parent_clear() { $Tables.clear(); }
		const string_view & get_this_table_name() const {
			static string_view ans = ""sv;
			/*never used*/return ans;
		}
#if defined(QUICK_CHECK_CODE)
		template<typename TableItem>
		inline void insert(LuaLock*, TableItem *);
		void begin(const string_view &, LuaLock*) {}
		void end(LuaLock*) {}
#else
		template<typename LuaLock, typename TableItem>
		inline void insert(LuaLock*, TableItem *);
		template<typename LuaLock>
		void begin(const string_view &, LuaLock*) {/*just do nothing*/ }
		template<typename LuaLock>
		void end(LuaLock*) {/*just do nothing*/ }
#endif

	};

#if defined(QUICK_CHECK_CODE)
	class WType {
	public:
		void inline write(const string_view &a) { std::cout << a; }
		const constexpr static bool value = false;
	};
#else
	template<typename WType/* WType w; w.write(const string_view &); */>
#endif
	class LuaLock :public CheckCircleTableData<WType::value> {
		/*prvate:memory:new&delete*/
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
			/*
			1:key
			2:table
			3:table name
			*/
		public:
			TableItem * const $Parent;
			int const $IndexInTmpTable;
			int  $LastIndex = 1/*用于判断Array是否连续*/;
			bool $TableArrayContinue = true;
			bool $IsCreate = true;
			bool $IsRootTableNameNull = false;
			bool $IsCircleTable = false;
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

#if defined(DEBUG_THIS_LIB)
					lua_rawgeti(arg, varThisTableIndex, 1);
					std::cout << "+=========="sv << std::endl;
					std::cout << debug_type_value(arg, lua_gettop(arg)) << std::endl;
					lua_rawgeti(arg, varThisTableIndex, 2);
					std::cout << debug_type_value(arg, lua_gettop(arg)) << std::endl;
					lua_rawgeti(arg, varThisTableIndex, 3);
					std::cout << debug_type_value(arg, lua_gettop(arg)) << std::endl;
					std::cout << "+=========="sv << std::endl;
#endif

					/*移除不用的值*/
					lua_settop(arg, arg.$UserKeyIndex);
				}
				else {
					$IsCreate = false;
					lua_settop(arg, arg.$UserKeyIndex);
					lua_rawgeti(arg, arg.$TmpTableIndex, $IndexInTmpTable);
					const auto varThisTableIndex = arg.$UserKeyIndex + 1;
					/*
					table key
					table
					*/
					{
						/* item.key */
						lua_pushvalue(arg, arg.$UserKeyIndex);
						lua_rawseti(arg, varThisTableIndex, 1);
					}
#if defined(DEBUG_THIS_LIB)
					lua_rawgeti(arg, varThisTableIndex, 1);
					std::cout << "==========="sv << std::endl;
					std::cout << debug_type_value(arg, lua_gettop(arg)) << std::endl;
					lua_rawgeti(arg, varThisTableIndex, 2);
					std::cout << debug_type_value(arg, lua_gettop(arg)) << std::endl;
					lua_rawgeti(arg, varThisTableIndex, 3);
					std::cout << debug_type_value(arg, lua_gettop(arg)) << std::endl;
					std::cout << "==========="sv << std::endl;
#endif
					lua_settop(arg, arg.$UserKeyIndex);
				}

			}

			inline void pop(const LuaLock & arg) {
				/*清除不用的值,保证top不小于$UserKeyIndex*/
				lua_settop(arg, arg.$UserValueIndex);
				lua_rawgeti(arg, arg.$TmpTableIndex, $IndexInTmpTable);
				const auto varThisTableIndex = lua_gettop(arg);
#if defined(DEBUG_THIS_LIB)
				std::cout << debug_type_value(arg, -1) << std::endl;
#endif
				/*key*/
				lua_rawgeti(arg, varThisTableIndex, 1);
#if defined(DEBUG_THIS_LIB)
				std::cout << debug_type_value(arg, -1) << std::endl;
#endif
				lua_copy(arg, -1, arg.$UserKeyIndex);
				/*table*/
				lua_rawgeti(arg, varThisTableIndex, 2);
#if defined(DEBUG_THIS_LIB)
				std::cout << debug_type_value(arg, -1) << std::endl;
#endif
				lua_copy(arg, -1, arg.$UserTableIndex);
				/*移除不用的值*/
				lua_settop(arg, arg.$UserKeyIndex);
			}
		};

		list< TableItem > $ItemTables;

		string __p_get_afull_table_name(TableItem * arg) {
			int varTryCount = 0;
			/*get tmp table*/
			lua_rawgeti(*this, this->$TmpTableIndex, arg->$IndexInTmpTable);
			/*get name*/
			lua_rawgeti(*this, -1, 3);
			/*the name index*/
			auto varNameIndex = lua_gettop(*this);
			/*convert the name to string*/
			/*the name should be simple*/
		try_next:
			++varTryCount;
			const auto varType = lua_type(*this, varNameIndex);
			switch (varType) {
			case LUA_TNONE: {}break;
			case LUA_TNIL: {}break;
			case LUA_TBOOLEAN: {}break;
			case LUA_TLIGHTUSERDATA: {}break;
			case LUA_TNUMBER: {
				if (lua_isinteger(*this, varNameIndex)) {
					const auto varAns1 = this->int_to_string(lua_tointeger(*this, varNameIndex));
					if (varAns1.empty()) { return {}; }
					string varAns2;
					varAns2.reserve(2 + varAns1.size());
					varAns2 += "["sv;
					varAns2 += varAns1;
					varAns2 += "]"sv;
					return std::move(varAns2);
				}
				else {
					/*
					double to index is ill format 
					this may be a error
					*/
					const auto varAns1 = this->double_to_string(lua_tonumber(*this, varNameIndex));
					if (varAns1.empty()) { return {}; }
					string varAns2;
					varAns2.reserve(2 + varAns1.size());
					varAns2 += "["sv;
					varAns2 += varAns1;
					varAns2 += "]"sv;
					return std::move(varAns2);
				}
			}break;
			case LUA_TSTRING: {
				std::size_t n = 0;
				const auto d = lua_tolstring(*this, varNameIndex, &n);
				if (n < 1) { return {}; }
				const string_view varAns1{ d,n };
				if (this->is_simple_string(varAns1)) {
					string varAns2;
					varAns2.reserve(4 + varAns1.size());
					varAns2 += u8R"([")"sv;
					varAns2 += varAns1;
					varAns2 += u8R"("])"sv;
					return std::move(varAns2);
				}
				/*this may be a illformat name*/
				const auto varEC = 1 + this->_p_get_equal_count(varAns1);
				string varAns2;
				varAns2.reserve(/*[ [=[*/ ((4 + varEC) << 1) + varAns1.size());

				auto varAddEquals = [varEC](string & ans) {
					auto varECS = _p_get_equal(varEC);
					if (varECS) {
						ans += *varECS;
					}
					constexpr const auto varMES = max_equal_size();
					auto varDIV = std::div(varEC, varMES);
					varECS = _p_get_equal(varDIV.rem);
					assert(varECS);
					ans += *varECS;
					varECS = _p_get_equal(varMES);
					assert(varECS);
					for (int i = 0; i < varDIV.quot; ++i) {
						ans += *varECS;
					}
				};

				varAns2 += u8R"([ [)"sv;
				varAddEquals(varAns2);
				varAns2 += u8R"([)"sv;
				varAns2 += varAns1;
				varAns2 += u8R"(])"sv;
				varAddEquals(varAns2);
				varAns2 += u8R"(] ])"sv;

				return std::move(varAns2);
			}break;
			case LUA_TTABLE: {}break;
			case LUA_TFUNCTION: {}break;
			case LUA_TUSERDATA: {}break;
			case LUA_TTHREAD: {}break;
			}

			if (varTryCount > 1) { return{}; }

			/*try __tostring*/
			std::size_t n = 0;
			luaL_tolstring(*this, varNameIndex, &n);
			if (n > 0) {
				varNameIndex = lua_gettop(*this);
				goto try_next;
			}
			/*****************************************/
			return {};
		}

		string get_full_table_name(TableItem*arg) {
			/*如果是Root，则返回tmp*/
			if ((arg->$Parent)==nullptr) {
				return { this->get_this_table_name().data(),this->get_this_table_name().size() };
			}

			vector< string > varAns;
			varAns.reserve(this->$ItemTables.size());
			{
				TableItem * varI = arg;
				const auto varTop = lua_gettop(*this);
				/*跳过首个表...*/
				while (varI&&varI->$Parent) {
					auto var = varI;
					varI = var->$Parent;
					varAns.push_back(__p_get_afull_table_name(var));
					lua_settop(*this, varTop)/*clean the value do not used*/;
				}
			}
			const auto & varRootName = this->get_this_table_name();
			std::size_t n = varRootName.size();
			assert(std::as_const(n));
			for (const auto & varI : varAns) {
				n += varI.size();
			}
			string ans;
			ans.reserve(n);
			ans = varRootName;

			{
				const auto varE = varAns.rend();
				auto varI = varAns.rbegin();
				for (; varI != varE; ++varI) {
					ans += std::move(*varI);
				}
				varAns.clear();
			}

			return std::move(ans);
		}

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
			return { $TmpStringBuffer.data(),static_cast<std::size_t>(varSB.position()) };
		}

		/* 将字符串加载到Lua栈 */
		inline void push_string(const string_view & s) {
			lua_pushlstring($L, s.data(), s.size());
		}

		template<typename U>
		inline LuaLock(lua_State * arg, U&&w) :$L(arg), $Writer(std::forward<U>(w)) {
			/* 保证最小栈区 */
			if constexpr(this->value) {
				lua_checkstack($L, concept_stack_min_size() + 8);
			}
			else {
				lua_checkstack($L, concept_stack_min_size());
			}

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
			parent_clear();
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
			lua_pushvalue($L, $TableIndex)/*the root table*/;
			if constexpr(this->value) {/*如果是完全表,输出tmp*/
				push_string(get_this_table_name());
			}
			else {
				push_string(argTableName)/*the table name*/;
			}
			lua_pushvalue($L, $TableIndex)/*this will not be used*/;
			auto & varI = $ItemTables.emplace_back(nullptr, ++$CurrentTableIndex);
			varI.push<true>(*this);
			varI.$IsRootTableNameNull = argTableName.empty();
			this->insert(this, &varI);
		}

		enum class PrintNameType : int {
			TableName,
			ValueName
		};

		template<PrintNameType NType = PrintNameType::ValueName>
		bool print_name(TableItem*arg) {

			if constexpr(NType == PrintNameType::ValueName) {/*just print value name*/
				const auto varType = lua_type(*this, $UserKeyIndex);

				if (LUA_TNUMBER == varType) {
					int n;
					if (lua_isinteger(*this, $UserKeyIndex)) {
						auto d = lua_tointegerx(*this, $UserKeyIndex, &n);
						if (arg->$TableArrayContinue) {
							if (d == arg->$LastIndex) {
								++(arg->$LastIndex);
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
						/*
						double to index is ill format
						may be a error
						*/
						arg->$TableArrayContinue = false;
						auto d = lua_tonumberx(*this, $UserKeyIndex, &n);
						$Writer.write(u8R"([)"sv);
						return ($Writer.write(double_to_string(d)), $Writer.write(u8R"(])"sv), true);
					}
				}

				arg->$TableArrayContinue = false;
				std::size_t n = 0;
				const char * d = nullptr;

				if ( lua_type(*this, $UserKeyIndex) == LUA_TSTRING ) {
					d = lua_tolstring(*this, $UserKeyIndex, &n);
				}
				else {
					d = luaL_tolstring(*this, $UserKeyIndex, &n);
				}
	 
				if (n > 0) {
					this->_p_print_value_string<false>({ d,n });
					return true;
				}
				else {
					return false;
				}

			}
			else {/*just print table name*/
				const auto RPos /*return position*/ = lua_gettop(*this);
#undef ReturnInThisFunction
#define ReturnInThisFunction /*space*/lua_settop(*this,RPos);return/*space*/

				lua_rawgeti(*this, this->$TmpTableIndex, arg->$IndexInTmpTable);
				lua_rawgeti(*this, -1, 3);
				const auto varTableNamePos /*table name position*/ = lua_gettop(*this);
				const auto varType = lua_type(*this, varTableNamePos);

				if (LUA_TNIL == varType) {
					ReturnInThisFunction true;
				}

				if (LUA_TNUMBER == varType) {
					int n;
					if (lua_isinteger(*this, varTableNamePos)) {
						auto d = lua_tointegerx(*this, varTableNamePos, &n);
						if (arg->$Parent == nullptr) {
							this->error(u8R"(Root Table Can Not Use Number To Key)"sv);
						}
						if (arg->$Parent->$TableArrayContinue) {
							if (d == arg->$Parent->$LastIndex) {
								++(arg->$Parent->$LastIndex);
								return/*连续table不输出变量名*/true;
							}
							else {
								arg->$Parent->$TableArrayContinue = false;
							}
						}
						$Writer.write(u8R"([)"sv);
						ReturnInThisFunction($Writer.write(int_to_string(d)), $Writer.write(u8R"(])"sv), true);
					}
					else {
						/*
						double to index is ill format
						may be a error
						*/
						if (arg->$Parent) {
							arg->$Parent->$TableArrayContinue = false;
						}
						auto d = lua_tonumberx(*this, varTableNamePos, &n);
						$Writer.write(u8R"([)"sv);
						return ($Writer.write(double_to_string(d)), $Writer.write(u8R"(])"sv), true);
					}
				}

				if (arg->$Parent) {
					arg->$Parent->$TableArrayContinue = false;
				}

				std::size_t n = 0;
				const char * d = nullptr;

				if (lua_type(*this, varTableNamePos) == LUA_TSTRING) {
					d = lua_tolstring(*this, varTableNamePos, &n);
				}
				else {
					d = luaL_tolstring(*this, varTableNamePos, &n);
				}

				if (n > 0) {
					this->_p_print_value_string<false>({ d,n });
					ReturnInThisFunction true;
				}
				else {
					ReturnInThisFunction false;
				}

				/**/ReturnInThisFunction false;
#undef ReturnInThisFunction
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
			/*empty string return true*/
			if (arg.empty()) { return true; }
			/*if start with number return false*/
			if (const auto varI = arg[0]; ((varI <= '9') && (varI >= '0'))) {
				return false;
			}
			/* a-z A-Z 0-9 _ */
			for (const auto & varI : arg) {
				if (varI & 0b010000000) {/*最高位是1肯定不符合要求...*/
					return false;
				}
				if ((varI == '_') ||
					((varI <= 'z') && (varI >= 'a')) ||
					((varI <= 'Z') && (varI >= 'A')) ||
					((varI <= '9') && (varI >= '0')) ||
					false
					) {
					continue;
				}
				else {
					return false;
				}
			}
			return true;
		}

		template<bool Vxx = true>
		void _p_print_value_string(const string_view varData) {
			if (is_simple_string(varData)) {
				if constexpr(Vxx == true) { $Writer.write(u8R"(")"sv); }
				$Writer.write(varData);
				if constexpr(Vxx == true) { $Writer.write(u8R"(")"sv); }
			}
			else {
				auto varEC = _p_get_equal_count(varData) + 1;
				auto varQE = _p_get_equal(varEC);
				if (varQE) {
					if constexpr(Vxx == true) { $Writer.write(u8R"([)"sv); }
					else { $Writer.write(u8R"([ [)"sv); }
					$Writer.write(*varQE);
					$Writer.write(u8R"([)"sv);
					$Writer.write(varData);
					$Writer.write(u8R"(])"sv);
					$Writer.write(*varQE);
					if constexpr(Vxx == true) { $Writer.write(u8R"(])"sv); }
					else { $Writer.write(u8R"(] ])"sv); }
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
				std::size_t n = 0;
				auto d = lua_tolstring(*this, $UserValueIndex, &n);
				if (n > 0) {
					string_view varData{ d ,n };
					return this->_p_print_value_string<true>(varData);
				}
				else {
					return $Writer.write(empty_string);
				}
			}
			else {
				std::size_t n = 0;
				auto d = luaL_tolstring(*this, $UserValueIndex, &n);
				if (n > 0) {
					const string_view varData{ d ,n };
					return this->_p_print_value_string<true>(varData);
				}
				else {
					return $Writer.write(empty_string);
				}
			}

			/*write empty sring in any case*/
			return $Writer.write(empty_string);
		}

		void print_value_function() {
			return $Writer.write(u8R"("function")"sv);
		}

		void print_value_userdata() {
			return $Writer.write(u8R"("userdata")"sv);
		}

		void print_value_thread() {
			return $Writer.write(u8R"("thread")"sv);
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

			auto pop_a_table = [this]() {
				--$CurrentTableIndex;
				$ItemTables.rbegin()->destory(*this);
				$ItemTables.pop_back();
			};

			if constexpr(this->value) {
				this->begin(argTableName, this);
				(*$ItemTables.rbegin()).$IsRootTableNameNull = false;
			};

		next_table:
			while (false == $ItemTables.empty()) {
				auto & varCurrent = *$ItemTables.rbegin();

				/*begin of a table*/
				if (varCurrent.$IsCreate) {

					varCurrent.$IsCreate = false;
					const bool & varIsRootNullName = varCurrent.$IsRootTableNameNull;

					if (false == varIsRootNullName) {
						if (false == print_name<PrintNameType::TableName>(&varCurrent)) {
							/* ??? */
							pop_a_table();
							continue;
						}
						if (varCurrent.$Parent) {
							if (false == varCurrent.$Parent->$TableArrayContinue) {
								this->print_equal();
							}
						}
						else {
							/*we have print table name , then print equal */
							this->print_equal();
						}
					}

					print_table_begin();
				}

				if (varCurrent.$IsCircleTable) {
					print_table_end();
					pop_a_table();
					continue;
				}

				varCurrent.pop(*this);
				while (lua_next(*this, $UserTableIndex)) {
					auto varType = lua_type(*this, $UserValueIndex);
					/**********************************************/
					do {
						/*如果是可序列化类型,则继续...*/
						if ((varType == LUA_TBOOLEAN) ||
							(varType == LUA_TNUMBER) ||
							(varType == LUA_TSTRING) ||
							(varType == LUA_TTABLE)
							) {
							break;
						}
						const auto varTop = lua_gettop(*this);
						/*then try __tostring*/
						if (luaL_callmeta(*this, this->$UserValueIndex, "__tostring") &&
							(lua_type(*this, -1) == LUA_TSTRING)) {
							varType = LUA_TSTRING;
							lua_copy(*this, -1, this->$UserValueIndex);
						}
						/*remove the data do not used*/
						lua_settop(*this, varTop);
					} while (false);
					/**********************************************/
					switch (varType) {
					case  LUA_TNONE: {/*跳过none*/ break; }break;
					case  LUA_TNIL: { /*跳过nil*/  break; } break;
					case  LUA_TBOOLEAN: {
						if (false == this->print_name(&varCurrent)) break;
						if (false == varCurrent.$TableArrayContinue) this->print_equal();
						this->print_value_bool();
						//this->$Writer.write(u8R"( --[[ bool --]] )"sv);
						this->print_endl();
					}break;
					case  LUA_TLIGHTUSERDATA: {
						/*跳过 light user data*/ // break;
						if (false == this->print_name(&varCurrent))break;
						if (false == varCurrent.$TableArrayContinue)this->print_equal();
						this->print_value_lightuserdata();
						//this->$Writer.write(u8R"( --[[ lightuserdata --]] )"sv);
						this->print_endl();
					}break;
					case  LUA_TNUMBER: {
						if (false == this->print_name(&varCurrent))break;
						if (false == varCurrent.$TableArrayContinue)this->print_equal();
						this->print_value_number();
						//this->$Writer.write(u8R"( --[[ number --]] )"sv);
						this->print_endl();
					}break;
					case  LUA_TSTRING: {
						if (false == this->print_name(&varCurrent))break;
						if (false == varCurrent.$TableArrayContinue)this->print_equal();
						this->print_value_string();
						//this->$Writer.write(u8R"( --[[ string --]] )"sv);
						this->print_endl();
					}break;
					case  LUA_TTABLE: {
						auto & varITT = $ItemTables.emplace_back(&varCurrent, ++$CurrentTableIndex);
						varITT.push<true>(*this);
						varCurrent.push<false>(*this);
						this->insert(this, &varITT);
						goto next_table;
					}break;
					case  LUA_TFUNCTION: {
						/*跳过 function*/ // break;
						if (false == this->print_name(&varCurrent))break;
						if (false == varCurrent.$TableArrayContinue)this->print_equal();
						this->print_value_function();
						//this->$Writer.write(u8R"( --[[ function --]] )"sv);
						this->print_endl();
					}break;
					case  LUA_TUSERDATA: {
						/*跳过 user data*/ // break;
						if (false == this->print_name(&varCurrent))break;
						if (false == varCurrent.$TableArrayContinue)this->print_equal();
						this->print_value_userdata();
						//this->$Writer.write(u8R"( --[[ userdata --]] )"sv);
						this->print_endl();
					}break;
					case  LUA_TTHREAD: {
						/*跳过 thread*/ // break;
						if (false == this->print_name(&varCurrent))break;
						if (false == varCurrent.$TableArrayContinue)this->print_equal();
						this->print_value_thread();
						//this->$Writer.write(u8R"( --[[ thread --]] )"sv);
						this->print_endl();
					}break;
					case  LUA_NUMTAGS: {
						/*跳过 numtags*/ // break;
						if (false == this->print_name(&varCurrent))break;
						if (false == varCurrent.$TableArrayContinue)this->print_equal();
						this->print_value_numtags();
						//this->$Writer.write(u8R"( --[[ numtags --]] )"sv);
						this->print_endl();
					}break;
					}/*switch*/
					lua_settop(*this, $UserKeyIndex);
				}/*whihle*/

				{/*End of a table*/
					print_table_end();
					pop_a_table();
				}

			}/*while*/

			if constexpr(this->value) this->end(this);

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

#if defined(QUICK_CHECK_CODE)
	template<typename TableItem>
#else
	template<typename LuaLock, typename TableItem>
#endif
	void CheckCircleTableData<false>::insert(LuaLock*L, TableItem *arg) {
		const auto varTop = lua_gettop(*L);
		lua_rawgeti(*L, L->$TmpTableIndex, arg->$IndexInTmpTable);
		lua_rawgeti(*L, -1, 2);
		assert(lua_istable(*L, -1) && "there must a table in the top");
		const auto varTablePointer = lua_topointer(*L, -1);
		lua_settop(*L, varTop)/*remove the data do not used*/;
		arg->$IsCircleTable = this->hasTable(varTablePointer);
		if (arg->$IsCircleTable == false)
			this->$Tables.insert(varTablePointer);
	}

#if defined(QUICK_CHECK_CODE)
	template<typename TableItem>
#else
	template<typename LuaLock, typename TableItem>
#endif
	void CheckCircleTableData<true>::insert(LuaLock*L, TableItem *arg) {
		const auto varTop = lua_gettop(*L);
		lua_rawgeti(*L, L->$TmpTableIndex, arg->$IndexInTmpTable);
		lua_rawgeti(*L, -1, 2);
		assert(lua_istable(*L, -1) && "there must a table in the top");
		const auto varTablePointer = lua_topointer(*L, -1);
		const auto varPPos = this->$Tables.find(varTablePointer);
		const bool varIsHasTable = (varPPos != this->$Tables.end());
		lua_settop(*L, varTop)/*remove the data do not used*/;
		if (varIsHasTable) {
			arg->$IsCircleTable = true;
			this->$catch_print_able_name.push_back(L->get_full_table_name(arg));
			const string_view varThisTableFullName = *(this->$catch_print_able_name.rbegin());
			this->$string_about_to_write.emplace_back(
				varThisTableFullName,
				varPPos->second.print_able_name);
		}
		else {
			arg->$IsCircleTable = false;
			auto varTPos = this->$Tables.emplace(varTablePointer, TableDetail{});
			/*如果被环表引用...*/
			if (this->$about_circle_tables.count(varTablePointer) > 0) {
				TableDetail & varDetail = varTPos.first->second;
				this->$catch_print_able_name.push_back(L->get_full_table_name(arg));
				varDetail.print_able_name = *(this->$catch_print_able_name.rbegin());
			}
		}
	}

#if defined(QUICK_CHECK_CODE)
#else
	template<typename LuaLock >
#endif
	void CheckCircleTableData<true>::begin(const string_view & argRTName, LuaLock*L) {
		{/*保存真实表的名称*/
			this->$real_root_table_name = argRTName;
			L->$Writer.write(u8R"(local )"sv);
		}
		/*遍历整个表，记录下要索引表的名称*/
		const auto RPos = lua_gettop(*L);
#undef ReturnInThisFunction
#define ReturnInThisFunction /*space*/lua_settop(*L,RPos);return/*space*/

		/*构建索引表*/
		class Item {
		public:
			const Item * const parent;
			const void * const data;

			class Compare {
			public:
				typedef int is_transparent;

				bool operator()(const Item & l, const Item &r) const { return l.data < r.data; }
				bool operator()(const void * l, const Item &r) const { return l < r.data; }
				bool operator()(const Item & l, const void * r) const { return l.data < r; }
			};

			Item(const Item * p, const void *d) :parent(p), data(d) {}

		};

		class Pack {
		public:
			lua_State * L;
			int $TmpTableIndex;
			int $UserKeyIndex;
			int $UserTableIndex;
			operator lua_State *() const { return L; }
			Pack(lua_State *a) :L(a) {}
		};

		class StackItem {
		public:
			int $IndexInTmpTable;
			const Item * const $TablePoiner;
			StackItem(const Item * p, int k) : $IndexInTmpTable(k), $TablePoiner(p) {}

			inline void destory(const Pack & arg) {
				lua_pushnil(arg);
				lua_rawseti(arg, arg.$TmpTableIndex, $IndexInTmpTable);
			}

			inline void push(const Pack & arg) {
				//$IsCreate = false;
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

			/*
			1 : key
			2 : table 
			*/
			inline void push_create(const Pack & arg) {

				/*创建索引表*/
				lua_createtable(arg, 2, 0);
				const auto varThisTableIndex = lua_gettop(arg);
				{
					/* value */
					lua_pushvalue(arg, varThisTableIndex);
					/* set tmp[IndexInTmpTable] = value */
					lua_rawseti(arg, arg.$TmpTableIndex, $IndexInTmpTable);
				}
				/*
				index table  索引表
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

				/*移除不用的值*/
				lua_settop(arg, arg.$UserKeyIndex);

			}

			inline void pop(const Pack & arg) {
				/*移除不用的值,确保堆栈大小*/
				lua_settop(arg, arg.$UserKeyIndex);
				lua_rawgeti(arg, arg.$TmpTableIndex, $IndexInTmpTable);
				const auto varThisTableIndex = lua_gettop(arg);
				/*key*/
				lua_rawgeti(arg, varThisTableIndex, 1);
				lua_copy(arg, -1, arg.$UserKeyIndex);
				/*table*/
				lua_rawgeti(arg, varThisTableIndex, 2);
				lua_copy(arg, -1, arg.$UserTableIndex);
				/************************************************/
#if defined(DEBUG_THIS_LIB)
				lua_rawgeti(arg, varThisTableIndex, 1);
				std::cout << "+~========="sv << std::endl;
				std::cout << debug_type_value(arg, lua_gettop(arg)) << std::endl;
				lua_rawgeti(arg, varThisTableIndex, 2);
				std::cout << debug_type_value(arg, lua_gettop(arg)) << std::endl;
				std::cout << "+~========="sv << std::endl;
#endif
				/************************************************/
				/*移除不用的值*/
				lua_settop(arg, arg.$UserKeyIndex);
			}

		};

		Pack varPack{ *L };

		lua_createtable(*L, concept_min_tmp_table_size(),0);
		varPack.$TmpTableIndex = lua_gettop(*L);

		lua_pushvalue(*L, L->$TableIndex);
		varPack.$UserTableIndex = lua_gettop(*L);

		lua_pushnil(*L);
		varPack.$UserKeyIndex = varPack.$UserTableIndex + 1;
		const auto varValueIndex = varPack.$UserTableIndex + 2;

		set<Item, Item::Compare> varAllTables;
		list<StackItem> varStack;

		lua_pushvalue(*L, L->$TableIndex);
		lua_pushvalue(*L, L->$TableIndex);
		lua_pushvalue(*L, L->$TableIndex);

		int varCurrentIndex = 1;
		{
			auto varTP = lua_topointer(*L, -1);
			varStack.emplace_back(&(*varAllTables.emplace(nullptr, varTP).first), varCurrentIndex).push_create(varPack);
		}

	next_table:
		while (false == varStack.empty()) {
			auto & varI = *varStack.rbegin();
			varI.pop(varPack);

			while (lua_next(varPack, varPack.$UserTableIndex)) {
				const auto varType = lua_type(varPack, varValueIndex);
				if (varType == LUA_TTABLE) {
					const void * varTableIndexPointer = lua_topointer(*L, varValueIndex);
					auto varData = varAllTables.find(varTableIndexPointer);
					if (varData != varAllTables.end()) {
						$about_circle_tables.insert(varTableIndexPointer);
						break;
					}
					else {
						++varCurrentIndex;
						varStack.emplace_back(&(*varAllTables.emplace(varI.$TablePoiner, varTableIndexPointer).first),
							varCurrentIndex).push_create(varPack);
						varI.push(varPack);
						goto next_table;
					}
				}
				lua_settop(varPack, varPack.$UserKeyIndex);
			}

			--varCurrentIndex;
			varI.destory(varPack);
			varStack.pop_back();
		}

		ReturnInThisFunction;
	}

#if defined(QUICK_CHECK_CODE)
#else
	template<typename LuaLock >
#endif
	void CheckCircleTableData<true>::end(LuaLock*L) {
		/*输出额外的值*/
		for (const auto & varI : $string_about_to_write) {
			L->$Writer.write(std::get<0>(varI));
			L->print_equal();
			L->$Writer.write(std::get<1>(varI));
			L->print_endl();
		}
		/*输出return 表名*/
		if (this->$real_root_table_name.empty()) {
			L->$Writer.write(u8R"(
return tmp;
--[[ endl of the table --]]
)"sv);
		}
		else {
			L->$Writer.write(u8R"(
)"sv);
			L->$Writer.write(this->$real_root_table_name);
			L->$Writer.write(u8R"( = tmp ;
return tmp;
--[[ endl of the table --]]
)"sv);
		}
	}

#if defined(QUICK_CHECK_CODE)
	static inline void lua_table_to_text(lua_State * argL, const string_view & argTableName) {
		LuaLock test{ argL,WType{} };
		test.print_table(argTableName);
	}
#endif

}/*namespace*/


#if defined(QUICK_CHECK_CODE)

#else
namespace {
	namespace easy::writer {

		template<bool V, typename T>
		class WrapWriter :public T {
		public:
			template<typename ... Args>
			WrapWriter(Args && ... args) : T(std::forward<Args>(args)...) {}
			constexpr const static bool value = V;
		};

		static inline void __p_push_string(lua_State *L, const string_view &arg) {
			lua_pushlstring(L, arg.data(), arg.size());
		}
		template<bool V, typename _Writer, typename ... Args>
		static inline int _p_print_table(lua_State * L, Args && ...args) {

			using Writer = WrapWriter<V, _Writer>;

			const auto varInputTop = lua_gettop(L);

			if (lua_istable(L, varInputTop)) {
				auto var = make_unique<LuaLock<Writer>/*space*/>(L, Writer{ std::forward<Args>(args)... });
				var->print_table({});
			}
			else {
				if (varInputTop > 1) {
					const auto varTableIndex = varInputTop - 1;
					{
						if (lua_istable(L, varTableIndex)) {
							/*确保Table在最顶层*/
						}
						else {
							__p_push_string(L, u8R"(can not find a table)"sv);
							lua_error(L);
						}
					}
					std::size_t n = 0;
					const char * varAns = nullptr;

					if (lua_type(L, varInputTop) == LUA_TSTRING) {
						varAns = lua_tolstring(L, varInputTop, &n);
					}
					else {
						varAns = luaL_tolstring(L, varInputTop, &n);
					}

					if (n > 0) {
						/*
						确保Table在最顶层
						并且保证字符串地址有效...
						*/
						lua_pushvalue(L, varTableIndex);
						auto var = make_unique<LuaLock<Writer>/*space*/>(L, Writer{ std::forward<Args>(args)... });
						/*we do not check the table name is error or not*/
						var->print_table({ varAns,n });
					}
					else {
						__p_push_string(L, u8R"(can not find table name)"sv);
						lua_error(L);
					}
				}
				else {
					__p_push_string(L, u8R"(can not find a table)"sv);
					lua_error(L);
				}
			}
			return 0;
		}/***/
	}
}/*::easy::writer*/

namespace {
	/*
	input table/tablename or table
	no output
	*/
	template<bool V>
	static inline int _print_table_by_std_cout(lua_State * L) {
		class Writer {
		public:
			inline void write(const string_view &arg) {
				std::cout << arg;
			}
		};
		return easy::writer::_p_print_table<V, Writer>(L);
	}/*print_table_by_std_cout*/

	 /*
	 input table/tablename filename or table filename
	 no output
	 */
	template<bool V>
	static inline int _print_table_by_std_ofstream(lua_State *L) {

		class Writer {
			unique_ptr<std::ofstream> outer;
		public:
			inline void write(const std::string_view &arg) {
				(*outer) << arg;
			}
			Writer(lua_State *L, const std::string_view &arg) {
				outer = make_unique<std::ofstream>(arg.data(), std::ios::binary | std::ios::out);
				if (outer->is_open() == false) {
					easy::writer::__p_push_string(L, u8R"(can not open file)"sv);
					lua_error(L);
				}
			}
		};

		std::size_t n = 0;
		const auto varTop = lua_gettop(L);
		if (varTop < 2) {
			easy::writer::__p_push_string(L, u8R"(input too less)"sv);
			lua_error(L);
		}
		const auto data = luaL_tolstring(L, -1, &n);
		if (n > 0) {
			/*将Lua值拷贝到C++，确保数据地址有效*/
			const string varTmpData{ data,n };
			lua_settop(L, varTop - 1);
			return easy::writer::_p_print_table<V, Writer>(L, L, varTmpData);
		}
		else {
			easy::writer::__p_push_string(L, u8R"(can not find output file name)"sv);
			lua_error(L);
		}
		return 0;
	}

}/*namespace*/

LUA_API int sstd_print_table_by_std_ofstream(lua_State *L);
LUA_API int sstd_print_table_by_std_cout(lua_State *L);
LUA_API int sstd_full_print_table_by_std_ofstream(lua_State *L);
LUA_API int sstd_full_print_table_by_std_cout(lua_State *L);

LUA_API int sstd_print_table_by_std_ofstream(lua_State *L) {
	return _print_table_by_std_ofstream<false>(L);
}

LUA_API int sstd_print_table_by_std_cout(lua_State *L) {
	return _print_table_by_std_cout<false>(L);
}

LUA_API int sstd_full_print_table_by_std_ofstream(lua_State *L) {
	return _print_table_by_std_ofstream<true>(L);
}

LUA_API int sstd_full_print_table_by_std_cout(lua_State *L) {
	return _print_table_by_std_cout<true>(L);
}

#endif

