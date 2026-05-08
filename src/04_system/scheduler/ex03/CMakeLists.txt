cmake_minimum_required(VERSION 3.28)
project(cgroup_cpu_tester)

include(../../../nanopi.cmake)
add_executable(cgroup_cpu_tester cgroup_cpu_tester.c)