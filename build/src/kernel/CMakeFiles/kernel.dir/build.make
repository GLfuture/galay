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
include src/kernel/CMakeFiles/kernel.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include src/kernel/CMakeFiles/kernel.dir/compiler_depend.make

# Include the progress variables for this target.
include src/kernel/CMakeFiles/kernel.dir/progress.make

# Include the compile flags for this target's objects.
include src/kernel/CMakeFiles/kernel.dir/flags.make

src/kernel/CMakeFiles/kernel.dir/engine.cc.o: src/kernel/CMakeFiles/kernel.dir/flags.make
src/kernel/CMakeFiles/kernel.dir/engine.cc.o: /home/gong/projects/galay/src/kernel/engine.cc
src/kernel/CMakeFiles/kernel.dir/engine.cc.o: src/kernel/CMakeFiles/kernel.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/gong/projects/galay/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object src/kernel/CMakeFiles/kernel.dir/engine.cc.o"
	cd /home/gong/projects/galay/build/src/kernel && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT src/kernel/CMakeFiles/kernel.dir/engine.cc.o -MF CMakeFiles/kernel.dir/engine.cc.o.d -o CMakeFiles/kernel.dir/engine.cc.o -c /home/gong/projects/galay/src/kernel/engine.cc

src/kernel/CMakeFiles/kernel.dir/engine.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/kernel.dir/engine.cc.i"
	cd /home/gong/projects/galay/build/src/kernel && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/gong/projects/galay/src/kernel/engine.cc > CMakeFiles/kernel.dir/engine.cc.i

src/kernel/CMakeFiles/kernel.dir/engine.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/kernel.dir/engine.cc.s"
	cd /home/gong/projects/galay/build/src/kernel && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/gong/projects/galay/src/kernel/engine.cc -o CMakeFiles/kernel.dir/engine.cc.s

src/kernel/CMakeFiles/kernel.dir/iofunction.cc.o: src/kernel/CMakeFiles/kernel.dir/flags.make
src/kernel/CMakeFiles/kernel.dir/iofunction.cc.o: /home/gong/projects/galay/src/kernel/iofunction.cc
src/kernel/CMakeFiles/kernel.dir/iofunction.cc.o: src/kernel/CMakeFiles/kernel.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/gong/projects/galay/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object src/kernel/CMakeFiles/kernel.dir/iofunction.cc.o"
	cd /home/gong/projects/galay/build/src/kernel && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT src/kernel/CMakeFiles/kernel.dir/iofunction.cc.o -MF CMakeFiles/kernel.dir/iofunction.cc.o.d -o CMakeFiles/kernel.dir/iofunction.cc.o -c /home/gong/projects/galay/src/kernel/iofunction.cc

src/kernel/CMakeFiles/kernel.dir/iofunction.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/kernel.dir/iofunction.cc.i"
	cd /home/gong/projects/galay/build/src/kernel && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/gong/projects/galay/src/kernel/iofunction.cc > CMakeFiles/kernel.dir/iofunction.cc.i

src/kernel/CMakeFiles/kernel.dir/iofunction.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/kernel.dir/iofunction.cc.s"
	cd /home/gong/projects/galay/build/src/kernel && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/gong/projects/galay/src/kernel/iofunction.cc -o CMakeFiles/kernel.dir/iofunction.cc.s

kernel: src/kernel/CMakeFiles/kernel.dir/engine.cc.o
kernel: src/kernel/CMakeFiles/kernel.dir/iofunction.cc.o
kernel: src/kernel/CMakeFiles/kernel.dir/build.make
.PHONY : kernel

# Rule to build all files generated by this target.
src/kernel/CMakeFiles/kernel.dir/build: kernel
.PHONY : src/kernel/CMakeFiles/kernel.dir/build

src/kernel/CMakeFiles/kernel.dir/clean:
	cd /home/gong/projects/galay/build/src/kernel && $(CMAKE_COMMAND) -P CMakeFiles/kernel.dir/cmake_clean.cmake
.PHONY : src/kernel/CMakeFiles/kernel.dir/clean

src/kernel/CMakeFiles/kernel.dir/depend:
	cd /home/gong/projects/galay/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/gong/projects/galay /home/gong/projects/galay/src/kernel /home/gong/projects/galay/build /home/gong/projects/galay/build/src/kernel /home/gong/projects/galay/build/src/kernel/CMakeFiles/kernel.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : src/kernel/CMakeFiles/kernel.dir/depend

