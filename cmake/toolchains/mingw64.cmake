set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(CMAKE_C_COMPILER   x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
set(CMAKE_RC_COMPILER  x86_64-w64-mingw32-windres)

# Where MinGW sysroot lives on Debian/Ubuntu
set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
# Force Win32 API paths for MinGW
add_compile_definitions(_WIN32 WIN32)
set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -include windows.h")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -include windows.h")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static -static-libgcc -static-libstdc++")
add_compile_definitions(NCLI_DISABLE_COLORS)
