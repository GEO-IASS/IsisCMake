

# Set up Xalan's command-line option names.
set(XALAN_VALIDATE_OPTION "-v")
set(XALAN_OUTFILE_OPTION  "-o")
set(XALAN_PARAM_OPTION    "-p")
set(XALAN_INFILE_OPTION   ""  )
set(XALAN_XSL_OPTION      ""  )

# TODO: How should this be set?
set(MODE "")

#------------------------------------------------------------

# Populate application doc files into "isis/doc/Application/presentation"
function(copy_app_docs_info)

  # Go through all application folders, copy .xml and assets
  SUBDIRLIST("${PROJECT_SOURCE_DIR}/src" moduleFolders)
  foreach(f ${moduleFolders})
    get_filename_component(moduleName ${f} NAME_WE)

    # Only need to process app folders, not obj folders.
    if ((${moduleName} STREQUAL "docsys") OR (NOT EXISTS "${f}/apps"))
      continue() # Skip this folder
    endif()

    file(MAKE_DIRECTORY ${appDataFolder}/${moduleName})

    SUBDIRLIST(${f}/apps appFolders)
    foreach(appF ${appFolders})
      # Each app gets its own folder in the build directory
      get_filename_component(appName ${appF} NAME_WE)
      set(thisDataFolder ${appDataFolder}/${moduleName}/${appName})
      file(MAKE_DIRECTORY ${thisDataFolder})

      # Copy the .xml file and the asset folder if it exists.
      copy_file(${appF}/${appName}.xml ${thisDataFolder}/${moduleName}.xml)
      if(EXISTS ${appF}/assets)
        file(MAKE_DIRECTORY ${thisDataFolder}/assets)
        execute_process(COMMAND cp -r ${appF}/assets ${thisDataFolder})
      endif()
    endforeach() # End loop through apps

  endforeach() # End loop through modules

endfunction()

#------------------------------------------------------------

