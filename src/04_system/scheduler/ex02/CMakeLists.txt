cmake_minimum_required(VERSION 3.28)
project(cgroup_mem_tester)

include(../../../nanopi.cmake)
add_executable(cgroup_mem_tester cgroup_mem_tester.c)