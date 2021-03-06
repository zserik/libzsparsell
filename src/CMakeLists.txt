add_library(zsparsell SHARED escape/char.cxx escape/cstr.cxx escape/str.cxx
                             lexer/get_line.cxx lexer/hexcode.cxx lexer/Lexer.cxx lexer/ldbg.cxx
                             parser/Parser.cxx random.cxx)

set_target_properties (zsparsell PROPERTIES VERSION "3.0.0" SOVERSION 3)
target_compile_options(zsparsell PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>)
target_link_libraries (zsparsell LowlevelZS::lowlevelzs)

set_source_files_properties(escape/cstr.cxx escape/char.cxx lexer/get_line.cxx lexer/hexcode.cxx
  PROPERTIES COMPILE_FLAGS "-fno-exceptions -fno-unwind-tables")

add_executable(zskwproc kwproc.c)

# === INSTALL SECTION ===
install(TARGETS zsparsell DESTINATION "${INSTALL_LIB_DIR}" EXPORT ZSparsellTargets)
install(EXPORT ZSparsellTargets DESTINATION "${INSTALL_CMAKE_DIR}" NAMESPACE ZSparsell:: COMPONENT dev)

install(FILES escape/escape.hpp parser/Parser.hpp random.hpp reverse.hpp
    lexer/Backtrack.hpp lexer/InputSlice.hpp lexer/hexcode.hpp lexer/LexPos.hpp lexer/Lexer.hpp
  DESTINATION "${INSTALL_INCLUDE_DIR}")

install(TARGETS zskwproc DESTINATION "${INSTALL_BIN_DIR}")
