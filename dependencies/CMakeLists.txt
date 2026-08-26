set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON  CACHE BOOL "" FORCE)

set(SDL_TESTS  OFF CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_DISABLE_INSTALL ON CACHE BOOL "" FORCE)

set(JSON_BuildTests OFF CACHE INTERNAL "")

add_subdirectory(SDL EXCLUDE_FROM_ALL)
add_subdirectory(Fizziks EXCLUDE_FROM_ALL)
add_subdirectory(nativefiledialog-extended EXCLUDE_FROM_ALL)
add_subdirectory(json EXCLUDE_FROM_ALL)

set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/imgui)

add_library(ImGui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdlrenderer3.cpp
)

target_include_directories(ImGui PUBLIC ${IMGUI_DIR})

# ImGui's SDL3 backend needs SDL3 headers to compile
target_link_libraries(ImGui PRIVATE SDL3::SDL3)
