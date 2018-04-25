#include "MainWindow.hpp"
#include <QApplication>

#include <lua.hpp>
 
inline void test_code1() {

	lua_State * L = luaL_newstate();
	luaL_openlibs(L);

	luaL_dostring(L,u8R"!!!!!!!(

print( sstd.utf8tolocal("今天天气很晴朗") );

return 0;

)!!!!!!!");

	lua_close(L);

}/*void test code 1*/

int main(int argc, char *argv[]){
    QApplication app(argc, argv);

	test_code1();

    MainWindow window;
    window.show();

    return app.exec();
}
