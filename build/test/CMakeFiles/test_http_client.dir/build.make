# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.25

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/gong/projects/galay

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/gong/projects/galay/build

# Include any dependencies generated for this target.
include test/CMakeFiles/test_http_client.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include test/CMakeFiles/test_http_client.dir/compiler_depend.make

# Include the progress variables for this target.
include test/CMakeFiles/test_http_client.dir/progress.make

# Include the compile flags for this target's objects.
include test/CMakeFiles/test_http_client.dir/flags.make

test/CMakeFiles/test_http_client.dir/test_http_client.cc.o: test/CMakeFiles/test_http_client.dir/flags.make
test/CMakeFiles/test_http_client.dir/test_http_client.cc.o: /home/gong/projects/galay/test/test_http_client.cc
test/CMakeFiles/test_http_client.dir/test_http_client.cc.o: test/CMakeFiles/test_http_client.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/gong/projects/galay/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object test/CMakeFiles/test_http_client.dir/test_http_client.cc.o"
	cd /home/gong/projects/galay/build/test && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT test/CMakeFiles/test_http_client.dir/test_http_client.cc.o -MF CMakeFiles/test_http_client.dir/test_http_client.cc.o.d -o CMakeFiles/test_http_client.dir/test_http_client.cc.o -c /home/gong/projects/galay/test/test_http_client.cc

test/CMakeFiles/test_http_client.dir/test_http_client.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/test_http_client.dir/test_http_client.cc.i"
	cd /home/gong/projects/galay/build/test && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/gong/projects/galay/test/test_http_client.cc > CMakeFiles/test_http_client.dir/test_http_client.cc.i

test/CMakeFiles/test_http_client.dir/test_http_client.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/test_http_client.dir/test_http_client.cc.s"
	cd /home/gong/projects/galay/build/test && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/gong/projects/galay/test/test_http_client.cc -o CMakeFiles/test_http_client.dir/test_http_client.cc.s

# Object files for target test_http_client
test_http_client_OBJECTS = \
"CMakeFiles/test_http_client.dir/test_http_client.cc.o"

# External object files for target test_http_client
test_http_client_EXTERNAL_OBJECTS =

test/test_http_client: test/CMakeFiles/test_http_client.dir/test_http_client.cc.o
test/test_http_client: test/CMakeFiles/test_http_client.dir/build.make
test/test_http_client: src/libgalay-static.a
test/test_http_client: test/CMakeFiles/test_http_client.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/gong/projects/galay/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable test_http_client"
	cd /home/gong/projects/galay/build/test && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/test_http_client.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
test/CMakeFiles/test_http_client.dir/build: test/test_http_client
.PHONY : test/CMakeFiles/test_http_client.dir/build

test/CMakeFiles/test_http_client.dir/clean:
	cd /home/gong/projects/galay/build/test && $(CMAKE_COMMAND) -P CMakeFiles/test_http_client.dir/cmake_clean.cmake
.PHONY : test/CMakeFiles/test_http_client.dir/clean

test/CMakeFiles/test_http_client.dir/depend:
	cd /home/gong/projects/galay/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/gong/projects/galay /home/gong/projects/galay/test /home/gong/projects/galay/build /home/gong/projects/galay/build/test /home/gong/projects/galay/build/test/CMakeFiles/test_http_client.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : test/CMakeFiles/test_http_client.dir/depend

