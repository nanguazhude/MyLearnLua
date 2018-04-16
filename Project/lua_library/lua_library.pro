DEFINES += LUA_BUILD_AS_DLL
DEFINES += LUA_CORE
DEFINES += LUA_LIB

QT       -= core 
QT       -= gui

TEMPLATE = lib

include( $$PWD/lua.pri ) 
include( $$PWD/lua_source.pri ) 
include( $$PWD/part_google_v8/double_conversion.pri )

#CONFIG += c++17

CONFIG(debug,debug|release){
	TARGET = lua_libraryd
	DESTDIR = $$PWD/../../debug
}else{
	TARGET = lua_library
	DESTDIR = $$PWD/../../release
}

