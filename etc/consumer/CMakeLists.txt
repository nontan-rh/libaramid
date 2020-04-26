cmake_minimum_required(VERSION 3.10.2)
cmake_policy(VERSION 3.10.2...3.10.2)

project(aramid_consumer)

find_package(aramid REQUIRED)

add_executable(consumer main.c)
target_link_libraries(consumer aramid::aramid)
