#
# By hand version of the Config.cmake file.
#
include(CMakeFindDependencyMacro)

find_dependency(evio CONFIG REQUIRED)
find_dependency(et CONFIG REQUIRED)
find_dependency(ROOT REQUIRED COMPONENTS Core)

include("${CMAKE_CURRENT_LIST_DIR}/EvioToolTargets.cmake")