# Build the top level of the documents directory
function(build_upper_level)

  # Copy existing folders to the install directory
  copy_folder(${docBuildFolder}/assets ${docInstallFolder})
  copy_folder(${docBuildFolder}/w3c    ${docInstallFolder})

  # Make new (empty) output folders
  set(newFolders UserDocs AboutIsis General Guides Installation TechnicalInfo Schemas)
  foreach(f ${newFolders})
    file(MAKE_DIRECTORY "${docInstallFolder}/${f}")
  endforeach()

  # These folders are populated inside "build_documents_folder"

  # Create index.html file
  execute_process(COMMAND ${XALAN} ${XALAN_VALIDATE_OPTION} ${XALAN_PARAM_OPTION} menuPath \"'./'\" ${XALAN_OUTFILE_OPTION} ${docInstallFolder}/index.html ${XALAN_INFILE_OPTION} ${docBuildFolder}/build/homepage.xml ${XALAN_XSL_OPTION} ${docBuildFolder}/build/main.xsl)
endfunction()

#------------------------------------------------------------


# Build src/docsys/documents folder.
function(build_documents_folder)

  message("Building documents folder...")
  message("    Building table of contents XML...")

  # Get list of folders of interest  
  SUBDIRLIST(${docBuildFolder}/documents docFolders)
  set(blacklistFolders  ${docBuildFolder}/documents/ReleaseNotes # Folders we don't want
                        ${docBuildFolder}/documents/ParameterChanges 
                        ${docBuildFolder}/documents/ApiChanges)
  list(REMOVE_ITEM docFolders ${blacklistFolders})

  # Build doctoc.xml, the documents table of contents file.
  set(doctocPath ${docBuildFolder}/build/doctoc.xml)
  file(REMOVE ${doctocPath})
  cat(${docBuildFolder}/build/doctoc_header.xml ${doctocPath})
  foreach(f ${docFolders})
    # Each folder in documents gets a section added to doctoc
    get_filename_component(docName ${f} NAME_WE)
    #message("Processing ${docName}")
    execute_process(COMMAND ${XALAN} ${XALAN_PARAM_OPTION} dirParam \"'${docName}'\"  ${XALAN_INFILE_OPTION} ${f}/${docName}.xml ${XALAN_XSL_OPTION} ${docBuildFolder}/build/IsisDocumentTOCbuild.xsl OUTPUT_VARIABLE result)
    file(APPEND ${doctocPath} ${result})
    #message("result = ${result}")
  endforeach()
  cat(${docBuildFolder}/build/doctoc_footer.xml ${doctocPath})
 
  # TODO: Should this happen first?

  # Build individual documents folders
  message("    Building individual documents...")
  file(MAKE_DIRECTORY ${docInstallFolder}/documents)
  foreach(f ${docFolders})

    message("Building documents folder: ${f}")

    # Handle paths for this folder
    get_filename_component(docName ${f} NAME_WE)
    set(thisOutputFolder ${docInstallFolder}/documents/${docName})
    file(MAKE_DIRECTORY ${thisOutputFolder})

    # TODO: Verify for each output document!!!
    # Make Xalan call to generate the .html output file from the .xml input file
    # - The original makefiles used an intermediate temporary makefile in this step.

    set(xalanCommand ${XALAN} ${XALAN_PARAM_OPTION} menuPath "'../../'" ${XALAN_PARAM_OPTION} filenameParam "'${docName}.html'" ${XALAN_OUTFILE_OPTION} ${thisOutputFolder}/${docName}.html ${XALAN_INFILE_OPTION} ${f}/${docName}.xml  ${XALAN_XSL_OPTION} ${docBuildFolder}/docsys/build/IsisSubPageBuild.xsl)
    #message("command = ${xalanCommand}")
    execute_process(COMMAND ${xalanCommand})

    # Copy any assets to the install folder
    copy_folder(${f}/assets ${thisOutputFolder}/assets)

  endforeach()

  #message( FATAL_ERROR "STOP." )

  message("    Building table of contents files...")
  # These go in top level folders in /doc/

  # ABOUT ISIS TOC
  execute_process(COMMAND ${XALAN} ${XALAN_PARAM_OPTION} menuPath \"'../'\" ${XALAN_OUTFILE_OPTION} ${docInstallFolder}/AboutIsis/index.html   ${XALAN_INFILE_OPTION} ${dotocPath} ${XALAN_XSL_OPTION} ${docBuildFolder}/build/AboutIsis.xsl)

  # GENERAL TOC
  execute_process(COMMAND ${XALAN} ${XALAN_PARAM_OPTION} menuPath \"'../'\" ${XALAN_OUTFILE_OPTION} ${docInstallFolder}/General/index.html ${XALAN_INFILE_OPTION} ${dotocPath} ${XALAN_XSL_OPTION} ${docBuildFolder}/build/General.xsl)

  # GUIDES TOC
  execute_process(COMMAND ${XALAN} ${XALAN_PARAM_OPTION} menuPath \"'../'\" ${XALAN_OUTFILE_OPTION} ${docInstallFolder}/Guides/index.html ${XALAN_INFILE_OPTION} ${dotocPath} ${XALAN_XSL_OPTION} ${docBuildFolder}/build/Guides.xsl)

  # INSTALLATION TOC
  execute_process(COMMAND ${XALAN} ${XALAN_PARAM_OPTION} menuPath \"'../'\" ${XALAN_OUTFILE_OPTION} ${docInstallFolder}/Installation/index.html ${XALAN_INFILE_OPTION} ${dotocPath} ${XALAN_XSL_OPTION} ${docBuildFolder}/build/Installation.xsl)

  # TECHNICAL INFO TOC
  execute_process(COMMAND ${XALAN} ${XALAN_PARAM_OPTION} menuPath \"'../'\" ${XALAN_OUTFILE_OPTION} ${docInstallFolder}/TechnicalInfo/index.html ${XALAN_INFILE_OPTION} ${dotocPath} ${XALAN_XSL_OPTION} ${docBuildFolder}/build/TechnicalInfo.xsl)

  # USER DOCS TOC
  execute_process(COMMAND ${XALAN} ${XALAN_PARAM_OPTION} menuPath \"'../'\" ${XALAN_OUTFILE_OPTION} ${docInstallFolder}/UserDocs/index.html    ${XALAN_INFILE_OPTION} ${dotocPath} ${XALAN_XSL_OPTION} ${docBuildFolder}/build/UserDocs.xsl)


endfunction()


#------------------------------------------------------------

# Supporting files should already be in /src/docsys/Application
function(build_application_docs)

  # Is there any reason not to just generate all these files from their original 
  #  locations instead of copying them to a temporary build directory?

  set(appFolder            "${docBuildFolder}/Application")
  set(printerStyleFolder   "${appFolder}/presentation/PrinterFriendly/styles")
  set(tabbedStyleFolder    "${appFolder}/presentation/Tabbed/styles")

  set(installAppFolder     "${docInstallFolder}/Application")
  set(installPrinterFolder "${installAppFolder}/presentation/PrinterFriendly")
  set(installTabbedFolder  "${installAppFolder}/presentation/Tabbed")

  # Make output directories and copy the styles
  file(MAKE_DIRECTORY "${installPrinterFolder}")
  file(MAKE_DIRECTORY "${installTabbedFolder}")
  file(MAKE_DIRECTORY "${installPrinterFolder}/styles")
  file(MAKE_DIRECTORY "${installTabbedFolder}/styles")
  execute_process(cp "${printerStyleFolder}/*.css" ${installPrinterFolder}/styles/)
  execute_process(cp "${tabbedStyleFolder}/*.css" ${installTabbedFolder}/styles/)

  # Loop through module folders
  SUBDIRLIST(${appDataFolder} moduleFolders)
  foreach(mod ${moduleFolders})

    get_filename_component(moduleName ${mod} NAME_WE)

    # Loop through application folders
    SUBDIRLIST(${mod} appDataFolders)
    foreach(f ${appDataFolders})

      get_filename_component(appName ${f} NAME_WE)
      message("Processing application folder: ${appName}")

      # Get printer-friendly and tabbed output folders
      set(pfAppFolder ${installPrinterFolder}/${appName})
      set(tbAppFolder ${installTabbedFolder}/${appName})
      file(MAKE_DIRECTORY "${pfAppFolder}")
      file(MAKE_DIRECTORY "${tbAppFolder}")

      copy_folder(${f}/assets ${pfAppFolder}/assets)
      copy_folder(${f}/assets ${tbAppFolder}/assets)

      execute_process(COMMAND ${XALAN} ${XALAN_PARAM_OPTION} menuPath \"'../../../../'\" ${XALAN_OUTFILE_OPTION} ${pfAppFolder}/${appName}.html ${XALAN_INFILE_OPTION} ${f}/${moduleName}.xml ${XALAN_XSL_OPTION} ${printerStyleFolder}/IsisApplicationDocStyle.xsl)

      execute_process(COMMAND ${XALAN} ${XALAN_PARAM_OPTION} menuPath \"'../../../../'\" ${XALAN_OUTFILE_OPTION} ${tbAppFolder}/${appName}.html ${XALAN_INFILE_OPTION} ${f}/${moduleName}.xml ${XALAN_XSL_OPTION} ${tabbedStyleFolder}/IsisApplicationDocStyle.xsl)

    endforeach() # End loop through app folders

  endforeach() # End loop through module folders

  # Make the table of contents that goes in the /bin/xml folder

  # Set up the file  
  set(appTocPath "${CMAKE_INSTALL_PREFIX}/bin/xml/applicationTOC.xml")
  file(REMOVE ${appTocPath})
  cat(${docBuildFolder}/Application/build/toc_header.xml ${appTocPath})
  SUBDIRLIST(${appDataFolder} moduleFolders)

  # Loop through module folders
  foreach(mod ${moduleFolders})
    get_filename_component(moduleName ${mod} NAME_WE)

    # Loop through application folders
    SUBDIRLIST(${mod} appDataFolders)
    foreach(f ${appDataFolders})

      # Use Xalan to generate a piece of the TOC and append it to the file
      get_filename_component(docName ${f} NAME_WE)
      execute_process(COMMAND ${XALAN} ${XALAN_INFILE_OPTION} ${f}/${moduleName}.xml ${XALAN_XSL_OPTION} ${docBuildFolder}/Application/build/IsisApplicationTOCbuild.xsl)
      file(APPEND ${appTocPath} ${result})
      #message("result = ${result}")
    endforeach()
  endforeach()
  # Append the footer to complete the TOC file!
  cat(${docBuildFolder}/Application/build/toc_footer.xml ${appTocPath})

endfunction()


#------------------------------------------------------------

# Use the application TOC file to build some other TOCs
function(add_extra_tocs)

  set(TOCDIR      "${docInstallFolder}/Application")
  set(buildFolder "${docBuildFolder}/Application/build")
  set(tocXml      "${CMAKE_INSTALL_PREFIX}/bin/xml/applicationTOC.xml")

  # Build alpha.html
  execute_process(COMMAND ${XALAN} ${XALAN_PARAM_OPTION} menuPath \"'../'\" ${XALAN_OUTFILE_OPTION} ${TOCDIR}/alpha.html ${XALAN_INFILE_OPTION} ${tocXml} ${XALAN_XSL_OPTION} ${buildFolder}/TOCindex_alpha.xsl)

  # Build index.html
  execute_process(COMMAND ${XALAN} ${XALAN_PARAM_OPTION} menuPath \"'../'\" ${XALAN_OUTFILE_OPTION} ${TOCDIR}/index.html ${XALAN_INFILE_OPTION} ${tocXml} ${XALAN_XSL_OPTION} ${buildFolder}/TOCindex_category.xsl)

  # Build oldvnew.html
  execute_process(COMMAND ${XALAN} ${XALAN_PARAM_OPTION} menuPath \"'../'\" ${XALAN_OUTFILE_OPTION} ${TOCDIR}/oldvnew.html ${XALAN_INFILE_OPTION} ${tocXml} ${XALAN_XSL_OPTION} ${buildFolder}/TOCindex_oldvnew.xsl)

  # Build applicationCategories.xml
  execute_process(COMMAND ${XALAN} ${XALAN_OUTFILE_OPTION} ${CMAKE_INSTALL_PREFIX}/bin/xml/applicationCategories.xml ${XALAN_INFILE_OPTION} ${docBuildFolder}/Schemas/Application/application.xsd ${XALAN_XSL_OPTION} ${buildFolder}/IsisApplicationCategoriesbuild.xsl)

endfunction()

#------------------------------------------------------------

# Set up three Doxygen configuration files
function(build_object_conf)

  message("Building apps configuration...")

  # Make a list of each object folder with an assets folder
  SUBDIRLIST(moduleFolders ${PROJECT_SOURCE_DIR})
  set(OBJECTASSETS)
  foreach(mod ${moduleFolders})
    SUBDIRLIST(objFolders ${mod}/objs)
    foreach(obj ${objFolders})
      if(EXISTS ${obj}/assets)
        set(OBJECTASSETS ${OBJECTASSETS} ${obj}/assets)
      endif()
    endforeach() # End obj loop
  endforeach() # End module loop

  set(objConfDir ${docBuildFolder}/src/docsys/Object/build)
  file(MAKE_DIRECTORY ${objConfDir}/apps)

  # The three conf files start from an input base file and append more options
  set(appsConf       ${objConfDir}/apps_tag_temp.conf  )
  set(programmerConf ${objConfDir}/Programmer_temp.conf)
  set(developerConf  ${objConfDir}/Developer_temp.conf )
  set(docInstallDir  ${docInstallFolder}/Object )

  # Copy settings files from the source folder to the build folder
  execute_process(cp "${PROJECT_SOURCE_DIR}/src/docsys/Object/build/*" ${objConfDir})

  # Append to the app conf file
  cat(${objConfDir}/apps_tag.conf ${appsConf})
  file(APPEND ${appsConf} "LATEX_CMD_NAME   = ${LATEX}\n")
  file(APPEND ${appsConf} "OUTPUT_DIRECTORY = ${docInstallDir}\n")
  file(APPEND ${appsConf} "STRIP_FROM_PATH  = ${CMAKE_INSTALL_PREFIX}/\n")
  file(APPEND ${appsConf} "INPUT            = ${CMAKE_INSTALL_PREFIX}/src/ \\\n")
  file(APPEND ${appsConf} "                   ${objConfDir}/isisAppsDoxyDefs.doxydef\n")
  file(APPEND ${appsConf} "HTML_HEADER      = ${objConfDir}/IsisObjectHeaderApps.html\n")
  file(APPEND ${appsConf} "HTML_FOOTER      = ${objConfDir}/IsisObjectFooter.html\n")
  file(APPEND ${appsConf} "HTML_OUTPUT      = apps\n")
  file(APPEND ${appsConf} "HTML_STYLESHEET  = ${objConfDir}/doxygen_apps.css\n")

  if(NOT ${DOT_PATH} STREQUAL "")
    file(APPEND ${appsConf} "DOT_PATH  = /opt/local/bin\n")
  endif()

  # Append to the programmer conf file
  cat(${objConfDir}/Programmer.conf ${programmerConf})
  file(APPEND ${programmerConf} "OUTPUT_DIRECTORY = ${docInstallDir}\n")
  file(APPEND ${programmerConf} "FILE_PATTERNS    = *objs*.h \ \n")
  file(APPEND ${programmerConf} "                   *objs*.cpp \ \n")
  file(APPEND ${programmerConf} "                   *build/isisDoxyDefs.doxydef\n")
  file(APPEND ${programmerConf} "STRIP_FROM_PATH  = ${CMAKE_INSTALL_PREFIX}/\n")
  file(APPEND ${programmerConf} "INPUT            = ${objConfDir}/isisDoxyDefs.doxydef ${CMAKE_INSTALL_PREFIX}/src/ \n")
  file(APPEND ${programmerConf} "HTML_HEADER      = ${objConfDir}/IsisObjectHeaderProgrammers.html\n")
  file(APPEND ${programmerConf} "HTML_FOOTER      = ${objConfDir}/IsisObjectFooter.html\n")
  file(APPEND ${programmerConf} "HTML_OUTPUT      = Programmer\n")
  file(APPEND ${programmerConf} "TAGFILES         = ${objConfDir}/apps/apps_fix.tag\n")
  file(APPEND ${programmerConf} "HTML_STYLESHEET  = ${objConfDir}/doxygen_prog.css\n")
  file(APPEND ${programmerConf} "IMAGE_PATH       = \n")

  string(FIND "${MODE}" "LOUD" pos)
  if (NOT ${pos} STREQUAL "-1")
    file(APPEND ${programmerConf} "QUIET                  = NO\n")
    file(APPEND ${programmerConf} "WARNINGS               = YES\n")
    file(APPEND ${programmerConf} "WARN_IF_UNDOCUMENTED   = YES\n")
    file(APPEND ${programmerConf} "WARN_IF_DOC_ERROR      = YES\n")
    file(APPEND ${programmerConf} "WARN_NO_PARAMDOC       = YES\n")
  else()
    file(APPEND ${programmerConf} "QUIET                  = YES\n")
    file(APPEND ${programmerConf} "WARN_IF_UNDOCUMENTED   = YES\n")
    file(APPEND ${programmerConf} "WARN_IF_DOC_ERROR      = YES\n")
    file(APPEND ${programmerConf} "WARN_NO_PARAMDOC       = YES\n")
  endif()

  if (NOT ${DOT_PATH} STREQUAL "")
    file(APPEND ${programmerConf} "DOT_PATH  = /opt/local/bin\n")
  endif()

  foreach(dirname ${OBJECTASSETS})
    file(APPEND ${programmerConf} "${dirname} \\\n")
  endforeach()

  # Append to the developer conf file
  cat(${objConfDir}/Developer.conf ${developerConf})
  file(APPEND ${developerConf} "LATEX_CMD_NAME   = ${LATEX}\n")
  file(APPEND ${developerConf} "OUTPUT_DIRECTORY = ${docInstallDir}\n")
  file(APPEND ${developerConf} "STRIP_FROM_PATH  = ${CMAKE_INSTALL_PREFIX}/\n")
  file(APPEND ${developerConf} "INPUT            = ${CMAKE_INSTALL_PREFIX}/src/ \\\n")
  file(APPEND ${developerConf} "                   ${objConfDir}/isisDoxyDefs.doxydef\n")
  file(APPEND ${developerConf} "HTML_HEADER      = ${objConfDir}/IsisObjectHeader.html\n")
  file(APPEND ${developerConf} "HTML_FOOTER      = ${objConfDir}/IsisObjectFooter.html\n")
  file(APPEND ${developerConf} "HTML_OUTPUT      = Developer\n")
  file(APPEND ${developerConf} "TAGFILES         = ${objConfDir}/apps/apps_fix.tag\n")
  file(APPEND ${developerConf} "HTML_STYLESHEET  = ${objConfDir}/doxygen.css\n")
  file(APPEND ${developerConf} "IMAGE_PATH       = \n")
  string(FIND "${MODE}" "LOUD" pos)
  if (NOT ${pos} STREQUAL "-1")
    file(APPEND ${developerConf} "QUIET                  = NO\n")
    file(APPEND ${developerConf} "WARNINGS               = YES\n")
    file(APPEND ${developerConf} "WARN_IF_UNDOCUMENTED   = YES\n")
    file(APPEND ${developerConf} "WARN_IF_DOC_ERROR      = YES\n")
    file(APPEND ${developerConf} "WARN_NO_PARAMDOC       = YES\n")
  else()
    file(APPEND ${developerConf} "QUIET                  = YES\n")
    file(APPEND ${developerConf} "WARNINGS               = NO\n")
    file(APPEND ${developerConf} "WARN_IF_UNDOCUMENTED   = NO\n")
    file(APPEND ${developerConf} "WARN_IF_DOC_ERROR      = NO\n")
    file(APPEND ${developerConf} "WARN_NO_PARAMDOC       = NO\n")
  endif()

  foreach(dirname ${OBJECTASSETS})
    file(APPEND ${developerConf} "${dirname} \\\n")
  endforeach()

endfunction()



function(build_object_docs)

  # TODO: Install doxygen and test!

  # Create app, developer, and programmer Doxygen configuration files.
  build_object_conf()

  # TODO: Do prog_tester conf here as well?

  set(objConfDir  ${docBuildFolder}/src/docsys/Object/build)
  #set(objBuildDir ${docBuildFolder}/Object/build)

  message("Copying assets...")
  file(MAKE_DIRECTORY "${docInstallFolder}/Object")
  execute_process(cp -r ${docBuildFolder}/Object/assets ${docInstallFolder}/Object/)

  # TODO: Check no double assets folder?

  message("Creating Object Documentation")
  file(MAKE_DIRECTORY ${docInstallFolder}/Object/apps)
  file(MAKE_DIRECTORY ${docInstallFolder}/Object/Developer)
  file(MAKE_DIRECTORY ${docInstallFolder}/Object/Programmer)
  file(MAKE_DIRECTORY ${docInstallFolder}/documents/DocStyle/assets)
  execute_process(cp "${docBuildFolder}/Object/*.html" ${docInstallFolder}/Object/)
  #copy_file(${objBuildDir}/isisDoxyDefs.doxydef ${docInstallFolder}/documents/DocStyle/assets/isisDoxyDefs.doxydef)


  message("Building apps documentation")
  execute_process(COMMAND ${DOXYGEN} "${objConfDir}/apps_tag_temp.conf"
                  WORKING_DIRECTORY ${docBuildFolder}/src/docsys/Object/)
  execute_process(COMMAND ${XALAN} ${XALAN_OUTFILE_OPTION} ${objConfDir}/apps/apps_fix.tag ${XALAN_INFILE_OPTION} ${objConfDir}/apps/apps.tag ${XALAN_XSL_OPTION} ${objConfDir}/IsisApplicationTagFix.xsl)
  copy_file(${objConfDir}/apps/apps_fix.tag ${docInstallFolder}/Object/apps/apps_fix.tag)


  message("Building Programmer documentation.")
  execute_process(COMMAND ${DOXYGEN} "${objConfDir}/Programmer_temp.conf"
                  WORKING_DIRECTORY ${docBuildFolder}/src/docsys/Object/)

  # TODO: Make sure there are no Latex lib conflicts
  message("Building Developer documentation...")
  execute_process(COMMAND ${DOXYGEN} "${objConfDir}/Developer_temp.conf"
                  WORKING_DIRECTORY ${docBuildFolder}/src/docsys/Object/)


endfunction()

#------------------------------------------------------------
# Build all the documentation
function(build_docs)

  message("Building Isis Documentation...")

  # Set up output directory and a temporary directory for building
  set(docBuildFolder   ${CMAKE_BINARY_DIR}/docBuild) #TODO: Is this needed???
  set(appDataFolder    ${docBuildFolder}/Application/data) # TODO: What is this for?
  set(docInstallFolder ${CMAKE_INSTALL_PREFIX}/doc) # Final output documentation

  # Clean up existing files
  execute_process(rm -rf ${docBuildFolder})
  execute_process(rm -rf ${docInstallFolder})

  # Copy everything from src/docsys to docBuildFolder
  execute_process(cp -r ${PROJECT_SOURCE_DIR}/src/docsys ${docBuildFolder})

  file(MAKE_DIRECTORY "${docBuildFolder}/Application")
  file(MAKE_DIRECTORY "${docBuildFolder}/Application/data")
  file(MAKE_DIRECTORY "${docInstallFolder}")

  message("Copying application information...")
  copy_app_docs_info()

  message("Building upper level directories...")
  build_upper_level()

  build_documents_folder()

  message("Building application docs...")
  build_application_docs()

  message("Building additional TOCs...")
  add_extra_tocs()

  message("Building object documentation")
  build_object_docs()

endfunction()
