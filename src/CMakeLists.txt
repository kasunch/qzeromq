set (QZMQ_PUBLIC_HEADERS
    qzmqcommon.hpp
#    qzmqexception.hpp
#    qzmqcontext.hpp
#    qzmqmessage.hpp
#    qzmqsocket.hpp
#    qzmq.hpp
)

set (QZMQ_SOURCES
#    qzmqexception.cpp
#    qzmqcontext.cpp
#    qzmqmessage.cpp
#    qzmqsocket.cpp
#    qzmqeventloophook.cpp
)

list(APPEND target_outputs "")

if(BUILD_SHARED)
    add_library(libqzmq SHARED ${QZMQ_SOURCES})
    list(APPEND target_outputs "libqzmq")
    # We are building a shared library
    set_target_properties(libqzmq 
        PROPERTIES
        COMPILE_DEFINITIONS "QZMQ_DLL;QZMQ_DLL_EXPORTS"
    )
    target_link_libraries(libqzmq PRIVATE Qt5::Core)
    if(ZMQ_SHARED)
        target_link_libraries(libqzmq PRIVATE ${ZeroMQ_LIBRARY})
    else()
        target_link_libraries(libqzmq PRIVATE ${ZeroMQ_STATIC_LIBRARY})
    endif()
endif()

if(BUILD_STATIC)
    add_library(libqzmq-static STATIC ${QTZMQ_SOURCES})
    list(APPEND target_outputs "libqzmq-static")
endif()

# Set common properties for all targets
foreach(target ${target_outputs})
    set_target_properties(${target} PROPERTIES POSITION_INDEPENDENT_CODE ON)
    set_target_properties(${target}
        PROPERTIES
            AUTOMOC TRUE
            PUBLIC_HEADER "${QZMQ_PUBLIC_HEADERS}"
            OUTPUT_NAME "qzmq"
            PREFIX "lib"
    )

    target_include_directories(${target}
        PUBLIC
            ${Qt5Core_INCLUDE_DIRS}
            ${ZeroMQ_INCLUDE_DIR}
    )
endforeach()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

if(BUILD_SHARED OR BUILD_STATIC)
    # Install compiled binaries and header files in the installation directory.
    # Also export targets which are used with export() in a later step.
    # Moreover, specify the directories to be added to the list of include interface
    install(TARGETS ${target_outputs}
        EXPORT ${PROJECT_NAME}-targets
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}
        INCLUDES DESTINATION include/${PROJECT_NAME}
    )

    # Setting up installation folder for *Config.cmake files
    if(WIN32)
        set(QZMQ_CMAKECONFIG_INSTALL_DIR "CMake" 
            CACHE 
            STRING "install path for qzmqConfig.cmake"
        )
    else()
        # GNUInstallDirs "DATADIR" wrong here; CMake search path wants "share".
        set(QZMQ_CMAKECONFIG_INSTALL_DIR "share/cmake/${PROJECT_NAME}" 
            CACHE 
            STRING "install path for qzmqConfig.cmake"
        )
    endif()

    # Create a *Config.cmake file using the given template in the current binary directory
    # This file will be installed in a later step.
    configure_package_config_file(${CMAKE_SOURCE_DIR}/${PROJECT_NAME}Config.cmake.in 
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
        INSTALL_DESTINATION ${QZMQ_CMAKECONFIG_INSTALL_DIR}
    )

    # Export the targets from the build tree to local *Targets.cmake file
    # So the other projects in the current build tree can use the exported targets.
    export(EXPORT ${PROJECT_NAME}-targets
        FILE ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Targets.cmake
    )

    # Install *Config.cmake and *Targets.cmake files which can be found by find_package()
    # in other proejcts outside the build tree.
    install(FILES 
            ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
            DESTINATION ${QTZMQ_CMAKECONFIG_INSTALL_DIR}
    )
    install(EXPORT 
        ${PROJECT_NAME}-targets
        FILE ${PROJECT_NAME}Targets.cmake
        DESTINATION ${QTZMQ_CMAKECONFIG_INSTALL_DIR}
    )

endif()