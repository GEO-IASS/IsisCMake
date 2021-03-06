#===============================================================================
#      The main build file for building ISIS using CMake.
#===============================================================================
# CMake initialization

# Specify the required version of CMake.  If your machine does not
#  have this, it should be easy to build from https://cmake.org/download/
cmake_minimum_required(VERSION 3.2)

# Point cmake to our other CMake files.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
list(APPEND CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/cmake")

include(AddIsisModule)
include(Utilities)
include(TestSetup)
include(InstallThirdParty)


#===============================================================================
#===============================================================================
# Project information

project (USGS_ISIS)

# Short and long name of this package
set(PACKAGE            "ISIS")
set(PACKAGE_NAME       "USGS ISIS")

# Version number
set(VERSION            "3.5.00.0")
set(PACKAGE_VERSION    ${VERSION})

# Full name and version number
set(PACKAGE_STRING     "${PACKAGE_NAME} ${VERSION}")

# Other release information
set(VERSION_DATE              "2017-04-24")
set(THIRD_PARTY_LIBS_VERSION  "v006")
set(RELEASE_STAGE             "alpha") # (alpha, beta, stable)

# Define to the address where bug reports for this package should be sent. 
set(PACKAGE_BUGREPORT  "https://isis.astrogeology.usgs.gov/fixit")

# Main website associated with the software package
set(PACKAGE_URL        "https://isis.astrogeology.usgs.gov/")

# Retrieve a string describing the OS this is built on.
get_os_version(osVersionString)
message("Detected Operating System: ${osVersionString}")


#===============================================================================
#===============================================================================
# Configuration options

# All libraries are build as shared.  The main library is also built 
#  as a static library using some specialized code in Utilities.cmake.
set(BUILD_SHARED_LIBS ON)

# Specify flags used 
set(thirdPartyCppFlags -Wall -ansi -DISIS_LITTLE_ENDIAN=1 -fPIC)

# Specify user options that can be passed in with the initial CMake command.
option(isis3Data       "Directory containing Isis3Data"              "NA")
option(isis3TestData   "Directory containing Isis3TestData"          "NA")
option(testOutputDir   "Directory to store app test output folders"  "NA")
option(buildCore       "Build the core ISIS modules"                 ON  )
option(buildMissions   "Build the mission specific modules"          ON  )
option(buildStaticCore "Build libisis3 static as well as dynamic"    OFF )
option(buildTests      "Set up unit, application, and module tests." ON  )

if(EXISTS ${isis3Data})
  set(ENV{ISIS3DATA} "${isis3Data}")
  message("Using ISIS3DATA = $ENV{ISIS3DATA}")
else()
  message(WARNING "Isis3Data directory ${isis3Data} not found, unit tests will fail.")
endif()
if(EXISTS ${isis3TestData})
  set(ENV{ISIS3TESTDATA} "${isis3TestData}")
  message("Using ISIS3TESTDATA = $ENV{ISIS3TESTDATA}")
else()
  message(WARNING "Isis3TestData directory ${isis3TestData} not found, application and module tests will fail.")
endif()
if(${testOutputDir} STREQUAL "NA")
  # User did not specify the test directory, default to the test input folder.
  message(WARNING "Test output directory not specified, setting it to ${isis3TestData}.")
else()
  # User specified a test output folder, make sure it exists.
  message("Writing test data folders to = ${testOutputDir}")
  execute_process(COMMAND mkdir -p ${testOutputDir})
endif()

# Set the default library extension based on the platform
if(APPLE)
  set(SO ".dylib")
else()
  set(SO ".so")
endif()


#===============================================================================
#===============================================================================
# Start setting up the build

# Set up all the third party library dependencies.
include(FindAllDependencies)

# Allow everything to include the 3rd party libraries
include_directories(${ALLINCDIRS})
link_directories(${ALLLIBDIRS})

set(CORE_LIB_NAME isis3)


# Specify relative library include paths which will be set up on
#  the installed files.
if(APPLE)
  set(CMAKE_INSTALL_RPATH "@loader_path/../lib;@loader_path/../3rdParty/lib")
else()
  set(CMAKE_INSTALL_RPATH "$ORIGIN/../lib;$ORIGIN/../3rdParty/lib")
endif()
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)

