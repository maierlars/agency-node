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
include velocypack/tools/CMakeFiles/vpack-validate.dir/depend.make

# Include the progress variables for this target.
include velocypack/tools/CMakeFiles/vpack-validate.dir/progress.make

# Include the compile flags for this target's objects.
include velocypack/tools/CMakeFiles/vpack-validate.dir/flags.make

velocypack/tools/CMakeFiles/vpack-validate.dir/vpack-validate.cpp.o: velocypack/tools/CMakeFiles/vpack-validate.dir/flags.make
velocypack/tools/CMakeFiles/vpack-validate.dir/vpack-validate.cpp.o: ../velocypack/tools/vpack-validate.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/lars/desktop/agency-test/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object velocypack/tools/CMakeFiles/vpack-validate.dir/vpack-validate.cpp.o"
	cd /home/lars/desktop/agency-test/build/velocypack/tools && /usr/lib/ccache/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/vpack-validate.dir/vpack-validate.cpp.o -c /home/lars/desktop/agency-test/velocypack/tools/vpack-validate.cpp

velocypack/tools/CMakeFiles/vpack-validate.dir/vpack-validate.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/vpack-validate.dir/vpack-validate.cpp.i"
	cd /home/lars/desktop/agency-test/build/velocypack/tools && /usr/lib/ccache/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/lars/desktop/agency-test/velocypack/tools/vpack-validate.cpp > CMakeFiles/vpack-validate.dir/vpack-validate.cpp.i

velocypack/tools/CMakeFiles/vpack-validate.dir/vpack-validate.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/vpack-validate.dir/vpack-validate.cpp.s"
	cd /home/lars/desktop/agency-test/build/velocypack/tools && /usr/lib/ccache/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/lars/desktop/agency-test/velocypack/tools/vpack-validate.cpp -o CMakeFiles/vpack-validate.dir/vpack-validate.cpp.s

velocypack/tools/CMakeFiles/vpack-validate.dir/vpack-validate.cpp.o.requires:

.PHONY : velocypack/tools/CMakeFiles/vpack-validate.dir/vpack-validate.cpp.o.requires

velocypack/tools/CMakeFiles/vpack-validate.dir/vpack-validate.cpp.o.provides: velocypack/tools/CMakeFiles/vpack-validate.dir/vpack-validate.cpp.o.requires
	$(MAKE) -f velocypack/tools/CMakeFiles/vpack-validate.dir/build.make velocypack/tools/CMakeFiles/vpack-validate.dir/vpack-validate.cpp.o.provides.build
.PHONY : velocypack/tools/CMakeFiles/vpack-validate.dir/vpack-validate.cpp.o.provides

velocypack/tools/CMakeFiles/vpack-validate.dir/vpack-validate.cpp.o.provides.build: velocypack/tools/CMakeFiles/vpack-validate.dir/vpack-validate.cpp.o


# Object files for target vpack-validate
vpack__validate_OBJECTS = \
"CMakeFiles/vpack-validate.dir/vpack-validate.cpp.o"

# External object files for target vpack-validate
vpack__validate_EXTERNAL_OBJECTS =

velocypack/tools/vpack-validate: velocypack/tools/CMakeFiles/vpack-validate.dir/vpack-validate.cpp.o
velocypack/tools/vpack-validate: velocypack/tools/CMakeFiles/vpack-validate.dir/build.make
velocypack/tools/vpack-validate: velocypack/libvelocypack.a
velocypack/tools/vpack-validate: velocypack/tools/CMakeFiles/vpack-validate.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/lars/desktop/agency-test/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable vpack-validate"
	cd /home/lars/desktop/agency-test/build/velocypack/tools && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/vpack-validate.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
velocypack/tools/CMakeFiles/vpack-validate.dir/build: velocypack/tools/vpack-validate

.PHONY : velocypack/tools/CMakeFiles/vpack-validate.dir/build

velocypack/tools/CMakeFiles/vpack-validate.dir/requires: velocypack/tools/CMakeFiles/vpack-validate.dir/vpack-validate.cpp.o.requires

.PHONY : velocypack/tools/CMakeFiles/vpack-validate.dir/requires

velocypack/tools/CMakeFiles/vpack-validate.dir/clean:
	cd /home/lars/desktop/agency-test/build/velocypack/tools && $(CMAKE_COMMAND) -P CMakeFiles/vpack-validate.dir/cmake_clean.cmake
.PHONY : velocypack/tools/CMakeFiles/vpack-validate.dir/clean

velocypack/tools/CMakeFiles/vpack-validate.dir/depend:
	cd /home/lars/desktop/agency-test/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/lars/desktop/agency-test /home/lars/desktop/agency-test/velocypack/tools /home/lars/desktop/agency-test/build /home/lars/desktop/agency-test/build/velocypack/tools /home/lars/desktop/agency-test/build/velocypack/tools/CMakeFiles/vpack-validate.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : velocypack/tools/CMakeFiles/vpack-validate.dir/depend
