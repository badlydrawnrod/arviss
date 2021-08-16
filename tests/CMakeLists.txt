# Arviss tests.
add_executable(decode_test decode_test.cpp ../arviss.h)
target_link_libraries(decode_test PRIVATE arviss gtest_main)
add_test(decode_test decode_test)