# We will set up some links with these files at the end of the install process so
#  make sure they are cleared at the start of the install process.
install(CODE "EXECUTE_PROCESS(COMMAND rm -f ${CMAKE_INSTALL_PREFIX}/lib/libisis3.5.0${SO})")
install(CODE "EXECUTE_PROCESS(COMMAND rm -f ${CMAKE_INSTALL_PREFIX}/lib/libisis3.5${SO})")
install(CODE "EXECUTE_PROCESS(COMMAND rm -f ${CMAKE_INSTALL_PREFIX}/lib/libisis3.${SO})")


# Append CPP flags set in the third party lib file to the string set in this file.
string(REPLACE ";" " " FLAGS_STR "${thirdPartyCppFlags}")
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${FLAGS_STR}" )
message("CMAKE_CXX_FLAGS = ${CMAKE_CXX_FLAGS}")

# Set up the ctest tool which is used to run all of the tests.
enable_testing()
include(CTest)

# Delete any existing plugin files in the build folder so they
#  don't get filled with duplicate entries.
file(GLOB existingPlugins "${CMAKE_BINARY_DIR}/src/*.plugin")
if(existingPlugins)
  file(REMOVE ${existingPlugins})
endif()

# Add a config file to the install bin directory so that QT can find the plugin libraries.
file(WRITE "${CMAKE_BINARY_DIR}/qt.conf" "[Paths]\nPlugins=../3rdParty/plugins/\n")
install(FILES "${CMAKE_BINARY_DIR}/qt.conf" DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)

# Some tests rely on a hardcoded path to $ISISROOT/bin, so set up a 
#  link to make that work.
if(NOT EXISTS ${CMAKE_SOURCE_DIR}/bin)
  execute_process(COMMAND ln -s "${CMAKE_BINARY_DIR}/src" bin
                  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
endif()

# Create an xml folder in the source directory that we will need later
set(sourceXmlFolder ${CMAKE_SOURCE_DIR}/bin/xml)
execute_process(COMMAND mkdir -p ${CMAKE_BINARY_DIR}/src/xml)

# Set up install of the templates folder.
install(DIRECTORY ${CMAKE_SOURCE_DIR}/templates DESTINATION .)

# Have CMake process all of the source code and tests.
add_subdirectory(src)

# Set up third party libraries for installation
install_third_party()

# Some tests rely on a hardcoded path to $ISISROOT/lib, so set up a 
#  link to make that work.
if(NOT EXISTS ${CMAKE_SOURCE_DIR}/lib)
  execute_process(COMMAND ln -s "${CMAKE_BINARY_DIR}/src" lib
                  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
endif()

# Create a link from the build/src directory to each folder in 3rdPartyLibs/plugins.
# - This is required so that the plugins can be found during unit tests.
# TIP: Set "export QT_DEBUG_PLUGINS=1" to help debug plugin errors.
foreach(plugin ${THIRDPARTYPLUGINFOLDERS})
  get_filename_component(pluginName ${plugin} NAME)
  if(NOT EXISTS ${CMAKE_BINARY_DIR}/src/${pluginName})
    execute_process(COMMAND ln -s "${plugin}" ${pluginName}
                    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src")
  endif()
endforeach()

# Set up documentation build target.
# - This script is called by running "make docs".
# - This long call passes all desired variables to the script.
add_custom_target(docs COMMAND ${CMAKE_COMMAND} 
                  -DPROJECT_SOURCE_DIR=${PROJECT_SOURCE_DIR}  
                  -DDOXYGEN=${DOXYGEN}  -DXALAN=${XALAN} 
                  -DLATEX=${LATEX} 
                  -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX} 
                  -P ${CMAKE_MODULE_PATH}/BuildDocs.cmake)

# Add a custom build target to clean out everything that gets added to the source
#  directory during the build process.
# - Only a few things are added in order to make the tests work properly so
#   this is very straightforward.
add_custom_target(clean_source COMMAND rm -rf ${CMAKE_SOURCE_DIR}/bin; 
                                       rm -rf ${CMAKE_SOURCE_DIR}/lib;)

# Set up a few top level files for installation.
install(FILES IsisPreferences DESTINATION ${CMAKE_INSTALL_PREFIX})
install(FILES license.txt     DESTINATION ${CMAKE_INSTALL_PREFIX})
install(FILES version         DESTINATION ${CMAKE_INSTALL_PREFIX})

# Trigger all post-install behavior.
# - The only way to run commands post-install in CMake is to add a subdirectory at
#   the end of this file containing a CMakeLists.txt file which includes all of 
#   the desired post-install commands inside.
add_subdirectory(cmake)


