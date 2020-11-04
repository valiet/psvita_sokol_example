## This file is a quick tutorial on writing CMakeLists for targeting the Vita
cmake_minimum_required(VERSION 2.8)

## This includes the DolceSDK toolchain, must go before project definition
# It is a convenience so you do not have to type
# -DCMAKE_TOOLCHAIN_FILE=$DOLCESDK/share/dolce.toolchain.cmake for cmake. It is
# highly recommended that you include this block for all projects.
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{DOLCESDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{DOLCESDK}/share/dolce.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define DOLCESDK to point to your SDK path!")
  endif()
endif()

## Define project parameters here
# Name of the project
project(sokol_example)

# This line adds DolceSDK CMake macros, must go after project definition in order
# to build Vita specific artifacts (self/vpk).
include("${DOLCESDK}/share/dolce.cmake" REQUIRED)


set(PSVITAIP "192.168.178.32")

## Configuration options for this app
# Display name (under bubble in LiveArea)
set(APP_NAME "Sokol Example")
# Unique ID must be exactly 9 characters. Recommended: XXXXYYYYY where X =
# unique string of developer and Y = a unique number for this app
set(APP_ID  "SKGL20001")
# Optional version string to show in LiveArea's more info screen
set(APP_VERSION  "01.00")

## Flags and includes for building
# Note that we make sure not to overwrite previous flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11 -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
# Optional. You can specify more param.sfo flags this way.
set(DOLCE_MKSFOEX_FLAGS "${DOLCE_MKSFOEX_FLAGS} -d PARENTAL_LEVEL=1")

# Add any additional include paths here
# include_directories( ../include )

# Add any additional library paths here
# ${CMAKE_CURRENT_BINARY_DIR} lets you use any library currently being built
# link_directories( ${CMAKE_CURRENT_BINARY_DIR} )

## Build and link
# Add all the files needed to compile here
add_executable(${PROJECT_NAME}
  src/main.c
)

# Library to link to (drop the -l prefix). This will mostly be stubs.
target_link_libraries(${PROJECT_NAME}
  pib

  SceLibKernel_stub
  SceThreadmgr_stub
  SceModulemgr_stub
  SceSysmodule_stub
  SceIofilemgr_stub
  SceGxm_stub
  SceLibc_stub
  SceLibm_stub

  SceTouch_stub
  SceCtrl_stub
)

# Piglet needs an SceLibc heap size of at least 3MB to intialize. Without it,
# the module will fail to start. This heap needs to be larger if using -nostdlib 
# as this becomes your main heap.
set(DOLCE_ELF_CREATE_FLAGS "${DOLCE_ELF_CREATE_FLAGS} -h 4194304")

## Create Vita files
dolce_create_self(${PROJECT_NAME}.self ${PROJECT_NAME} UNSAFE)
# The FILE directive lets you add additional files to the VPK, the syntax is
# FILE src_path dst_path_in_vpk. In this case, we add the LiveArea paths.
dolce_create_vpk(${PROJECT_NAME}.vpk ${APP_ID} ${PROJECT_NAME}.self
  VERSION ${APP_VERSION}
  NAME ${APP_NAME}
  FILE sce_sys/icon0.png sce_sys/icon0.png
  FILE sce_sys/livearea/contents/bg.png sce_sys/livearea/contents/bg.png
  FILE sce_sys/livearea/contents/startup.png sce_sys/livearea/contents/startup.png
  FILE sce_sys/livearea/contents/template.xml sce_sys/livearea/contents/template.xml
)

add_custom_target(vpksend
	COMMAND curl -T ${PROJECT_NAME}.vpk ftp://${PSVITAIP}:1337/ux0:/data/
	DEPENDS ${PROJECT_NAME}.vpk
)

add_custom_target(send
	COMMAND curl -T ${PROJECT_NAME}.self ftp://${PSVITAIP}:1337/ux0:/app/${APP_ID}/eboot.bin
	DEPENDS ${PROJECT_NAME}.self
)