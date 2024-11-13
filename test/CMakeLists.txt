
function(create_test name)
  add_executable(${name} src/${name}.cpp)
  target_link_libraries(${name} PRIVATE common)
  add_test(NAME ${name} COMMAND ${name})
endfunction(create_test)

create_test(seq_simple_read_test)
create_test(seq_writeback_test)

