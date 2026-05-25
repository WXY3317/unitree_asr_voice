cmake_minimum_required(VERSION 3.0.2)
project(unitree_asr_voice)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(catkin REQUIRED COMPONENTS
  roscpp
  std_msgs
)

catkin_package()

# Bundled SDK (vendor/unitree_sdk2). No external unitree_sdk2 path required.
set(UNITREE_SDK_ROOT ${CMAKE_CURRENT_LIST_DIR}/vendor/unitree_sdk2)
set(UNITREE_THIRDPARTY_LIB_DIR ${UNITREE_SDK_ROOT}/thirdparty/lib/${CMAKE_SYSTEM_PROCESSOR})
set(UNITREE_CORE_LIB_DIR ${UNITREE_SDK_ROOT}/lib/${CMAKE_SYSTEM_PROCESSOR})

if(NOT EXISTS "${UNITREE_SDK_ROOT}/include")
  message(FATAL_ERROR
    "Bundled SDK missing: ${UNITREE_SDK_ROOT}/include "
    "(run scripts/update_vendor_sdk.sh to populate vendor/)")
endif()

if(NOT EXISTS "${UNITREE_CORE_LIB_DIR}/libunitree_sdk2.a")
  message(FATAL_ERROR
    "No libunitree_sdk2.a for ${CMAKE_SYSTEM_PROCESSOR} under ${UNITREE_CORE_LIB_DIR}")
endif()

include_directories(
  ${catkin_INCLUDE_DIRS}
  ${UNITREE_SDK_ROOT}/include
  ${UNITREE_SDK_ROOT}/thirdparty/include
  ${UNITREE_SDK_ROOT}/thirdparty/include/ddscxx
)

link_directories(
  ${UNITREE_THIRDPARTY_LIB_DIR}
  ${UNITREE_CORE_LIB_DIR}
)

add_executable(unitree_asr_voice_node src/unitree_asr_voice_node.cpp)
target_link_libraries(unitree_asr_voice_node
  ${catkin_LIBRARIES}
  unitree_sdk2
  ddscxx
  ddsc
  pthread
)

# devel: RPATH into package vendor; install: .so next to catkin lib/, node uses $ORIGIN/..
set_target_properties(unitree_asr_voice_node PROPERTIES
  BUILD_RPATH "${UNITREE_THIRDPARTY_LIB_DIR};${UNITREE_CORE_LIB_DIR}"
  INSTALL_RPATH "\$ORIGIN/.."
  INSTALL_RPATH_USE_LINK_PATH TRUE
)

install(TARGETS unitree_asr_voice_node
  RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
)

install(DIRECTORY ${UNITREE_THIRDPARTY_LIB_DIR}/
  DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
  FILES_MATCHING
  PATTERN "*.so"
  PATTERN "*.so.*"
)

install(DIRECTORY launch config
  DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION}
)
