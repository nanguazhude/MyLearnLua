
#include "../lua.hpp"

#include <list>
#include <memory>
#include <string>
#include <cstddef>
#include <utility>
#include <string_view>
#include <type_traits>
#include <memory_resource>

using namespace std::string_view_literals;

namespace {

	using string_view = std::string_view;
	using string = std::string;// std::pmr::string;
	template<typename T> using list = std::list<T>;// std::pmr::list;

	/* Lua栈区最小值 */
	constexpr int inline concept_stack_min_size() { return 16; }
	/* 临时表最小大小 */
	constexpr int inline concept_min_tmp_table_size() { return 128; }

	template<typename WType/* WType w; w.write(const string_view &); */>
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
		class TableItem {
		public:
			int const $IndexInTmpTable;
			bool $TableArrayContinue = true;
			bool $IsCreate = true;
			TableItem(const int & arg) :$IndexInTmpTable(arg) {}

			inline void destory(const LuaLock & arg) {
				lua_pushnil(arg);
				lua_rawseti(arg, arg.$TmpTableIndex, $IndexInTmpTable);
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
			$ItemTables.emplace_back(++$CurrentTableIndex).push<true>(*this);
		}

		void print_name(TableItem*) {}
		void print_equal() {}
		void print_value_none() {}
		void print_value_nil() {}
		void print_value_bool() {}
		void print_value_lightuserdata() {}
		void print_value_number() {}
		void print_value_string() {}
		void print_value_function() {}
		void print_value_userdata() {}
		void print_value_thread() {}
		void print_value_numtags() {}
		void print_endl() {}

		void print_table(const string_view & argTableName) {
			this->clear(argTableName);

		next_table:
			while (false == $ItemTables.empty()) {
				auto & varCurrent = *$ItemTables.rbegin();
				if (varCurrent.$IsCreate) {
				}
				varCurrent.pop(*this);
				while (lua_next(*this, $UserTableIndex)) {
					const auto varType = lua_type(*this, $UserValueIndex);
					switch (varType) {
					case  LUA_TNONE: {
						this->print_name(&varCurrent);
						this->print_equal();
						this->print_value_none();
						this->print_endl();
					}break;
					case  LUA_TNIL: {
						this->print_name(&varCurrent);
						this->print_equal();
						this->print_value_nil();
						this->print_endl();
					}break;
					case  LUA_TBOOLEAN: {
						this->print_name(&varCurrent);
						this->print_equal();
						this->print_value_bool();
						this->print_endl();
					}break;
					case  LUA_TLIGHTUSERDATA: {
						this->print_name(&varCurrent);
						this->print_equal();
						this->print_value_lightuserdata();
						this->print_endl();
					}break;
					case  LUA_TNUMBER: {
						this->print_name(&varCurrent);
						this->print_equal();
						this->print_value_number();
						this->print_endl();
					}break;
					case  LUA_TSTRING: {
						this->print_name(&varCurrent);
						this->print_equal();
						this->print_value_string();
						this->print_endl();
					}break;
					case  LUA_TTABLE: {
						$ItemTables.emplace_back(++$CurrentTableIndex).push<true>(*this);
						varCurrent.push<false>(*this);
						goto next_table;
					}break;
					case  LUA_TFUNCTION: {
						this->print_name(&varCurrent);
						this->print_equal();
						this->print_value_function();
						this->print_endl();
					}break;
					case  LUA_TUSERDATA: {
						this->print_name(&varCurrent);
						this->print_equal();
						this->print_value_userdata();
						this->print_endl();
					}break;
					case  LUA_TTHREAD: {
						this->print_name(&varCurrent);
						this->print_equal();
						this->print_value_thread();
						this->print_endl();
					}break;
					case  LUA_NUMTAGS: {
						this->print_name(&varCurrent);
						this->print_equal();
						this->print_value_numtags();
						this->print_endl();
					}break;
					}
				}
			}

			--$CurrentTableIndex;
			$ItemTables.rbegin()->destory(*this);
			$ItemTables.pop_back();

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




