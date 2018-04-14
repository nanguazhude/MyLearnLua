
QT       -= core 
QT       -= gui

TEMPLATE = lib

include( $$PWD/lua.pri ) 

#CONFIG += c++17

CONFIG(debug,debug|release){
	TARGET = lua_libraryd
	DESTDIR = $$PWD/../../debug
}else{
	TARGET = lua_library
	DESTDIR = $$PWD/../../release
}

