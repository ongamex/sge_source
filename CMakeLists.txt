# SGEEngine root CMake file.

cmake_minimum_required(VERSION 3.14)
project(sge)

# Needed for Liunx:
if(NOT WIN32)
	# This is equivalent to specifying -fPIC to g++. It tells the compiler to use 
	# relative location in the machine code rather than absolute (for example in jump instructions).
	# This is needed for *.so linking.
	set(CMAKE_POSITION_INDEPENDENT_CODE ON) 

	# By default on Linux, the OS does not look for linked *.so in the 
	# current directory or the directory of the object needing that symbol.
	# By adding -rpath=${'ORIGIN'} we will tell the OS to look in these directories as well.
	# We need this to be able to link locally with sge_core, sge_engine and maybe other statically linked shared libraries.
	set(CMAKE_INSTALL_RPATH "$ORIGIN")
endif()

# CAUTION: In order to "Support" Window 7...
# https://msdn.microsoft.com/en-us/library/6sehtctf.aspx
if(WIN32)
	add_definitions(-DWINVER=0x0601)
	add_definitions(-D_WIN32_WINNT=0x0601)
endif()

# Enable parallel compilation for MSVC by defult.
add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/MP>)

# Enable C++17. Add other compiler specific options.
if(UNIX)
	# [TODO] remove -w, find out what -g means (i think it instructs NOT to strip debug symbols)
	set(CMAKE_CXX_STANDARD 17)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w -g -fpermissive") #-std=c++14
endif()

if(MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17")
endif()

if(EMSCRIPTEN)
	# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Os") This flag should decrease the build size but it does not.
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s DISABLE_DEPRECATED_FIND_EVENT_TARGET_BEHAVIOR=1")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s USE_SDL=2")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s USE_WEBGL2=1")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s FULL_ES3=1") # TODO: Does it work without this one?
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s ALLOW_MEMORY_GROWTH=1") # By default Emscripten has a fixed pool size for dynamic allocations.
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Os")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g0") # Emscripten debug symbols take a lot of space for disable it by default.
	#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g3")
endif()

# Create the NoOpt configuration, which is the same as Release, but without optimizations.
# it is useful while developing as it is faster then Debug.
set(CMAKE_CONFIGURATION_TYPES Release RelWithDebInfo Debug NoOpt)

SET(CMAKE_C_FLAGS_NOOPT ${CMAKE_C_FLAGS_RELWITHDEBINFO})
SET(CMAKE_CXX_FLAGS_NOOPT ${CMAKE_CXX_FLAGS_RELWITHDEBINFO})
SET(CMAKE_EXE_LINKER_FLAGS_NOOPT ${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO})
SET(CMAKE_SHARED_LINKER_FLAGS_NOOPT ${CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO})

STRING(REPLACE "-O2" "-O0" CMAKE_C_FLAGS_NOOPT ${CMAKE_C_FLAGS_NOOPT})
STRING(REPLACE "-O2" "-O0" CMAKE_CXX_FLAGS_NOOPT ${CMAKE_CXX_FLAGS_NOOPT})

STRING(REPLACE "/O2" "/Od" CMAKE_C_FLAGS_NOOPT ${CMAKE_C_FLAGS_NOOPT})
STRING(REPLACE "/O2" "/Od" CMAKE_CXX_FLAGS_NOOPT ${CMAKE_CXX_FLAGS_NOOPT})

add_compile_options($<$<CONFIG:NoOpt>:/MD>)

SET(CMAKE_C_FLAGS_NOOPT ${CMAKE_C_FLAGS_NOOPT})
SET(CMAKE_CXX_FLAGS_NOOPT ${CMAKE_CXX_FLAGS_NOOPT})
SET(CMAKE_EXE_LINKER_FLAGS_NOOPT ${CMAKE_EXE_LINKER_FLAGS_NOOPT})

SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DSGE_USE_DEBUG")
SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DSGE_USE_DEBUG")

SET(CMAKE_C_FLAGS_NOOPT "${CMAKE_C_FLAGS_NOOPT} -DSGE_USE_DEBUG")
SET(CMAKE_CXX_FLAGS_NOOPT "${CMAKE_CXX_FLAGS_NOOPT} -DSGE_USE_DEBUG")

