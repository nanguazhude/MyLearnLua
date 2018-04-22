#include "MainWindow.hpp"
#include <QApplication>

#include <lua.hpp>
 
inline void test_code1() {

	lua_State * L = luaL_newstate();
	luaL_openlibs(L);

	luaL_dostring(L,u8R"!!!!!!!(
a = { } ; --[[ a empty talbe --]]
a[1] = 1 ;
a[2] = 2 ;

sstd.tcout( a );

return 0;

)!!!!!!!");

	lua_close(L);

}/*void test code 1*/

inline void test_code2() {

	lua_State * L = luaL_newstate();
	luaL_openlibs(L);

	luaL_dostring(L, u8R"!!!!!!!(
a = { } ;  --[[ a empty talbe --]]
a[1] = 1 ;
a[2] = 2 ;

a.b={  } ; --[[ b empty talbe --]]
a.b[1] = 1 ;
a.b[2] = 2 ;

sstd.tcout( a );

return 0;

)!!!!!!!");

	lua_close(L);

}/*void test code 1*/

int main(int argc, char *argv[]){
    QApplication app(argc, argv);

	test_code1();
	test_code2();

    MainWindow window;
    window.show();

    return app.exec();
}
