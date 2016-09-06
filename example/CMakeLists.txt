#=================== add all example ===================
file(GLOB_RECURSE AllExample "." "*.cpp")
foreach(example ${AllExample})
    get_filename_component(name ${example} NAME_WE) # get NAME Without Extension
    add_executable(${name} ${name}.cpp)
    #~ target_link_libraries(${name} lib_name)
endforeach(example)