# Add some helper functions.
include(./scripts/cmake/macros.cmake)

# Marks the "target_name" (if exits) to be filtered as an external. 
macro(sge_mark_external_target target_name)
if(TARGET ${target_name})
	set_target_properties (${target_name} PROPERTIES FOLDER lib_ext)
endif()
endmacro()

# Marks the specified target (if exists) to be filtered in IDE as a library.
macro(sge_mark_internal_lib target_name)
if(TARGET ${target_name})
	set_target_properties (${target_name} PROPERTIES FOLDER lib)
endif()
endmacro()

# Marks the specified target (if exists) to be filtered in IDE as tools that are executable.
macro(sge_mark_internal_tools target_name)
if(TARGET ${target_name})
	set_target_properties (${target_name} PROPERTIES FOLDER tools)
endif()
endmacro()

#---------------------------------------------------------------
# Properties and config settings that the user should specify
# or hopefully be able to use the default.
#---------------------------------------------------------------
if(WIN32)
	set(SGE_REND_API "Direct3D11" CACHE STRING "Rendering API")
else()
	set(SGE_REND_API "OpenGL" CACHE STRING "Rendering API")
endif()
set_property(CACHE SGE_REND_API PROPERTY STRINGS Direct3D11 OpenGL)

set(SGE_FBX_SDK_DIR "" CACHE PATH "The directory that contains the FBX SDK include directory.")

if(SGE_FBX_SDK_DIR STREQUAL "")
	message(WARNING "FBX SDK Is not specified! Importing FBX Files would not be possible. However the build should succeed!")
endif()

# Print some messages that show what configuration is being used.
MESSAGE("SGE_REND_API = ${SGE_REND_API}")
MESSAGE("SGE_FBX_SDK_DIR = ${SGE_FBX_SDK_DIR}")

# Promotes and disables some warrning to make the development process a bit better.
macro(sgePromoteWarningsOnTarget target)
	if(MSVC)
		set(sge_compiler_opt_warnings "/WX" "/W4"
			# Disable some warnings
			"/wd4251" # 'type' : class 'type1' needs to have dll-interface to be used by clients of class 'type2'
			"/wd4275"
			"/wd4127" # The controlling expression of an if statement or while loop evaluates to a constant. 
			"/wd4201" # nonstandard extension used : nameless struct/union
			"/wd4245" # conversion from 'type1' to 'type2', signed/unsigned mismatch
			"/wd4324" # structure was padded due to __declspec(align())
					 
			"/wd4061" # enumerator 'identifier' in switch of enum 'enumeration' is not explicitly handled by a case label
			"/wd4191" # unsafe conversion from 'type of expression' to 'type required'
			"/wd4264" # no override available for virtual member function from base 'class'; function is hidden
			"/wd4266" # no override available for virtual member function from base 'type'; function is hidden
			"/wd4355" # this used in base member initializer list
			"/wd4365" # action conversion from 'type_1' to 'type_2', signed/unsigned mismatch
			"/wd4371" # class layout of class may have changed from a previous version of the compiler due to better packing of member 'member'
			"/wd4388"
			"/wd4514" # function unreferenced inline function has been removed
			"/wd4571" # catch(...) semantics changed since Visual C++ 7.1; structured exceptions (SEH) are no longer caught
			"/wd4582"
			"/wd4583"
			"/wd4623" # 'derived class' : default constructor was implicitly defined as deleted because a base class default constructor is inaccessible or deleted
			"/wd4625" # 'derived class' : copy constructor was implicitly defined as deleted because a base class copy constructor is inaccessible or deleted
			"/wd4626" # 'derived class' : assignment operator was implicitly defined as deleted because a base class assignment operator is inaccessible or deleted
			"/wd4710" # 'function' : function not inlined
			"/wd4711" # function 'function' selected for inline expansion
			"/wd4820" # 'bytes' bytes padding added after construct 'member_name'
			"/wd4866" # 'file(line_number)' compiler may not enforce left-to-right evaluation order for call to operator_name
			"/wd4868" # 'file(line_number)' compiler may not enforce left-to-right evaluation order in braced initializer list
			"/wd4458" # declaration of 'X' hides class member
			"/wd5026"
			"/wd5027"
		)
		
		target_compile_options(${target} PRIVATE ${sge_compiler_opt_warnings})
	endif()
