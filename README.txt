########################################################################
########################################################################
##                USGS ISIS CMake build system notes                  ##
########################################################################
########################################################################

#==============================================================================
#============================ Introduction ====================================
#==============================================================================


# Overview --------------------------------------------------------------

This build system was developed for the post-3.5.0 version of the USGS
ISIS software.  The intent is to fully replace the existing Makefile
based build system with something that is easier to update and maintain.


# Requirements --------------------------------------------------------------

- Tested Operating Systems

Ubuntu 14, CentOS 7, and OSX 10.

- Tested Compilers

Linux: GCC 4.8.4
OSX:   Clang 7.3.0

- Other software:

CMake version 3.2 or higher.

The 3rd party library requirements for ISIS are the same as they
are with the old Makefile build system.


# CMake information ------------------------------------------------------------

There is not a really good CMake guide on the web but here are some useful resources
for getting started with it:

The official CMake tutorial
--> https://cmake.org/cmake-tutorial/

A CMake syntax overview
--> https://cmake.org/Wiki/CMake/Language_Syntax

A list of all the supported CMake commands and their arguments
--> https://cmake.org/cmake/help/v3.2/manual/cmake-commands.7.html

A list of standard CMake variables that may be used
--> https://cmake.org/Wiki/CMake_Useful_Variable


#==============================================================================
#========================= Using the build system =============================
#==============================================================================


# Running the build --------------------------------------------------------

Using the new build system is very similar to building other CMake based
software.  Start by creating a new build folder, a common choice is a 
folder named "build" in the same folder as the source folder.  By 
using a new folder you can easily delete it to get a completely clean
build and you won't introduce any new files into your source folder.
Change directory into the new folder, then run cmake pointed at the top
level of the ISIS folder.  ISIS supports the following special options:

isis3Data     = Specify where the downloadable ISIS data is located.
                This folder contains mission kernels and sensor info.
isis3TestData = Specify the location containing the special ISIS application test data.
                This folder contains input and truth folder for application and module tests.
testOutputDir = Specify where output folder from running app/module tests will go.
                Can be set to the same location as isis3TestData.
buildCore     = Set to OFF to skip building core libraries.  This includes libisis3 and
                the "plugin" libraries such as libShade, libTopo, and libPlanar.
buildMissions = Set to OFF to skip building mission code.  This includes all the
                libraries with mission names such as libMsiCamera and libcassini.
buildStatic   = Set to ON to build the core library as static in addition to dynamic.
                Adding a static build will increase the total build time.
buildTests    = Set to OFF to disable building all unit tests.  If this is set to ON,
                all tests will be built for all enabled modules.

You can also use standard CMake options.  The following options will
probably be the most frequently used:

CMAKE_INSTALL_PREFIX = Specify where the installation folder.
CMAKE_BUILD_TYPE     = Choose Debug, Release, RelWithDebInfo, or MinSizeRel


Once CMake finishes running, run "make" to build the code, "make install" to 
install it to the output, and "make docs" to generate the documentation files
in the installation folder.  Use the "-j" argument to specify the number of
threads to use.  A full set of commands may look like:

cd ~/isis_code
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=~/isis_install -Disis3Data=~/isis3data \
-Disis3TestData=~/isis3TestData ~/path/to/isis/source
make -j 4
make install -j 4
make docs

The build system also accepts the "make clean_source" command which removes
all files and folders that need to be created in the top level of the ISIS
folder in order to run the unit tests.


# Building individual components --------------------------------------------

Unfortunately the design of CMake does not lend itself to building individual
software components independently of each other but it is still possible with
some effort.  The trick to doing this is that the output of CMake is a Makefile
based system and those Makefiles can be accessed directly.  For example, to 
make a change to the file src/base/objs/Pvl/Pvl.cpp and rerun a unit test,
the following steps can be performed:

1 - Start from the build folder.
2 - Recompile the .o file with this command:
>> make -f  src/CMakeFiles/isis3.dir/build.make  src/CMakeFiles/isis3.dir/base/objs/Pvl/Pvl.cpp.o
3 - Recompile the unit test with this command:
>> make isis3_unit_test_Pvl
4 - Run the unit test normally.
>> ctest -R isis3_unit_test_Pvl

If one of the make commands has no effect, try manually deleting the existing file you are
trying to build and then re-run the make command.


# Running tests ------------------------------------------------------------

The CMake build system relies on the CTest tool, which should be included with
CMake, to handle unit, application, and module tests.  To run the tests, simply
run the ctest command from the build directory.  The CTest tool has a number of
useful built-in options to help run the tests which you can see by running 
"ctest --help".  Note that due to the naming convention applied to the tests in
the build system you can choose to run only a certain category of tests by adding
the options "-R _unit_", "-R _app_", or "-R _module_".

Application and module test output folders will be placed in a directory structure
underneath the folder specified with the "testOutputDir" option.  Console output
from running tests is placed in the [build_folder]/src location in files ending
with the extension ".output" and with names matching the test they correspond to.
These test files are only retained if the test failed.  You can find additional 
information about the last test that was run by looking at the file 
[build_folder]/Testing/Temporary/LastTest.log.  


# Building the documentation --------------------------------------------------------

To generate the documentation, just run "make docs" after running CMake.  All of the
relevant documentation should be placed in the installation folder.  Intermediate
documentation build files will be placed in the build folder, but the final output
documentation will only be created in the install folder.  The third party 
tools "xalan", "doxygen", and "latex" must be available in order to generate the
documentation.


