cmake_minimum_required(VERSION 3.10)
project(Starwave3D)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Option to use vcpkg or manual dependencies
option(USE_VCPKG "Use vcpkg for dependencies" OFF)

# Find OpenGL
find_package(OpenGL REQUIRED)

# Source files
set(SOURCES
    main.cpp
    Game.cpp
    PlayerShip.cpp
    Texture.cpp
    CameraController.cpp
    LevelManager.cpp
    Level1OuterDriftZone.cpp
    Level2SolarDebrisPath.cpp
    Asteroid.cpp
    SmallAsteroid.cpp
    Bullet.cpp
    Collectible.cpp
    WavePortal.cpp
    BlackHolePortal.cpp
    SimpleTextRenderer.cpp
    Skybox.cpp
    SatelliteFragment.cpp
    SwingSolarPanel.cpp
    CometRock.cpp
    Level2Collectible.cpp
    IntroScreen.cpp
)

set(HEADERS
    Game.h
    PlayerShip.h
    Shader.h
    Camera.h
    CameraController.h
    Texture.h
    BaseLevel.h
    LevelManager.h
    Level1OuterDriftZone.h
    Level2SolarDebrisPath.h
    Asteroid.h
    SmallAsteroid.h
    Bullet.h
    Collectible.h
    WavePortal.h
    BlackHolePortal.h
    SimpleTextRenderer.h
    Skybox.h
    SatelliteFragment.h
    SwingSolarPanel.h
    CometRock.h
    Level2Collectible.h
    IntroScreen.h
)

# Try to find packages
if(USE_VCPKG)
    find_package(glfw3 CONFIG REQUIRED)
    find_package(glad CONFIG REQUIRED)
    find_package(glm CONFIG REQUIRED)
else()
    # Manual dependency paths
    set(GLFW_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/glfw" CACHE PATH "GLFW directory")
    set(GLAD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/glad" CACHE PATH "GLAD directory")
    set(GLM_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/glm" CACHE PATH "GLM directory")
    set(STB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/stb" CACHE PATH "STB directory")
    
    # Add GLAD source file if it exists
    if(EXISTS "${GLAD_DIR}/src/glad.c")
        list(APPEND SOURCES "${GLAD_DIR}/src/glad.c")
    endif()
endif()

# Add executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

if(USE_VCPKG)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        OpenGL::GL
        glfw
        glad::glad
        glm::glm
    )
else()
    target_include_directories(${PROJECT_NAME} PRIVATE
        ${GLFW_DIR}/include
        ${GLAD_DIR}/include
        ${GLM_DIR}
        ${STB_DIR}
    )
    
    # Link libraries - adjust for your system
    if(MINGW)
        target_link_libraries(${PROJECT_NAME} PRIVATE
            OpenGL::GL
            ${GLFW_DIR}/lib-mingw-w64/libglfw3.a
        )
    elseif(WIN32)
        target_link_libraries(${PROJECT_NAME} PRIVATE
            OpenGL::GL
            ${GLFW_DIR}/lib-vc2022/glfw3.lib  # Adjust based on your Visual Studio version
            mfplat.lib
            mfreadwrite.lib
            mfuuid.lib
            propsys.lib
            winmm.lib
        )
    else()
        target_link_libraries(${PROJECT_NAME} PRIVATE
            OpenGL::GL
            ${GLFW_DIR}/lib/libglfw3.a
            ${CMAKE_DL_LIBS}
        )
    endif()
endif()

# Copy shaders to build directory
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/shaders DESTINATION ${CMAKE_BINARY_DIR})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/shaders DESTINATION ${CMAKE_BINARY_DIR}/Release)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/shaders DESTINATION ${CMAKE_BINARY_DIR}/Debug)

# Copy textures to build directory (when you add them)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/textures)
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/textures DESTINATION ${CMAKE_BINARY_DIR})
endif()

# Copy Intro folder (videos and audio) to build directories
if(EXISTS ${CMAKE_BINARY_DIR}/Intro)
    file(COPY ${CMAKE_BINARY_DIR}/Intro DESTINATION ${CMAKE_BINARY_DIR}/Release)
    file(COPY ${CMAKE_BINARY_DIR}/Intro DESTINATION ${CMAKE_BINARY_DIR}/Debug)
endif()

# Platform-specific settings
if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)
    # Add console for debug output on Windows
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE FALSE
    )
elseif(APPLE)
    target_compile_definitions(${PROJECT_NAME} PRIVATE GL_SILENCE_DEPRECATION)
endif()