endmacro()

# Add the 3rd party libraries to the build.
add_subdirectory(./libs_ext/mcpp)
add_subdirectory(./libs_ext/HLSLParser)
add_subdirectory(./libs_ext/bullet)
add_subdirectory(./libs_ext/recastnavigation)
add_subdirectory(./libs_ext/imgui)
add_subdirectory(./libs_ext/doctest)

# Use SDL2 as a dll.
# The code shouldn't depend on SDL2 being a DLL
# it should work fine if it is a static library.
if(NOT EMSCRIPTEN)
	set(SDL_STATIC OFF CACHE BOOL "  " FORCE)
	set(SDL_SHARED ON CACHE BOOL "  " FORCE)
	add_subdirectory(./libs_ext/SDL2)
endif()

# Add SGEEngine and all of its targets.
add_subdirectory(./libs/sge_utils)

# Set Definitions Rendering APIs.
if(SGE_REND_API STREQUAL "Direct3D11")
	add_definitions(-DSGE_RENDERER_D3D11)
endif()
if(SGE_REND_API STREQUAL "OpenGL")
	add_definitions(-DSGE_RENDERER_GL)

	# add GLEW
	if(NOT EMSCRIPTEN)
		add_definitions(-DGLEW_STATIC)
		add_subdirectory(./libs_ext/glew)
	endif()
endif()

add_subdirectory(./libs/sge_renderer)
add_subdirectory(./libs/sge_audio)
add_subdirectory(./libs/sge_core)
add_subdirectory(./libs/sge_engine)
add_subdirectory(./sge_player)
if(NOT EMSCRIPTEN)
	add_subdirectory(./sge_editor)
	add_subdirectory(./libs/mdlconvlib)
endif()

# No matter what the other project sets (particularly 3rd parties)
# we do not want to used configuration postfix in the output files.
set(CMAKE_DEBUG_POSTFIX "" CACHE STRING "" FORCE)
set(CMAKE_NOOPT_POSTFIX "" CACHE STRING "" FORCE)
set(CMAKE_RELEASE_POSTFIX "" CACHE STRING "" FORCE)
set(CMAKE_RELWITHDEBINFO_POSTFIX "" CACHE STRING "" FORCE)

# Organize the generated project in gropus to have them nicely displaced in the IDE (if supported).
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# External projects.
sge_mark_external_target(glew)
sge_mark_external_target(lua)
sge_mark_external_target(imgui)
sge_mark_external_target(BulletCollision)
sge_mark_external_target(BulletDynamics)
sge_mark_external_target(BulletSoftBody)
sge_mark_external_target(LinearMath)
sge_mark_external_target(Bullet3Common)
sge_mark_external_target(BulletInverseDynamics)
sge_mark_external_target(Recast)
sge_mark_external_target(Detour)
sge_mark_external_target(SDL2)
sge_mark_external_target(SDL2main)
sge_mark_external_target(SDL2-static)
#sge_mark_external_target(doctest) # doctest doesn't create any targets, so it is not needed.
sge_mark_external_target(mcpp)
sge_mark_external_target(HLSLParser)

# Internal projects.
sge_mark_internal_lib(mdlconvlib)
sge_mark_internal_lib(sge_utils)
sge_mark_internal_lib(sge_utils_Tests)
sge_mark_internal_lib(sge_renderer)
sge_mark_internal_lib(sge_audio)
sge_mark_internal_lib(sge_core)
sge_mark_internal_lib(sge_engine)
sge_mark_internal_lib(mdlconvlib)

sge_mark_internal_tools(sge_player)
sge_mark_internal_tools(sge_editor)

#include(engine_install.cmake)
include(copy_engine_helper.cmake)

# Samples
add_subdirectory(samples/Demo)
add_subdirectory(samples/TemplateGame)
add_subdirectory(samples/CharacterGame)
add_subdirectory(samples/SpriteGame2D)
