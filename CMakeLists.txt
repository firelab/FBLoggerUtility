CMAKE_MINIMUM_REQUIRED(VERSION 3.1)

PROJECT(fb_logger_utility)

ADD_DEFINITIONS(-DUNICODE -D_UNICODE)

SET(CMAKE_CXX_STANDARD 11)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

# Find includes in the build directories
SET(CMAKE_INCLUDE_CURRENT_DIR ON)

# Turn on automatic invocation of the MOC, UIC & RCC
SET(CMAKE_AUTOMOC ON)
SET(CMAKE_AUTOUIC ON)
SET(CMAKE_AUTORCC ON)

# Make this a GUI application on Windows
IF(WIN32)
  SET(CMAKE_WIN32_EXECUTABLE ON)
ENDIF()

IF(MSVC)
      ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS)
      IF((${CMAKE_BUILD_TYPE} STREQUAL "Release"))
        ADD_DEFINITIONS(-DQT_NO_DEBUG_OUTPUT)
      ENDIF()
ENDIF(MSVC)

#Find OpenMP
INCLUDE(FindOpenMP)
FIND_PACKAGE(OpenMP)
IF(OPENMP_FOUND)
    ADD_DEFINITIONS(-DOMP_SUPPORT)
    IF(MSVC)
        SET(CMAKE_CXX_FLAGS "${cmake_cxx_flags} ${OpenMP_CXX_FLAGS} /EHsc")
    ELSE(MSVC) # Not using MSVC
        SET(CMAKE_CXX_FLAGS "${cmake_cxx_flags} ${OpenMP_CXX_FLAGS}")
    ENDIF(MSVC)
ELSE(OPENMP_FOUND)
    MESSAGE("OpenMP could not be found on your system, disabling support")
ENDIF(OPENMP_FOUND)

# Find the Qt library
FIND_PACKAGE(Qt6 REQUIRED COMPONENTS Core  Widgets Widgets)
GET_TARGET_PROPERTY(QtCore_location Qt6::Core LOCATION)

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/src
                    ${CMAKE_SOURCE_DIR}/include)

ADD_EXECUTABLE(${PROJECT_NAME}
                src/air.cpp
                src/convert_velocity.cpp
                src/fb_logger_log_file.cpp
                src/fluid.cpp
                src/legacy_logger_data_reader.cpp
                src/logger_data_reader.cpp
                src/gps_file.cpp
                src/gui/logger_data_worker.cpp
                src/gui/logger_data_worker.ui # empty ui file, harmless yet irritating hack to force Visual Studio to automoc class
                src/gui/main.cpp
                src/gui/mainwindow.cpp
                src/air.h
                src/convert_velocity.h
                src/fb_logger_log_file.h
                src/fluid.h
                src/legacy_logger_data_reader.h
                src/logger_data_reader.h
                src/gps_file.h
                src/gui/logger_data_worker.h
                src/gui/logger_data_worker.ui # empty ui file, harmless yet irritating hack to force Visual Studio to automoc class
                src/gui/mainwindow.h
                src/gui/mainwindow.ui)

# Add the Qt Widgets for linking
TARGET_LINK_LIBRARIES(${PROJECT_NAME} Qt6::Widgets)
