function (CollectSources SourcesVar HeadersVar RelativePath FromPath)
    if (NOT FromPath OR ${FromPath} STREQUAL "")
        set (FromPath ${CMAKE_CURRENT_SOURCE_DIR})
    endif ()

    file (GLOB_RECURSE Sources RELATIVE ${RelativePath} ${FromPath}/*.c ${FromPath}/*.cpp ${FromPath}/*.cxx)
    file (GLOB_RECURSE Headers RELATIVE ${RelativePath} ${FromPath}/*.h ${FromPath}/*.hpp ${FromPath}/*.hxx ${FromPath}/*.inc ${FromPath}/*.inl)

    set (${SourcesVar} ${Sources} PARENT_SCOPE)
    set (${HeadersVar} ${Headers} PARENT_SCOPE)
endfunction ()
