
if(UNIX)
	set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -O3 -std=c++0x -Wall -DREGEX_MAX_NODES=307 -g")
endif(UNIX)

set(COMMON common)

set(TESTS_HEADERS
	lightweight_test.hpp
	test_config.h
)

install(FILES ${TESTS_HEADERS} DESTINATION include/public/regex)

add_executable(test_trie test_trie.cpp)
target_link_libraries(test_trie ${COMMON})

add_executable(test_regex test_regex.cpp)
target_link_libraries(test_regex ${COMMON} ${PCRE2})

add_executable(test_regexobj test_regexobj.cpp)
target_link_libraries(test_regexobj ${COMMON})

add_executable(regex_test regex_test.cpp)
target_link_libraries(regex_test ${COMMON})

add_executable(test_qmatch test_qmatch.cpp)
target_link_libraries(test_qmatch ${COMMON})

add_test(test_regex test_regex)
add_test(test_regexobj1 test_regexobj)
add_test(test_regexobj2 test_regexobj "abc.*123(a|b|c){2,3}")
add_test(test_qmatch test_qmatch)
add_test(test_trie_tree test_trie ${PROJECT_SOURCE_DIR}/test/word.list ${PROJECT_SOURCE_DIR}/test/word.txt)
add_test(test_regexobj3 test_regexobj ${PROJECT_SOURCE_DIR}/test/rules.txt)

set(TESTS_HEADERS
	lightweight_test.hpp
	test_config.h
)

install(FILES ${TESTS_HEADERS} DESTINATION include/public/regex)