# Build versus install versus source ------------------------------------------------

Unless you specify the source directory as the build or test output directory, very
few files will be placed in the source folder as part of the build.  All of the other files
generated by running "make" will be placed in the build folder.  The build folder contains 
all of the compiled .o files, generated protobuf/uic/moc files, libraries, and executables.
The build folder also contains many automatically generated CMake files which can usually
be ignored.

The "make install" step is only required to prepare software for distribution.  The
installation folder will contain only the libraries, binaries, supporting files, and
documentation that a user needs to run ISIS tools.  Source code, tests, and build
system files are not placed in to the installation folder.  The installation folder
should be ready to tar up and be used on another computer.


# Cleaning up the build -------------------------------------------------------------

To completely clean up the build just delete the entire build folder and create a new
one.  You can also run "make clean" to remove files generated during the build as 
opposed to files generated while running CMake.  To remove all cmake generated files
from the source directory (there should only be a couple folders) run "make clean_source".


# Troubleshooting -------------------------------------------------------------

Q: I am running cmake from a build directory but the files are being generated in
the source directory.
A: Completely clean the cmake generated files from the source directory and try again.
If generated cmake files are there it will continue to generate them in that location.

Q: The build fails early on complaining about a library not found.
A: Make sure that the dynamic libraries needed to run uic, moc, and protoc are in the
search path.  This may mean adding the 3rdParty/lib folder to LD_LIBRARY_PATH.

Q: A third party library is not being found.
A: Verify that the library is installed on your system or in the 3rdParty/lib folder.
The build system does not search for libraries outside these locations.


#==============================================================================
#======================== Build system details ================================
#==============================================================================


# File listing ----------------------------------------------------------------

The CMake build system consists of 15 files, each of which is described below.

README.txt = This file!  Contains a description of the build system.

CMakeLists.txt = The top level build file.  Handles input options, defines project
characteristics, sets up some required links, and calls all of the other required modules.

src/CMakeLists.txt = The lower level build file.  This defines which module folders
are used to create libraries.  There is one core library and one library per mission
directory.  The source files under this folder are found by search functions running
from this folder.

In the /cmake folder:

AddIsisModule.cmake       = Functions for parsing a module folder and adding all 
                            of the components to the build.
BuildDocs.cmake           = This file handles tho documentation production.
CodeGeneration.cmake      = Functions for generating moc, uic, and protobuf files.
FindAllDependencies.cmake = Identify all of the required 3rd party libraries.
InstallThirdParty.cmake   = Handle transfer of 3rd party libraries to the install folder.
RunMakeFileTest.cmake     = Script that is called to execute an app or module (Makefile based) test.
RunUnitTest.cmake         = Script that is called to execute a unit test.
TestSetup.cmake           = Helper functions for setting up tests.
Utilities.cmake           = Miscellaneous functions used in the other files.

In the /scripts folder:

finalizeInstalledOsxRpaths.py   = Script to correct the RPaths of OSX libraries 
                                  after they are installed.
IsisInlineDocumentBuild_mod.xsl = An old documentation build file modified for 
                                  use by the CMake build system.
fetchRequiredData.py            = Script for fetching a subset of the ISIS mission 
                                  data for the purpose of running unit tests.  
                                  Not required by the build system.


# Maintenance tips ----------------------------------------------------------------

- There are a number of file requirements of ISIS that the build system takes extra 
steps to work around.  These include:
  - Unit tests (as opposed to application or module tests) must be run from an executable
    named "unitTest" in order for certain GUIs to be suppressed.  They must have the 
    associated xml file with the same name in the same directory.
  - Application xml files must be located in $ISISROOT/bin/xml
  - $ISISROOT/IsisPreferences must exist.
  - $ISISROOT/src/base/objs/Preference/TestPreferences must exist to run tests.
  
- New files are added automatically when CMake is run, but new module folders must be
  manually added in src/CMakeLists.txt.  Tests of all types are added automatically.

- Required 3rd party libraries are explicitly listed in FindAllDependencies.cmake

- Miscellaneous installation and build settings are found in the top level CMakeLists.txt

- Turning on static building of the core library increases the build time.

- The file organization in the build folder does not mimic the file organization
  in the source directory but all of the .o files are there and can be accessed.


# Possible new features -----------------------------------------------------------

- Building/testing individual modules from within the CMake build directory is 
  inconvenient.  It would be possible to add any number of separate build targets
  so that a user could type "make base_Pvl_test" or something similar from the build
  folder and have the relevant class and its unit tests rebuilt.
  
- It is easy in CMake to add new build targets to do things like "remove all output
  folders from the test output directory".
  

# Future development ----------------------------------------------------------------

- libisis is the only library with a static build option but it would be easy to make
  the other libraries also built as static.

- CMake has its own system for unit test coverage that could be used as an alternative for
  a proprietary system.  The current CMake build system does not provide any test coverage
  options.

- CMake has tools for automated fetching and building of 3rd party libraries that could
  be used to obtain all of the ISIS dependencies.  Ames Stereo Pipeline has a longstanding
  system to do this that installs nearly all of ISIS' dependencies.

- Unlike in the MakeFile build, there are not CMake build files spread throughout the 
  source tree.  Setting up a tree of CMakeLists.txt files should make the CMake generated 
  build files more closely resemble the source directory but it would require the creation
  of many more files (as of now there are almost 2400 Makefiles in the src folder.)  
  Some special handling may be needed to make this work with the mutually interdependent
  core library folders.






