#####################################################
# Project SGE ENGINE
#####################################################

cmake_minimum_required(VERSION 3.5)

# Create a target for the editor itself.
add_dir_rec_2(SGE_EDITOR_SOURCES "./src" 1)
add_executable(sge_editor WIN32 ${SGE_EDITOR_SOURCES}) 
target_link_libraries(sge_editor sge_engine)

if(WIN32)
	# Needed for minidump.
	target_link_libraries(sge_editor Dbghelp.lib)
endif()

sgePromoteWarningsOnTarget(sge_editor)

# Visual Studio Debugging		   
set_target_properties(sge_editor PROPERTIES 
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_INSTALL_PREFIX}/$<CONFIG>/binaries")
	
set_target_properties(sge_editor PROPERTIES 
    VS_DEBUGGER_COMMAND "${CMAKE_INSTALL_PREFIX}/$<CONFIG>/binaries/$<TARGET_FILE_NAME:sge_editor>")
