
TEMPLATE = subdirs

lua_library.file = $$PWD/Project/lua_library/lua_library.pro
SUBDIRS += lua_library

test_table_to_text.file = $$PWD/Project/TestPrintTable/TestPrintTable.pro
test_table_to_text.depends += lua_library
SUBDIRS += test_table_to_text

