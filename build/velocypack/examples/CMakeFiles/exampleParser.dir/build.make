# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.10

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
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
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/lars/desktop/agency-test

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/lars/desktop/agency-test/build

# Include any dependencies generated for this target.
include velocypack/examples/CMakeFiles/exampleParser.dir/depend.make

# Include the progress variables for this target.
include velocypack/examples/CMakeFiles/exampleParser.dir/progress.make

# Include the compile flags for this target's objects.
include velocypack/examples/CMakeFiles/exampleParser.dir/flags.make

velocypack/examples/CMakeFiles/exampleParser.dir/exampleParser.cpp.o: velocypack/examples/CMakeFiles/exampleParser.dir/flags.make
velocypack/examples/CMakeFiles/exampleParser.dir/exampleParser.cpp.o: ../velocypack/examples/exampleParser.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/lars/desktop/agency-test/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object velocypack/examples/CMakeFiles/exampleParser.dir/exampleParser.cpp.o"
	cd /home/lars/desktop/agency-test/build/velocypack/examples && /usr/lib/ccache/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/exampleParser.dir/exampleParser.cpp.o -c /home/lars/desktop/agency-test/velocypack/examples/exampleParser.cpp

velocypack/examples/CMakeFiles/exampleParser.dir/exampleParser.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/exampleParser.dir/exampleParser.cpp.i"
	cd /home/lars/desktop/agency-test/build/velocypack/examples && /usr/lib/ccache/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/lars/desktop/agency-test/velocypack/examples/exampleParser.cpp > CMakeFiles/exampleParser.dir/exampleParser.cpp.i

velocypack/examples/CMakeFiles/exampleParser.dir/exampleParser.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/exampleParser.dir/exampleParser.cpp.s"
	cd /home/lars/desktop/agency-test/build/velocypack/examples && /usr/lib/ccache/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/lars/desktop/agency-test/velocypack/examples/exampleParser.cpp -o CMakeFiles/exampleParser.dir/exampleParser.cpp.s

velocypack/examples/CMakeFiles/exampleParser.dir/exampleParser.cpp.o.requires:

.PHONY : velocypack/examples/CMakeFiles/exampleParser.dir/exampleParser.cpp.o.requires

velocypack/examples/CMakeFiles/exampleParser.dir/exampleParser.cpp.o.provides: velocypack/examples/CMakeFiles/exampleParser.dir/exampleParser.cpp.o.requires
	$(MAKE) -f velocypack/examples/CMakeFiles/exampleParser.dir/build.make velocypack/examples/CMakeFiles/exampleParser.dir/exampleParser.cpp.o.provides.build
.PHONY : velocypack/examples/CMakeFiles/exampleParser.dir/exampleParser.cpp.o.provides

velocypack/examples/CMakeFiles/exampleParser.dir/exampleParser.cpp.o.provides.build: velocypack/examples/CMakeFiles/exampleParser.dir/exampleParser.cpp.o


# Object files for target exampleParser
exampleParser_OBJECTS = \
"CMakeFiles/exampleParser.dir/exampleParser.cpp.o"

# External object files for target exampleParser
exampleParser_EXTERNAL_OBJECTS =

velocypack/examples/exampleParser: velocypack/examples/CMakeFiles/exampleParser.dir/exampleParser.cpp.o
velocypack/examples/exampleParser: velocypack/examples/CMakeFiles/exampleParser.dir/build.make
velocypack/examples/exampleParser: velocypack/libvelocypack.a
velocypack/examples/exampleParser: velocypack/examples/CMakeFiles/exampleParser.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/lars/desktop/agency-test/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable exampleParser"
	cd /home/lars/desktop/agency-test/build/velocypack/examples && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/exampleParser.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
velocypack/examples/CMakeFiles/exampleParser.dir/build: velocypack/examples/exampleParser

.PHONY : velocypack/examples/CMakeFiles/exampleParser.dir/build

velocypack/examples/CMakeFiles/exampleParser.dir/requires: velocypack/examples/CMakeFiles/exampleParser.dir/exampleParser.cpp.o.requires

.PHONY : velocypack/examples/CMakeFiles/exampleParser.dir/requires

velocypack/examples/CMakeFiles/exampleParser.dir/clean:
	cd /home/lars/desktop/agency-test/build/velocypack/examples && $(CMAKE_COMMAND) -P CMakeFiles/exampleParser.dir/cmake_clean.cmake
.PHONY : velocypack/examples/CMakeFiles/exampleParser.dir/clean

velocypack/examples/CMakeFiles/exampleParser.dir/depend:
	cd /home/lars/desktop/agency-test/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/lars/desktop/agency-test /home/lars/desktop/agency-test/velocypack/examples /home/lars/desktop/agency-test/build /home/lars/desktop/agency-test/build/velocypack/examples /home/lars/desktop/agency-test/build/velocypack/examples/CMakeFiles/exampleParser.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : velocypack/examples/CMakeFiles/exampleParser.dir/depend

