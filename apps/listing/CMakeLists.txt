include(warnings)

if (NOT MGPS_BUILD_TESTS_ONLY)
    add_executable(listing main.cc listing.cc listing.hh debug.cc debug.hh)
    target_link_libraries(listing PRIVATE mgps)
    set_target_properties(listing PROPERTIES FOLDER ${FOLDER_APPS})

    install(TARGETS listing COMPONENT tools)
endif()