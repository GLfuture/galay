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
include src/CMakeFiles/galay-shared.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include src/CMakeFiles/galay-shared.dir/compiler_depend.make

# Include the progress variables for this target.
include src/CMakeFiles/galay-shared.dir/progress.make

# Include the compile flags for this target's objects.
include src/CMakeFiles/galay-shared.dir/flags.make

# Object files for target galay-shared
galay__shared_OBJECTS =

# External object files for target galay-shared
galay__shared_EXTERNAL_OBJECTS = \
"/home/gong/projects/galay/build/src/kernel/CMakeFiles/kernel.dir/engine.cc.o" \
"/home/gong/projects/galay/build/src/kernel/CMakeFiles/kernel.dir/iofunction.cc.o" \
"/home/gong/projects/galay/build/src/kernel/CMakeFiles/kernel.dir/task.cc.o" \
"/home/gong/projects/galay/build/src/util/CMakeFiles/util.dir/stringutil.cc.o" \
"/home/gong/projects/galay/build/src/util/CMakeFiles/util.dir/base64.cc.o" \
"/home/gong/projects/galay/build/src/factory/CMakeFiles/factory.dir/factory.cc.o" \
"/home/gong/projects/galay/build/src/protocol/CMakeFiles/protocol.dir/http.cc.o" \
"/home/gong/projects/galay/build/src/protocol/CMakeFiles/protocol.dir/tcp.cc.o" \
"/home/gong/projects/galay/build/src/config/CMakeFiles/config.dir/config.cc.o" \
"/home/gong/projects/galay/build/src/server/CMakeFiles/server.dir/server.cc.o" \
"/home/gong/projects/galay/build/src/client/CMakeFiles/client.dir/client.cc.o"

src/libgalay-shared.so: src/kernel/CMakeFiles/kernel.dir/engine.cc.o
src/libgalay-shared.so: src/kernel/CMakeFiles/kernel.dir/iofunction.cc.o
src/libgalay-shared.so: src/kernel/CMakeFiles/kernel.dir/task.cc.o
src/libgalay-shared.so: src/util/CMakeFiles/util.dir/stringutil.cc.o
src/libgalay-shared.so: src/util/CMakeFiles/util.dir/base64.cc.o
src/libgalay-shared.so: src/factory/CMakeFiles/factory.dir/factory.cc.o
src/libgalay-shared.so: src/protocol/CMakeFiles/protocol.dir/http.cc.o
src/libgalay-shared.so: src/protocol/CMakeFiles/protocol.dir/tcp.cc.o
src/libgalay-shared.so: src/config/CMakeFiles/config.dir/config.cc.o
src/libgalay-shared.so: src/server/CMakeFiles/server.dir/server.cc.o
src/libgalay-shared.so: src/client/CMakeFiles/client.dir/client.cc.o
src/libgalay-shared.so: src/CMakeFiles/galay-shared.dir/build.make
src/libgalay-shared.so: src/CMakeFiles/galay-shared.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/gong/projects/galay/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Linking CXX shared library libgalay-shared.so"
	cd /home/gong/projects/galay/build/src && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/galay-shared.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
src/CMakeFiles/galay-shared.dir/build: src/libgalay-shared.so
.PHONY : src/CMakeFiles/galay-shared.dir/build

src/CMakeFiles/galay-shared.dir/clean:
	cd /home/gong/projects/galay/build/src && $(CMAKE_COMMAND) -P CMakeFiles/galay-shared.dir/cmake_clean.cmake
.PHONY : src/CMakeFiles/galay-shared.dir/clean

src/CMakeFiles/galay-shared.dir/depend:
	cd /home/gong/projects/galay/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/gong/projects/galay /home/gong/projects/galay/src /home/gong/projects/galay/build /home/gong/projects/galay/build/src /home/gong/projects/galay/build/src/CMakeFiles/galay-shared.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : src/CMakeFiles/galay-shared.dir/depend

