cmake_minimum_required(VERSION 3.15)
project(Demo CXX)

# Set the path to clang.exe
set(CLANG_EXECUTABLE "C:\\Users\\dol12\\llvm-project\\build\\Debug\\bin\\clang.exe")

# Specify multiple include directories
set(INCLUDE_DIRS
    "C:\\Program Files (x86)\\Windows Kits\\10\\Include\\10.0.22621.0\\um"
    "${CMAKE_SOURCE_DIR}/kernel"
    "${CMAKE_SOURCE_DIR}/../HvcNope"
)

# Set your libtooling tool's executable
set(LIBTOOLING_TOOL "${CMAKE_SOURCE_DIR}/../x64/Debug/Converter.exe")

# Add source files
set(SOURCE_FILES
    ${CMAKE_SOURCE_DIR}/main.cpp
    ${CMAKE_SOURCE_DIR}/kernel/objects.h
)

set(KERNEL_DIR
    ${CMAKE_SOURCE_DIR}/kernel)

# Create a directory for rewritten files
set(REWRITE_DIR "${CMAKE_BINARY_DIR}/rewritten")
file(MAKE_DIRECTORY ${REWRITE_DIR})

# Join all include directories for libtooling tool's command line
set(INCLUDE_FLAGS "")
foreach(INCLUDE_DIR ${INCLUDE_DIRS})
    set(INCLUDE_FLAGS "${INCLUDE_FLAGS} -I${INCLUDE_DIR}")
endforeach()

# Define a custom target to run the libtooling tool on each source file
add_custom_target(run_libtooling ALL
    COMMENT "Running libtooling tool on source files..."
)

# Run libtooling tool on each source file
foreach(SRC ${SOURCE_FILES})
    get_filename_component(FILENAME ${SRC} NAME)
    set(REWRITTEN_FILE "${REWRITE_DIR}/${FILENAME}")

    add_custom_command(
        TARGET run_libtooling
        COMMAND ${LIBTOOLING_TOOL} --kernel-dir=${KERNEL_DIR} ${INCLUDE_FLAGS} ${SRC} > ${REWRITTEN_FILE}
        DEPENDS ${SRC}
        OUTPUT ${REWRITTEN_FILE}
        COMMENT "Rewriting ${FILENAME}"
    )
endforeach()

# Compile the rewritten files using clang
add_executable(DemoExe
    ${REWRITE_DIR}/main.cpp
    ${REWRITE_DIR}/kernel/objects.h
)

# Use clang for compilation
set(CMAKE_CXX_COMPILER ${CLANG_EXECUTABLE})
target_include_directories(MyExecutable PRIVATE ${INCLUDE_DIRS})
