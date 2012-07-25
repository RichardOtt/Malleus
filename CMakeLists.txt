# CMakeLists for Malleus
# Author: N. Oblath

# minimum cmake version 2.6 required by the scripts to get Git information
cmake_minimum_required (VERSION 2.6)

# specify the local directory for CMake modules
set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")


# define the project
project (Malleus)

# define the variables to describe the package (will go in the MalleusConfig.hh file)
set (${PROJECT_NAME}_VERSION_MAJOR 0)
set (${PROJECT_NAME}_VERSION_MINOR 1)
set (${PROJECT_NAME}_REVISION 0)
set (${PROJECT_NAME}_VERSION "${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.${${PROJECT_NAME}_REVISION}")
set (${PROJECT_NAME}_FULL_VERSION "${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR} (${${PROJECT_NAME}_REVISION})")
set (${PROJECT_NAME}_PACKAGE_NAME "${CMAKE_PROJECT_NAME}")
set (${PROJECT_NAME}_PACKAGE_STRING "${CMAKE_PROJECT_NAME} ${${PROJECT_NAME}_FULL_VERSION}")


# Setup the default install prefix
# This gets set to the binary directory upon first configuring.
# If the user changes the prefix, but leaves the flag OFF, then it will remain as the user specified.
# If the user wants to reset the prefix to the default (i.e. the binary directory), then the flag should be set ON.
if (NOT DEFINED SET_INSTALL_PREFIX_TO_DEFAULT)
    set (SET_INSTALL_PREFIX_TO_DEFAULT ON)
endif (NOT DEFINED SET_INSTALL_PREFIX_TO_DEFAULT)
if (SET_INSTALL_PREFIX_TO_DEFAULT)
    set (CMAKE_INSTALL_PREFIX ${PROJECT_BINARY_DIR} CACHE PATH "Install prefix" FORCE)
    set (SET_INSTALL_PREFIX_TO_DEFAULT OFF CACHE BOOL "Reset default install path when when configuring" FORCE)
endif (SET_INSTALL_PREFIX_TO_DEFAULT)

# install subdirectories
set (INCLUDE_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/include" CACHE PATH "Install directory for headers")
set (LIB_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/lib" CACHE PATH "Install directory for libraries")
set (BIN_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/bin" CACHE PATH "Install directory for binaries")
set (CONFIG_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/config" CACHE PATH "Install directory for config files")

# build shared libraries
set (BUILD_SHARED_LIBS ON)

# setup the ROOT install
find_package (ROOT 5.28 REQUIRED)
if (ROOT_FOUND)
    add_definitions(-DROOT_FOUND)
endif (ROOT_FOUND)
include_directories (${ROOT_INCLUDES})
set (LIBS ${LIBS} ${ROOT_LIBRARIES})

# setup the log4cxx install
#find_package (LOG4CXX)
#if (LOG4CXX_FOUND)
#    message (STATUS "Log4cxx found")
#    list (APPEND LIBS ${LOG4CXX_LIBRARY})
#    include_directories (${LOG4CXX_INCLUDE_DIRS})
#endif (LOG4CXX_FOUND)


# configuration header file
#configure_file (
#    "${CMAKE_CURRENT_SOURCE_DIR}/MalleusConfig.hh.in"
#    "${PROJECT_BINARY_DIR}/MalleusConfig.hh"
#)

# install location for the configuration header
#install (FILES "${PROJECT_BINARY_DIR}/MalleusConfig.hh" DESTINATION include)

# add the binary tree to the search path for include files so that the config file is found
#include_directories ("${PROJECT_BINARY_DIR}")


####################################################
# Header files
####################################################

set (MALLEUS_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Bkgd.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ConfigFile.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Decider.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Errors.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Flux.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/FunctionDefs.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/MCMC.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/metaReader.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Pdf1D.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Pdf3D.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/PdfParent.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/RealFunction.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Sys.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Tools.h
)

install (FILES ${MALLEUS_HEADERS}  DESTINATION ${INCLUDE_INSTALL_DIR})


####################################################
# Source files for different executables
####################################################

set (MALLEUS_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/src/MCMC.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/src/PdfParent.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Pdf1D.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Pdf3D.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Sys.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Flux.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Bkgd.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ConfigFile.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Errors.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Tools.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/src/RealFunction.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/src/metaReader.cxx
)

set (MALLEUS_DRAW_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/drawResults.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/src/MCMC.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/src/PdfParent.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Pdf1D.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Pdf3D.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Sys.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Flux.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Bkgd.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ConfigFile.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Errors.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Tools.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/src/RealFunction.cxx
)

set (MALLEUS_META_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/metaConfig.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/src/metaReader.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Errors.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Tools.cxx
)

set (MALLEUS_GENERATE_MAP_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/generateMap.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Errors.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Tools.cxx
)


####################################################
# Executables
####################################################

add_custom_target (doc
    ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating API documentation with Doxygen" VERBATIM
)

add_executable (generateMap.exe ${MALLEUS_GENERATE_MAP_SRC})
target_link_libraries (generateMap.exe ${LIBS})

add_executable (Malleus ${MALLEUS_SRC})
target_link_libraries (Malleus ${LIBS})

add_executable (drawResults.exe ${MALLEUS_DRAW_SRC})
target_link_libraries (drawResults.exe ${LIBS})

add_executable (metaConfig.exe ${MALLEUS_META_SRC})
target_link_libraries (metaConfig.exe ${LIBS})

add_executable (getAutoCorr.exe ${CMAKE_CURRENT_SOURCE_DIR}/src/getAutoCorr.C)
target_link_libraries (getAutoCorr.exe ${LIBS})

add_executable (autoFit.exe ${CMAKE_CURRENT_SOURCE_DIR}/src/autoFit.C)
target_link_libraries (autoFit.exe ${LIBS})

install (TARGETS generateMap.exe
                 Malleus
                 drawResults.exe
                 metaConfig.exe
                 getAutoCorr.exe
                 autoFit.exe
         DESTINATION ${BIN_INSTALL_DIR})



# Doxygen documentation can be built with "make doc" (it's not included with the "all" make target)
# By default it builds in the source tree.
#add_subdirectory (Documentation/ReferenceGuide)


# this is apparently not the right way to run the link_dylibs script
#execute_process (COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/Scripts/link_dylibs.sh ${PROJECT_BINARY_DIR}/lib)

#install (CODE "execute_process (COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/Scripts/link_dylibs.sh ${PROJECT_BINARY_DIR}/lib)")
