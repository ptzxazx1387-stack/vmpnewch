cmake_minimum_required(VERSION 3.20)
project(VMP-Cheat LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(/W3 /MT)
file(GLOB_RECURSE SRC_FILES "src/*.cpp" "src/*.h")
list(FILTER SRC_FILES EXCLUDE REGEX ".*/steam_proxy\\.cpp$")
list(FILTER SRC_FILES EXCLUDE REGEX ".*/injector/.*")
set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/vendor/imgui")
set(MINHOOK_DIR "${CMAKE_SOURCE_DIR}/vendor/minhook")
set(IMGUI_SOURCES
    "${IMGUI_DIR}/imgui.cpp" "${IMGUI_DIR}/imgui_demo.cpp"
    "${IMGUI_DIR}/imgui_draw.cpp" "${IMGUI_DIR}/imgui_tables.cpp"
    "${IMGUI_DIR}/imgui_widgets.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_dx11.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_win32.cpp"
)
set(MINHOOK_SOURCES
    "${MINHOOK_DIR}/src/hook.c" "${MINHOOK_DIR}/src/trampoline.c"
    "${MINHOOK_DIR}/src/buffer.c"
    "${MINHOOK_DIR}/src/hde/hde32.c" "${MINHOOK_DIR}/src/hde/hde64.c"
)
add_library(${PROJECT_NAME} SHARED
    ${SRC_FILES} ${IMGUI_SOURCES} ${MINHOOK_SOURCES}
    "vendor/kiero/kiero.cpp"
)
target_include_directories(${PROJECT_NAME} PRIVATE
    "${IMGUI_DIR}" "${IMGUI_DIR}/backends"
    "${MINHOOK_DIR}/include" "vendor/kiero" "src"
)
target_link_libraries(${PROJECT_NAME} d3d11.lib dxgi.lib d3dcompiler.lib winmm.lib)
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "vmp_cheat" SUFFIX ".dll")
add_definitions(-DNOMINMAX -DWIN32_LEAN_AND_MEAN -D_CRT_SECURE_NO_WARNINGS)

# Injector executable
add_executable(injector src/injector/injector.cpp)
target_compile_options(injector PRIVATE /W3 /MT)
target_link_libraries(injector kernel32.lib)