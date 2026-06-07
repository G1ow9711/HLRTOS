function(mrt_add_test test_name test_source)
    add_executable(${test_name} ${test_source})
    target_link_libraries(${test_name} PRIVATE myrtos_kernel)
    target_include_directories(${test_name} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/support
    )
    add_test(NAME ${test_name} COMMAND ${test_name})
endfunction()

mrt_add_test(test_types_contract unit/test_types_contract.c)
mrt_add_test(test_config_defaults unit/test_config_defaults.c)
