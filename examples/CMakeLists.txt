list(APPEND example_target_outputs "")
list(APPEND example_target_outputs "inproc_lat")

if(BUILD_STATIC)
    foreach(target ${example_target_outputs})
        add_executable(${target} "${target}.cpp")
        set_target_properties(${target}
            PROPERTIES
                AUTOMOC TRUE
                POSITION_INDEPENDENT_CODE ON
        )
        target_include_directories(${target} PRIVATE ${CMAKE_SOURCE_DIR}/src)
        target_include_directories(${target} PRIVATE ${ZeroMQ_INCLUDE_DIR})
        target_include_directories(${target} PRIVATE ${Qt5Core_INCLUDE_DIRS})
        target_link_libraries(${target} libqzmq-static)
        if(ZMQ_SHARED)
            target_link_libraries(${target} ${libzmq_shared})
        else()
            target_link_libraries(${target} ${libzmq_static})
        endif()

        target_link_libraries(${target} Qt5::Core)
        target_link_libraries(${target} ${CMAKE_THREAD_LIBS_INIT})
    endforeach()

endif()