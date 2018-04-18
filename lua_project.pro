
TEMPLATE = subdirs

lua_library.file = $$PWD/Project/lua_library/lua_library.pro
SUBDIRS += lua_library

lua.file = $$PWD/Project/lua/lua.pro
lua.depends += lua_library
SUBDIRS += lua

test_table_to_text.file = $$PWD/Project/TestPrintTable/TestPrintTable.pro
test_table_to_text.depends += lua_library
SUBDIRS += test_table_to_text

