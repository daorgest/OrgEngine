cmake_minimum_required(VERSION 3.28)

project(OrgEngine VERSION 0.1)
# Set C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Optionally define the flag based on some condition
option(USE_DIRECTX11 "Use DirectX 11 Renderer" FALSE)
option(USE_VULKAN "Use Vulkan Renderer" TRUE)


add_definitions(-DVULKAN_BUILD)


# Ensure we are using wide characters on Windows universally
add_definitions(-DUNICODE -D_UNICODE)
add_compile_definitions(VK_USE_PLATFORM_WIN32_KHR)
add_definitions(-DNOMINMAX)

# Set the MSVC debug information format for supporting Edit and Continue
if(MSVC)
  # Release Flags
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /DNDEBUG")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Ot /GT /arch:AVX2")

  # Debug flags
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Od /ZI /DEBUG")
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<CONFIG:Debug,RelWithDebInfo>,EditAndContinue,ProgramDatabase>")
endif()

# Globbing source files
file(GLOB_RECURSE SOURCE_FILES "src/*.cpp")

# Add source to this project's executable
add_executable(OrgEngine ${SOURCE_FILES})

# Find Vulkan package
find_package(Vulkan REQUIRED)

# Include and link only necessary libraries
add_subdirectory(libs/3rdParty/fmt)
add_subdirectory(libs/3rdParty/glm)
add_subdirectory(libs/3rdParty/vk-bootstrap)
add_subdirectory(libs/3rdParty/vma)
add_subdirectory(libs/3rdParty/fastgltf)

set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs/3rdParty/imgui")
file(GLOB IMGUI_SOURCES
        "${IMGUI_DIR}/*.cpp"
        "${IMGUI_DIR}/*.h"
        "${IMGUI_DIR}/backends/imgui_impl_win32.cpp"
        "${IMGUI_DIR}/backends/imgui_impl_win32.h"
        "${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp"
        "${IMGUI_DIR}/backends/imgui_impl_vulkan.h")

# Include directories
target_include_directories(OrgEngine PRIVATE
        ${Vulkan_INCLUDE_DIRS}
        "${CMAKE_SOURCE_DIR}/3rdParty/vma/"
        "${IMGUI_DIR}"
#        libs/3rdParty/stb
)

# Add ImGui sources to the executable
target_sources(OrgEngine PRIVATE ${IMGUI_SOURCES})
option(FASTGLTF_COMPILE_AS_CPP20 "Compile FastGLTF as C++20" ON)

# Link libraries
target_link_libraries(OrgEngine PRIVATE
        xinput.lib  # for now
        Vulkan::Vulkan
        fmt::fmt
        vk-bootstrap::vk-bootstrap
        GPUOpen::VulkanMemoryAllocator
        glm::glm-header-only
        fastgltf
)

target_compile_definitions(OrgEngine PRIVATE
        $<$<CONFIG:Debug>:DEBUG>
)

# Custom function to compile GLSL shaders
function(add_shaders TARGET SHADER_DIR)
  file(GLOB_RECURSE SHADER_FILES
          "${SHADER_DIR}/*.comp"
          "${SHADER_DIR}/*.frag"
          "${SHADER_DIR}/*.vert")

  set(SHADER_OUTPUTS "")
  foreach(SHADER_FILE ${SHADER_FILES})
    get_filename_component(SHADER_NAME ${SHADER_FILE} NAME)
    set(SHADER_OUTPUT "${CMAKE_BINARY_DIR}/App/shaders/${SHADER_NAME}.spv")

    add_custom_command(
            OUTPUT ${SHADER_OUTPUT}
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/App/shaders"
            COMMAND glslc ${SHADER_FILE} -o ${SHADER_OUTPUT}
            DEPENDS ${SHADER_FILE}
            COMMENT "Compiling ${SHADER_NAME} to ${SHADER_OUTPUT}"
    )

    list(APPEND SHADER_OUTPUTS ${SHADER_OUTPUT})
  endforeach()

  add_custom_target(${TARGET}_Shaders ALL DEPENDS ${SHADER_OUTPUTS})
  add_dependencies(${TARGET} ${TARGET}_Shaders)
endfunction()

# Add shader compilation for shaders in 'src/Shaders' directory
add_shaders(OrgEngine "${CMAKE_CURRENT_SOURCE_DIR}/src/Shaders")

# Function to copy assets
function(copy_assets TARGET ASSET_DIR OUTPUT_DIR)
  file(GLOB_RECURSE ASSET_FILES "${ASSET_DIR}/*")

  add_custom_command(
          TARGET ${TARGET}
          PRE_BUILD
          COMMAND ${CMAKE_COMMAND} -E make_directory "${OUTPUT_DIR}"
          COMMAND ${CMAKE_COMMAND} -E copy_directory "${ASSET_DIR}" "${OUTPUT_DIR}"
          COMMENT "Copying assets from ${ASSET_DIR} to ${OUTPUT_DIR}"
  )
endfunction()

# Copy assets to the output directory
copy_assets(OrgEngine "${CMAKE_CURRENT_SOURCE_DIR}/src/Assets" "${CMAKE_BINARY_DIR}/App/assets")

# Set the output directories for the executable and compiled shaders
set_target_properties(OrgEngine PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/App"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/App"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/App"
)

# Create a target for compiling shaders
add_custom_target(Shaders DEPENDS ${SHADER_OUTPUT})

# Add Shaders target as a dependency of OrgEngine
add_dependencies(OrgEngine Shaders)

if(${CMAKE_CXX_FLAGS_RELEASE})
  include(InstallRequiredSystemLibraries)
  set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/License.txt")
  set(CPACK_PACKAGE_VERSION_MAJOR "${Tutorial_VERSION_MAJOR}")
  set(CPACK_PACKAGE_VERSION_MINOR "${Tutorial_VERSION_MINOR}")
  set(CPACK_GENERATOR "TGZ")
  set(CPACK_SOURCE_GENERATOR "TGZ")
  include(CPack)
endif()
