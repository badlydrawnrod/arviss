# Arviss tests.
add_executable(decode_test decode_test.cpp smallmem.c smallmem.h ../arviss_impl.h)
target_link_libraries(decode_test PRIVATE arviss gtest_main)
add_test(decode_test decode_test)
