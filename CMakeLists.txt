cmake_minimum_required(VERSION 3.5)
cmake_policy(SET CMP0076 NEW)

# Has to be before project()
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING
        "Choose type of build, options are: Debug Release RelWithDebInfo MinSizeRel")
endif(NOT CMAKE_BUILD_TYPE)

project(libicd
    DESCRIPTION "AHTSE Image Codec Library"
    VERSION 0.9.1
    LANGUAGES CXX C
)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
option(BUILD_SHARED_LIBS "Library style" On)

option(BUILD_TESTING "Build tests" Off)

set(namespace "AHTSE")

set(JP12_SOURCES
    jcapimin.c
    jcapistd.c
    jccoefct.c
    jccolor.c
    jcdctmgr.c
    jchuff.c
    jcinit.c
    jcmainct.c
    jcmarker.c
    jcmaster.c
    jcomapi.c
    jcparam.c
    jcphuff.c
    jcprepct.c
    jcsample.c
    jctrans.c
    jdapimin.c
    jdapistd.c
    jdatadst.c
    jdatasrc.c
    jdcoefct.c
    jdcolor.c
    jddctmgr.c
    jdhuff.c
    jdinput.c
    jdmainct.c
    jdmarker.c
    jdmaster.c
    jdmerge.c
    jdphuff.c
    jdpostct.c
    jdsample.c
    jdtrans.c
    jerror.c
    jfdctflt.c
    jfdctfst.c
    jfdctint.c
    jidctflt.c
    jidctfst.c
    jidctint.c
    jidctred.c
    jmemmgr.c
    jmemnobs.c
    jquant1.c
    jquant2.c
    jutils.c
)
list(TRANSFORM JP12_SOURCES PREPEND src/jpeg12-6b/)
# set_source_files_properties(${JP12_SOURCES} PROPERTIES LANGUAGE CXX)

set(JP12_HEADERS
    jchuff.h
    jconfig.h
    jdct.h
    jdhuff.h
    jerror.h
    jinclude.h
    jmemsys.h
    jmorecfg.h
    jpegint.h
    jpeglib.h
    jversion.h
)
list(TRANSFORM JP12_HEADERS PREPEND src/jpeg12-6b/)

set(ICD_SOURCES
    icd_codecs.cpp
    JPEG_codec.cpp
    JPEG8.cpp
    JPEG12.cpp
    LERC_codec.cpp
    Packer_RLE.cpp
    PNG_codec.cpp
    lerc1/Lerc1Image.cpp
)

set(ICD_HEADERS
    BitMask2D.h
    icd_codecs.h
    JPEG_codec.h
    lerc1/Lerc1Image.h
)

find_package(JPEG REQUIRED)
find_package(PNG REQUIRED)
find_package(libQB3) # libQB3_INCLUDE_DIRS libQB3_LIBRARIES

if (${libQB3_FOUND})
    add_compile_definitions(LIBQB3_FOUND)
    list(APPEND ICD_SOURCES QB3_codec.cpp)
endif ()

list(TRANSFORM ICD_SOURCES PREPEND src/)
list(APPEND ICD_SOURCES ${JP12_SOURCES})

list(TRANSFORM ICD_HEADERS PREPEND src/)
list(APPEND ICD_HEADERS ${JP12_HEADERS})

add_library(${PROJECT_NAME})
if (MSVC)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
    target_link_options(${PROJECT_NAME} PUBLIC "/NODEFAULTLIB:LIBCMT")
endif (MSVC)

target_sources(${PROJECT_NAME} PRIVATE ${ICD_SOURCES} ${ICD_HEADERS})

target_include_directories(${PROJECT_NAME}
    PRIVATE
        # Where the library itself looks for internal headers
        ${JPEG_INCLUDE_DIRS} ${PNG_INCLUDE_DIRS}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(${PROJECT_NAME} PRIVATE ${JPEG_LIBRARIES} ${PNG_LIBRARIES} QB3::libQB3)

include(GenerateExportHeader)
generate_export_header(${PROJECT_NAME})

set_target_properties(${PROJECT_NAME} PROPERTIES 
    PUBLIC_HEADER "src/icd_codecs.h;${CMAKE_CURRENT_BINARY_DIR}/libicd_export.h"
    DEBUG_POSTFIX "d"
    PREFIX ""
)

include(GNUInstallDirs)

install(TARGETS ${PROJECT_NAME}
    EXPORT "${PROJECT_NAME}Targets"
    # PUBLIC_HEADER
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT "${PROJECT_NAME}Targets"
    FILE "${PROJECT_NAME}Targets.cmake"
    NAMESPACE ${namespace}::
    DESTINATION cmake
)

include(CMakePackageConfigHelpers)

# generate the version file for the config file
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
    VERSION "${version}"
    COMPATIBILITY AnyNewerVersion
)
# create config file
configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
    INSTALL_DESTINATION cmake
)
# install config files
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
    DESTINATION cmake
)
# generate the export targets for the build tree
export(EXPORT "${PROJECT_NAME}Targets"
    FILE "${CMAKE_CURRENT_BINARY_DIR}/cmake/${PROJECT_NAME}Targets.cmake"
    NAMESPACE ${namespace}::
)

if (BUILD_TESTING)
include(CTest)
    add_executable(testicd testicd.cpp)
    target_link_libraries(testicd PRIVATE libicd)

    add_test(NAME testpng COMMAND testicd image/png)
    add_test(NAME testjpeg COMMAND testicd image/jpeg)
    add_test(NAME testlerc COMMAND testicd raster/lerc)

endif ()
