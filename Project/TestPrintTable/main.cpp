#include "MainWindow.hpp"
#include <QApplication>

#include <lua.hpp>
 
inline void test_code1() {

	lua_State * L = luaL_newstate();
	luaL_openlibs(L);

	luaL_dostring(L,u8R"!!!!!!!(
a = { } ; --[[ a empty talbe --]]
a[1] = "today is a fine day!" ;
a[2] = 2 ;
function a:xxx() end;

print("---------------------")
sstd.tcout( a );
print("+--------------------")
sstd.ftcout( a , "mytest_0" );

return 0;

)!!!!!!!");

	lua_close(L);

}/*void test code 1*/

inline void test_code2() {

	lua_State * L = luaL_newstate();
	luaL_openlibs(L);

	luaL_dostring(L, u8R"!!!!!!!(
a = { x = 1 ; } ;  --[[ a empty talbe --]]
 

a.b={ [ "y" ] = 2 ; } ; --[[ b empty talbe --]]
 
print("---------------------")
sstd.tcout( a );
print("+--------------------")
sstd.ftcout( a );

return 0;

)!!!!!!!");

	lua_close(L);

}/*void test code 2*/

inline void test_code3() {

	lua_State * L = luaL_newstate();
	luaL_openlibs(L);

	luaL_dostring(L, u8R"!!!!!!!(

a={} ;
a.b={} ;
a.c=a.b ;
a.d=a ;

print("---------------------")
sstd.tcout( a );
print("+--------------------")
sstd.ftcout( a );

)!!!!!!!");

	lua_close(L);

}/*void test code 3*/

inline void test_code4() {

	lua_State * L = luaL_newstate();
	luaL_openlibs(L);

	luaL_dostring(L, u8R"!!!!!!!(

a={ test4 = 0; } ;
a.b={} ;
a.c=a.b ;
a.d=a ;

print("---------------------")
print( sstd.tstring( a ) );
print("+--------------------")
print( sstd.ftstring( a ) );

)!!!!!!!");

	lua_close(L);

}/*void test code 4*/

inline void test_code5() {
	lua_State * L = luaL_newstate();
	luaL_openlibs(L);

	lua_createtable(L,2,2);
	lua_pushinteger(L,12);
	lua_rawsetp(L,-2,L);
	lua_setglobal(L,u8R"(a)");

	luaL_dostring(L, u8R"!!!!!!!(

print("---------------------")
print( sstd.tstring( a ) );
print("+--------------------")
print( sstd.ftstring( a ) );

)!!!!!!!");

	lua_close(L);
}/*void test code 5*/

int main(int argc, char *argv[]){
    QApplication app(argc, argv);

	//test_code1();
	//test_code2();
	//test_code3();
	//test_code4();
	test_code5();

    MainWindow window;
    window.show();

    return app.exec();
}
