﻿#define QUICK_CHECK_CODE

#include "../lua.hpp"

#include <list>
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
	constexpr int inline concept_tmp_buffer_size() { return 512; }

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
			int $LastIndex = 0;
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
			switch (data) {
			case -10: return u8R"(-10)"sv;
			case -9: return u8R"(-9)"sv;
			case -8: return u8R"(-8)"sv;
			case -7: return u8R"(-7)"sv;
			case -6: return u8R"(-6)"sv;
			case -5: return u8R"(-5)"sv;
			case -4: return u8R"(-4)"sv;
			case -3: return u8R"(-3)"sv;
			case -2: return u8R"(-2)"sv;
			case -1: return u8R"(-1)"sv;
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

			constexpr const static char int_to_string_map[] = {
				'0',
				'1',
				'2',
				'3',
				'4',
				'5',
				'6',
				'7',
				'8',
				'9',
			};

			{
				lldiv_t varTmp;
				varTmp.quot = data;
				do {
					varTmp = std::lldiv(varTmp.quot, 10);
					*varPointer = int_to_string_map[varTmp.rem];
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
			return {};
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
					$Writer.write(u8R"([)"sv);
					return ($Writer.write(int_to_string(d)), $Writer.write(u8R"(])"sv), true);
				}
				else {
					auto d = lua_tonumberx(*this, $UserKeyIndex, &n);
					$Writer.write(u8R"([)"sv);
					return ($Writer.write(double_to_string(d)), $Writer.write(u8R"(])"sv), true);
				}
			}

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

		void print_value_string() {
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
		catch (...) {/*意外的异常？？？*/
			$ReturnClearIndex = $ErrorStringIndex;
			throw;
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




