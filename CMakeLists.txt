
cmake_minimum_required(VERSION 2.8)

PROJECT(VolumeViewer)

find_package(VTK REQUIRED)
include(${VTK_USE_FILE})

add_executable(PerfusionViewer PerfusionViewer)

if(VTK_LIBRARIES)
  target_link_libraries(PerfusionViewer ${VTK_LIBRARIES})
else()
  target_link_libraries(PerfusionViewer vtkHybrid)
endif()
