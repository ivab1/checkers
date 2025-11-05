cmake_minimum_required(VERSION 3.10)
project(Checkers)

# Добавляем исходные файлы
file(GLOB SOURCES "*.c")
add_executable(${PROJECT_NAME} ${SOURCES})

# Поиск OpenGL
find_package(OpenGL REQUIRED)

# Проверка операционной системы
if(WIN32)
    set(GLFW_DIR "${CMAKE_SOURCE_DIR}/libs/glfw")

    # Путь к заголовочным файлам GLFW
    include_directories("${GLFW_DIR}/include")

    # Путь к библиотекам GLFW
    find_library(GLFW_LIBRARY
        NAMES glfw3 glfw
        PATHS "${GLFW_DIR}/lib"
        NO_DEFAULT_PATH
    )

    # Линкуем библиотеки
    target_link_libraries(Checkers PRIVATE OpenGL::GL ${GLFW_LIBRARY})
elseif(UNIX)
    find_package(glfw3 REQUIRED)
    target_link_libraries(Checkers PRIVATE OpenGL::GL glfw m)
endif()