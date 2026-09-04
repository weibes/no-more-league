add_executable(no-more-league
    no-more-league.cpp
    no-more-league.h
)

target_include_directories(no-more-league PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if(WIN32)
    target_link_libraries(no-more-league PRIVATE
        psapi
        shell32
        advapi32
        user32
        ole32
    )
endif()
