
if(CMAKE_CROSSCOMPILING)
    include(FetchContent)
    if(WIN32)
        FetchContent_Declare(
            windows_jdk
            URL "https://github.com/adoptium/temurin25-binaries/releases/download/jdk-25.0.4.1%2B1/OpenJDK25U-jdk_x64_windows_hotspot_25.0.4.1_1.zip"
            URL_HASH SHA256=00c847d804f4a78e9f04f2683faf14fed898535b177b7fc704486cb0284e9283
            SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/openjdk"
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        )
        FetchContent_MakeAvailable(windows_jdk)

        set(JAVA_HOME "${windows_jdk_SOURCE_DIR}")
        set(JAVA_INCLUDE_PATH "${windows_jdk_SOURCE_DIR}/include")
        set(JAVA_INCLUDE_PATH2 "${windows_jdk_SOURCE_DIR}/include/win32")
        set(JAVA_JVM_LIBRARY "${windows_jdk_SOURCE_DIR}/lib/jvm.lib")
    elseif(LINUX)
        FetchContent_Declare(
            linux_jdk
            URL "https://github.com/adoptium/temurin25-binaries/releases/download/jdk-25.0.4.1%2B1/OpenJDK25U-jdk_x64_linux_hotspot_25.0.4.1_1.tar.gz"
            URL_HASH SHA256=dbb698396d478e7fa2b1e50f4103324b2a99b90569ee27c33f2261f9215cf41e
            SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/openjdk"
            DOWNLOAD_EXTRACT_TIMESTAMP FALSE
        )
        FetchContent_MakeAvailable(linux_jdk)

        set(JAVA_HOME "${linux_jdk_SOURCE_DIR}")
        set(JAVA_INCLUDE_PATH "${linux_jdk_SOURCE_DIR}/include")
        set(JAVA_INCLUDE_PATH2 "${linux_jdk_SOURCE_DIR}/include/linux")
        set(JAVA_JVM_LIBRARY "${linux_jdk_SOURCE_DIR}/lib/jvm.lib")
    endif()
endif()

find_package(JNI REQUIRED COMPONENTS JVM)