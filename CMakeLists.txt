cmake_minimum_required (VERSION 2.6)
project (Bageld)
add_executable(Bageld bageld.c string.c)
target_link_libraries(Bageld krb5 ssl)