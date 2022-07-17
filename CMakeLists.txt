cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

project(VRMLConverter)

find_package(VTK COMPONENTS
    CommonCore
    IOExport
    IOLegacy
    RenderingCore
    RenderingContextOpenGL2
    RenderingFreeType
    RenderingGL2PSOpenGL2
    RenderingOpenGL2
)

if (NOT VTK_FOUND)
  message(FATAL_ERROR "Unable to find the VTK build folder.")
endif()

add_executable(${PROJECT_NAME} main.cxx)
target_link_libraries(${PROJECT_NAME} PRIVATE ${VTK_LIBRARIES})

# vtk_module_autoinit is needed
vtk_module_autoinit(
  TARGETS ${PROJECT_NAME}
  MODULES ${VTK_LIBRARIES}
)

