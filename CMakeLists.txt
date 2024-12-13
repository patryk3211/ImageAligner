cmake_minimum_required(VERSION 3.18)

project(ImageAlignmentProgram VERSION 0.0.1 LANGUAGES CXX)

include(CTest)
enable_testing()

# Sources
set(COMMON_SRC
  src/io/sequence.cpp
  src/io/fits.cpp
  src/io/provider.cpp

  src/objects/stats.cpp
  src/objects/matrix.cpp
  src/objects/registration.cpp
  src/objects/image.cpp

  src/cv/context.cpp
)

set(EXEC_SOURCE
  src/main.cpp

  src/ui/app.cpp
  src/ui/window.cpp
  src/ui/state.cpp

  src/ui/pages/page.cpp
  src/ui/pages/cv.cpp

  src/ui/widgets/util.cpp
  src/ui/widgets/sequence_list.cpp
  src/ui/widgets/gl_area_plus.cpp
  src/ui/widgets/main_view.cpp
  src/ui/widgets/alignment_view.cpp

  src/ui/widgets/gl/object.cpp
  src/ui/widgets/gl/texture.cpp
  src/ui/widgets/gl/buffer.cpp
  src/ui/widgets/gl/program.cpp
  src/ui/widgets/gl/vao.cpp
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
set(CMAKE_CXX_STANDARD 20)

# Dependencies
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 REQUIRED IMPORTED_TARGET gtk4)
pkg_check_modules(GTKMM gtkmm-4.0)
pkg_check_modules(Adwaita libadwaita-1)

find_package(OpenGL REQUIRED)
find_package(spdlog REQUIRED)
find_package(OpenCV REQUIRED)

# Common library (used for testing functionality)
add_library(common ${COMMON_SRC})

target_include_directories(common PUBLIC
  include
  ${GTKMM_INCLUDE_DIRS}
  ${OpenCV_INCLUDE_DIRS}
)
target_link_directories(common PUBLIC
  ${GTKMM_LIBRARY_DIRS}
)
target_link_libraries(common PUBLIC
  cfitsio
  spdlog::spdlog
  PkgConfig::GTK4
  ${GTKMM_LIBRARIES}
  ${OpenCV_LIBS}
)

# Executable target
add_executable(aligner ${EXEC_SOURCE})

target_include_directories(aligner PRIVATE
  ${Adwaita_INCLUDE_DIRS}
  ${OPENGL_INCLUDE_DIRS}
)

target_link_directories(aligner PRIVATE
  ${Adwaita_LIBRARY_DIRS}
)
target_link_libraries(aligner PRIVATE
  ${Adwaita_LIBRARIES}
  ${OPENGL_LIBRARIES}
)
target_link_libraries(aligner PRIVATE common)

file(CREATE_LINK ${CMAKE_CURRENT_BINARY_DIR}/ui ${CMAKE_CURRENT_SOURCE_DIR}/ui SYMBOLIC)

function(compile_blueprint target input)
  make_directory(${CMAKE_CURRENT_BINARY_DIR}/ui)

  add_custom_target(
    ${input}.ui
    blueprint-compiler compile ${CMAKE_CURRENT_SOURCE_DIR}/resource/scheme/${input} --output ${CMAKE_CURRENT_BINARY_DIR}/ui/${input}.ui
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/resource/scheme/${input}
    COMMENT "Compiling ${input}"
  )

  add_dependencies(${target} ${input}.ui)
endfunction(compile_blueprint)

compile_blueprint(aligner main.blp)
compile_blueprint(aligner cv.blp)

file(CREATE_LINK ${CMAKE_CURRENT_SOURCE_DIR}/resource/css/main.scss ${CMAKE_CURRENT_BINARY_DIR}/ui/main.scss)
file(CREATE_LINK ${CMAKE_CURRENT_SOURCE_DIR}/resource/gl ${CMAKE_CURRENT_BINARY_DIR}/ui/gl SYMBOLIC)

add_subdirectory(test)

