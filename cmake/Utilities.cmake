
# Small space saving functions
function(copy_file src dest)
  configure_file(${src} ${dest} COPYONLY)
endfunction()

function(copy_folder src dest)
  execute_process(COMMAND cp -r ${src} ${dest})
endfunction()

# Use this function to copy all files matching a wildcard to the output folder.
function(copy_wildcard wildcard outputFolder)
  file(GLOB files ${wildcard})
  file(COPY ${files} DESTINATION ${outputFolder})
endfunction()

function(verify_file_exists path)
  if(NOT EXISTS ${path})
    message( FATAL_ERROR "Required file ${path} does not exist!" )
  endif()
endfunction()

function(file_contains path s result)
  file(READ ${path} contents)
  string(FIND "${contents}" "${s}" position)
  set(${result} ON PARENT_SCOPE)
  if(${position} EQUAL -1)
    set(${result} OFF PARENT_SCOPE)
  endif()
endfunction()

# Set up a symlink file for installation
#function(install_symlink link target)
#  install(CODE "EXECUTE_PROCESS(COMMAND cmake -E create_symlink ${link} ${target})")
#endfunction()

# This macro returns a list of all the subdirectories in the given directory
MACRO(SUBDIRLIST curdir result)
  FILE(GLOB children RELATIVE ${curdir} ${curdir}/*)
  SET(dirlist "")
  FOREACH(child ${children})
    # Skip files and hidden folders.
    string(SUBSTRING ${child} 0 1 firstChar)
    IF( (IS_DIRECTORY ${curdir}/${child}) AND (NOT ${firstChar} STREQUAL ".") )
      LIST(APPEND dirlist ${curdir}/${child})
    ENDIF()
  ENDFOREACH()
  SET(${result} ${dirlist})
ENDMACRO()

# Append the contents of IN_FILE to the end of OUT_FILE
function(cat IN_FILE OUT_FILE)

  # If the output file does not exist, init with an empty file.
  if(NOT EXISTS "${OUT_FILE}")
    file(WRITE ${OUT_FILE} "")
  endif()

  # Perform the file concatenation.
  file(READ ${IN_FILE} CONTENTS)
  file(APPEND ${OUT_FILE} "${CONTENTS}")
endfunction()


# Get the correct location to generate code for an input folder
MACRO(get_code_gen_dir inputFolder outputFolder)
  file(RELATIVE_PATH relPath ${PROJECT_SOURCE_DIR} ${inputFolder})
  set(${outputFolder} "${PROJECT_BINARY_DIR}/${relPath}")
  file(MAKE_DIRECTORY ${${outputFolder}})
  # Also add this folder to the include path
  include_directories(${${outputFolder}})
ENDMACRO()

# Copy all input files to the output folder
function(copy_files_to_folder files folder)
  foreach(f ${files})
    get_filename_component(filename ${f} NAME)
    set(outputPath "${folder}/${filename}")
    configure_file(${f} ${outputPath} COPYONLY)
  endforeach()
endfunction()

# Determine the text string used to describe this OS version
function(get_os_version text)
  
  if(UNIX AND NOT APPLE)
  
    # Fetch OS information
    execute_process(COMMAND cat "/etc/os-release"
                    OUTPUT_VARIABLE result
                    ERROR_VARIABLE result)
    #message("result = ${result}")
  
    # Extract OS name and version
    string(REGEX MATCH "NAME=[A-Za-z\"]+" name "${result}")
    string(REGEX MATCH "VERSION_ID=[0-9\\.\"]+" version "${result}")
    string(SUBSTRING ${name} 6 -1 name)
    string(SUBSTRING ${version} 12 -1 version)
    string(REPLACE "\"" "" name ${name})
    string(REPLACE "\"" "" version ${version})
    string(REPLACE "." "_" version ${version})
    #message("name = ${name}")
    #message("version = ${version}")

    set(prefix "Linux_x86_64_")
  
  # Build the final output string
  elseif(APPLE)

    # Fetch OS information
    execute_process(COMMAND sw_vers
                    OUTPUT_VARIABLE result
                    ERROR_VARIABLE result)


    string(REGEX MATCH "[0-9]+.[0-9]+.[0-9]+" version "${result}")
    string(REGEX MATCH "^[0-9]+.[0-9]+" version "${version}")
    string(REPLACE "." "_" version ${version})
    
    set(name   "MacOSX")
    set(prefix "Darwin_")

  else()
    message( FATAL_ERROR "Did not recognize a supported operating system!" )
  endif()
  
  set(${text} ${prefix}${name}${version} PARENT_SCOPE)
endfunction()

#------------------------------------------------------------
# Standard function to add a library and its components
function(add_library_wrapper name sourceFiles libDependencies)

  #get_property(inc_dirs DIRECTORY PROPERTY INCLUDE_DIRECTORIES)
  #message("inc_dirs = ${inc_dirs}")

  # The only optional argument is "alsoStatic", which indicates that
  #  the library should be build both shared and static.
  set(alsoStatic ${ARGN})

  # Add library, set dependencies, and add to installation list.
  add_library(${name} SHARED ${sourceFiles})
  set_target_properties(${name} PROPERTIES LINKER_LANGUAGE CXX) 
  target_link_libraries(${name} ${libDependencies})
  install(TARGETS ${name} DESTINATION lib)
    
  if(alsoStatic)
    # The static version needs a different name, but in the end the file
    # needs to have the same name as the shared lib.
    set(staticName "${name}_static") 
    message("Adding static library ${staticName}")
    add_library("${staticName}" STATIC ${sourceFiles})
    set_target_properties(${staticName} PROPERTIES LINKER_LANGUAGE CXX) 
    target_link_libraries(${staticName} ${libDependencies})
    # Use a copy -> install combo to get the file to the correct place.
    add_custom_command(TARGET ${staticName} POST_BUILD 
                       COMMAND mv ${CMAKE_BINARY_DIR}/src/lib${staticName}.a
                                  ${CMAKE_BINARY_DIR}/src/lib${name}.a)
                                  #${CMAKE_INSTALL_PREFIX}/lib/lib${name}.a)
    install(CODE "EXECUTE_PROCESS(COMMAND cp ${CMAKE_BINARY_DIR}/src/lib${name}.a 
                                             ${CMAKE_INSTALL_PREFIX}/lib/lib${name}.a)")
  endif()


  # Set all the header files to be installed to the include directory
  foreach(f ${sourceFiles})
    get_filename_component(extension ${f} EXT) # Get file extension  
    string( TOLOWER "${extension}" extensionLower )
    if( extensionLower STREQUAL ".h" OR extensionLower STREQUAL ".hpp" OR extensionLower STREQUAL ".tcc")
      set(fullPath "${CMAKE_CURRENT_SOURCE_DIR}/${f}") # TODO: Check this!
      #message("Install include file ${f} to inc")
      INSTALL(FILES ${f} DESTINATION inc)
    endif()
  endforeach(f)

endfunction(add_library_wrapper)

#------------------------------------------------------------

# TODO: Can we consolidate the following three functions?

# Generate ui_*.h files from *.ui files using QT tool uic.
function( generate_ui_files UI_GEN_OUT folder)

  # Finds all .ui files in the current dir
  file(GLOB UI_INPUT "${folder}/*.ui")

  # If no .ui files in this folder we are finished.
  list(LENGTH UI_INPUT numFiles)
  if (${numFiles} EQUAL 0)
    set(${UI_GEN_OUT} "" PARENT_SCOPE)
    return()
  endif()

  #message("FOUND UI FILES ${UI_INPUT}")

  # Set where generated files go to and add that directory to the include path
  get_code_gen_dir(${folder} UI_GEN_DIR)

  # For each input ui file
  foreach(UI_FILE ${UI_INPUT})
    # Get the name of the file without extension
    get_filename_component(UI_NAME ${UI_FILE} NAME_WE)
    
    # Add the generated file to UI_GEN variable
    set(OUT_UI_FILE "${UI_GEN_DIR}/ui_${UI_NAME}.h")
    set(UI_GEN       ${UI_GEN} ${OUT_UI_FILE})

    # Add the custom command that will generate this file
    # - The generated files will be put in the CMake build directory, 
    #   not the source tree    
    add_custom_command(OUTPUT   ${OUT_UI_FILE}
                       COMMAND  ${UIC}  ${UI_FILE} -o ${OUT_UI_FILE}
                       DEPENDS  ${UI_FILE}
                       WORKING_DIRECTORY ${folder}
                       COMMENT "Generating UI headers...")

  endforeach()

  #message("UI Output files: ${UI_GEN}")
  set(${UI_GEN_OUT} ${UI_GEN} PARENT_SCOPE) # Set up output variable

endfunction(generate_ui_files)

#------------------------------------------------------------

# Generate moc_*.cpp files from *.h files using Q_OBJECT using the moc tool.
function( generate_moc_files MOC_GEN_OUT folder)

  # Finds all .h files in the current dir including the text Q_OBJECT
  file(GLOB CANDIDATE_FILES "${folder}/*.h")
  set(MOC_INPUT)
  foreach(f ${CANDIDATE_FILES})
    execute_process(COMMAND grep Q_OBJECT ${f}
                    OUTPUT_VARIABLE result
                    RESULT_VARIABLE code)
    if("${code}" STREQUAL "0")
      set(MOC_INPUT ${MOC_INPUT} ${f})
    endif()
  endforeach()


  # If no Q_OBJECT files in this folder we are finished.
  list(LENGTH MOC_INPUT numFiles)
  if (${numFiles} EQUAL 0)
    set(${MOC_GEN_OUT} "" PARENT_SCOPE)
    return()
  endif()

  #message("FOUND MOC FILES ${MOC_INPUT}")

  # Set where generated files go to and add that directory to the include path
  get_code_gen_dir(${folder} MOC_GEN_DIR)
  #message("MOC_GEN_DIR = ${MOC_GEN_DIR}")

  # For each input moc file
  foreach(MOC_FILE ${MOC_INPUT})
    # Get the name of the file without extension
    get_filename_component(MOC_NAME ${MOC_FILE} NAME_WE)
    
    # Add the generated file to MOC_GEN variable
    set(OUT_MOC_FILE "${MOC_GEN_DIR}/moc_${MOC_NAME}.cpp")
    set(MOC_GEN       ${MOC_GEN} ${OUT_MOC_FILE})
    #message("OUT_MOC_FILE = ${OUT_MOC_FILE}")

    # Add the custom command that will generate this file
    # - The generated files will be put in the CMake build directory, 
    #   not the source tree    
    add_custom_command(OUTPUT   ${OUT_MOC_FILE}
                       COMMAND  ${MOC}  ${MOC_FILE} -o ${OUT_MOC_FILE}
                       DEPENDS  ${MOC_FILE}
                       WORKING_DIRECTORY ${folder}
                       COMMENT "Generating MOC files...")

  endforeach()

  #message("MOC Output files: ${MOC_GEN}")
  set(${MOC_GEN_OUT} ${MOC_GEN} PARENT_SCOPE) # Set up output variable

endfunction(generate_moc_files)


#------------------------------------------------------------

# Generate ProtoBuf output files for an obj folder.
# ${PROTOC} must point to the protobuf tool
function(generate_protobuf_files PROTO_GEN_OUT folder)

  # Finds all .proto files in the current dir
  file(GLOB PROTO_INPUT "${folder}/*.proto")

  # If no .proto files in this folder we are finished.
  list(LENGTH PROTO_INPUT numFiles)
  if (${numFiles} EQUAL 0)
    set(${PROTO_GEN_OUT} "" PARENT_SCOPE)
    return()
  endif()

  #message("FOUND PROTOBUFF FILES ${PROTO_INPUT}")


  # Set where generated files go to and add that directory to the include path
  get_code_gen_dir(${folder} PROTO_GEN_DIR)

  # For each input protobuf file
  foreach(PROTO_FILE ${PROTO_INPUT})
    # Get the name of the file without extension
    get_filename_component(PROTO_NAME ${PROTO_FILE} NAME_WE)
    
    # Add the two generated files to PROTO_GEN variable
    set(PROTO_GEN ${PROTO_GEN}
        ${PROTO_GEN_DIR}/${PROTO_NAME}.pb.h
        ${PROTO_GEN_DIR}/${PROTO_NAME}.pb.cc)
  endforeach()

  # Add the custom command that will generate all the files
  # - The generated files will be put in the CMake build directory, not the source tree.
  #message("Protobuff Output files: ${PROTO_GEN}")
  add_custom_command(OUTPUT   ${PROTO_GEN}
                     COMMAND  ${PROTOC} --proto_path ${folder} --cpp_out ${PROTO_GEN_DIR} ${PROTO_INPUT}
                     DEPENDS  ${PROTO_INPUT}
                     WORKING_DIRECTORY ${folder}
                     COMMENT "Generating Protocol Buffers...")

  set(${PROTO_GEN_OUT} ${PROTO_GEN} PARENT_SCOPE) # Set up output variable
endfunction(generate_protobuf_files)









