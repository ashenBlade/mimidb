add_library(${SQLPARSER_LIB_NAME} STATIC)

target_include_directories(${SQLPARSER_LIB_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")

target_sources(${SQLPARSER_LIB_NAME} PRIVATE
    src/SQLParser.cpp
    src/SQLParserResult.cpp

    src/parser/bison_parser.cpp
    src/parser/flex_lexer.cpp

    src/sql/CreateStatement.cpp
    src/sql/Expr.cpp
    src/sql/PrepareStatement.cpp
    src/sql/SQLStatement.cpp
    src/sql/statements.cpp

    src/util/sqlhelper.cpp
)

# Top level CMakeFile adds multiple different strict checks,
# but they are firing here a lot, so disable all checks.
target_compile_options(${SQLPARSER_LIB_NAME} PRIVATE
    -Wno-error=conversion
    -Wno-error=old-style-cast
    -Wno-error=unused-parameter
    -Wno-error=sign-conversion
    -Wno-error=free-nonheap-object
    -Wno-error=sign-promo
)
