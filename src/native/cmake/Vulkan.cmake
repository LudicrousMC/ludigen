
if(NOT CMAKE_CROSSCOMPILING)
    find_package(Vulkan REQUIRED)
else()
    include(FetchContent)
    # Retrieve vulkan headers
    FetchContent_Declare(
        windows_vulkan_headers
        URL "https://github.com/KhronosGroup/Vulkan-Headers/archive/refs/tags/v1.4.363.zip"
        SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/vulkan-headers"
        SOURCE_SUBDIR "IGNORE_SUBDIRECTORY"
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(windows_vulkan_headers)
    set(VULKAN_HEADERS_DIR "${windows_vulkan_headers_SOURCE_DIR}/include")

    add_library(Vulkan::Vulkan SHARED IMPORTED GLOBAL)

    # Retrieve platform-dependent vulkan loader
    if(WIN32)
        FetchContent_Declare(
            windows_vulkan
            URL "https://sdk.lunarg.com/sdk/download/1.4.363.0/windows/VulkanRT-X64-1.4.363.0-Components.zip"
            URL_HASH SHA256=a25a927aa8b9f0371048f1861cf88ac3b9bc9b1fb332c42d897c8ab32695769a
            SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/vulkan"
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        )
        FetchContent_MakeAvailable(windows_vulkan)

        find_program(GENDEF_EXECUTABLE NAMES gendef x86_64-w64-mingw32-gendef)
        if(NOT GENDEF_EXECUTABLE)
            message(FATAL_ERROR "Could not find gendef. Ensure mingw-w64 gendef is installed.")
        endif()
        set(VULKAN_DLL "${windows_vulkan_SOURCE_DIR}/x64/vulkan-1.dll")
        set(VULKAN_DEF "${CMAKE_CURRENT_BINARY_DIR}/vulkan-1.def")
        set(VULKAN_LIBRARY "${CMAKE_CURRENT_BINARY_DIR}/generated/libvulkan-1.dll.a")
        add_custom_command(
            OUTPUT ${VULKAN_DEF}
            COMMAND ${GENDEF_EXECUTABLE} ${VULKAN_DLL}
            DEPENDS ${VULKAN_DLL}
        )
        find_program(DLLTOOL_EXECUTABLE NAMES dlltool x86_64-w64-mingw32-dlltool)
        if(NOT DLLTOOL_EXECUTABLE)
            message(FATAL_ERROR "Could not find dlltool. Ensure mingw-w64 dlltool is installed.")
        endif()
        add_custom_command(
            OUTPUT ${VULKAN_LIBRARY}
            COMMAND ${DLLTOOL_EXECUTABLE} -d ${VULKAN_DEF} -l ${VULKAN_LIBRARY}
            DEPENDS ${VULKAN_DEF}
        )
        set_target_properties(Vulkan::Vulkan PROPERTIES
            IMPORTED_IMPLIB ${VULKAN_LIBRARY}
            IMPORTED_LOCATION ${VULKAN_DLL}
            INTERFACE_INCLUDE_DIRECTORIES ${VULKAN_HEADERS_DIR}
        )
    elseif(LINUX)
        FetchContent_Declare(
            linux_vulkan
            URL "https://sdk.lunarg.com/sdk/download/1.4.363.0/linux/vulkansdk-linux-x86_64-1.4.363.0.tar.xz"
            URL_HASH SHA256=197962f5cbf80baf2775a03336a01cee7c8745686c65aaa70d3f751ade4d7e43
            SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/vulkan"
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        )
        FetchContent_MakeAvailable(linux_vulkan)

        set(VULKAN_SO "${linux_vulkan_SOURCE_DIR}/x86_64/lib/VulkanLoader/lib/libvulkan.so")
        set_target_properties(Vulkan::Vulkan PROPERTIES
            IMPORTED_LOCATION ${VULKAN_SO}
            INTERFACE_INCLUDE_DIRECTORIES ${VULKAN_HEADERS_DIR}
        )
    endif()

    add_custom_target(vulkan_import_library DEPENDS "${VULKAN_LIBRARY}")
    add_dependencies(ludicrous_engine vulkan_import_library)
endif()