# CMake generated Testfile for 
# Source directory: /Users/glenn/reliable
# Build directory: /Users/glenn/reliable
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(test "/Users/glenn/reliable/bin/test")
set_tests_properties(test PROPERTIES  _BACKTRACE_TRIPLES "/Users/glenn/reliable/CMakeLists.txt;58;add_test;/Users/glenn/reliable/CMakeLists.txt;0;")
add_test(fuzz "/Users/glenn/reliable/bin/fuzz" "20000" "12345")
set_tests_properties(fuzz PROPERTIES  _BACKTRACE_TRIPLES "/Users/glenn/reliable/CMakeLists.txt;59;add_test;/Users/glenn/reliable/CMakeLists.txt;0;")
