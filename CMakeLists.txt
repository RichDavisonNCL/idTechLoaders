set(PROJECT_NAME idTechLoaders)

################################################################################
# Source groups
################################################################################
set(Source_Files
    "MD5MeshLoader.cpp"
    "Quake3MapLoader.cpp"
)
source_group("Source Files" FILES ${Source_Files})

set(Header_Files
    "MD5MeshLoader.h"
    "Quake3MapLoader.h"
)
source_group("Header Files" FILES ${Header_Files})


set(ALL_FILES
    ${Header_Files}
	${Source_Files}
)

################################################################################
# Target
################################################################################
add_library(${PROJECT_NAME} STATIC ${ALL_FILES})

set_target_properties(${PROJECT_NAME} PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION_RELEASE "TRUE"
)
target_precompile_headers(${PROJECT_NAME} PRIVATE
    <memory>
    <unordered_set>
    <vector>
    <string>
    <fstream>
    <sstream>
    <iostream>
    <string>
    <iosfwd>
    <set>
    <map> 
    <chrono>
    <thread>
    <filesystem>
    <functional>
	<algorithm>
	<assert.h>  
)

################################################################################
# Compile and link options
################################################################################
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        ${DEFAULT_CXX_DEBUG_INFORMATION_FORMAT};
        ${DEFAULT_CXX_EXCEPTION_HANDLING};
    )
endif()