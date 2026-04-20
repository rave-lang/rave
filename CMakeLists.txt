cmake_minimum_required(VERSION 4.0)



project(
	rave
	VERSION 0.1.0
	DESCRIPTION "Rave is a programming language for everyone."
	HOMEPAGE_URL "https://github.com/rave-lang/rave"
	LANGUAGES C CXX
)


set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

foreach(config IN ITEMS Debug Release RelWithDebInfo MinSizeRel)
	string(TOUPPER ${config} config_upper)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${config_upper} ${CMAKE_BINARY_DIR}/bin)
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${config_upper} ${CMAKE_BINARY_DIR}/bin)
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${config_upper} ${CMAKE_BINARY_DIR}/bin)
endforeach()


function(configure_rave)
	find_package(LLVM CONFIG REQUIRED)
	message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
	message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
	enable_testing()

	find_program(LLVM_CONFIG_EXECUTABLE llvm-config REQUIRED)

	# System Libs
	execute_process(
			COMMAND ${LLVM_CONFIG_EXECUTABLE} --system-libs
			OUTPUT_VARIABLE LLVM_SYSTEM_LIBS
			OUTPUT_STRIP_TRAILING_WHITESPACE
	)
	separate_arguments(LLVM_SYSTEM_LIBS)

	# Libfiles for AOT and JIT
	execute_process(
			COMMAND ${LLVM_CONFIG_EXECUTABLE} --libfiles core orcjit native analysis codegen passes target mc
			OUTPUT_VARIABLE LLVM_LIB_FILES
			OUTPUT_STRIP_TRAILING_WHITESPACE
	)
	separate_arguments(LLVM_LIB_FILES)

	message(STATUS "LLVM System Libraries: ${LLVM_SYSTEM_LIBS}")
	message(STATUS "LLVM Lib Files: ${LLVM_LIB_FILES}")
	message(STATUS "LLVM Include Dirs: ${LLVM_INCLUDE_DIRS}")


	file(GLOB_RECURSE RAVE_SOURCES CONFIGURE_DEPENDS language/*.cpp)
	add_executable(rave ${RAVE_SOURCES})
	target_include_directories(rave PRIVATE ${LLVM_INCLUDE_DIRS})
	target_link_libraries(rave PRIVATE ${LLVM_LIB_FILES} ${LLVM_SYSTEM_LIBS})
endfunction()

configure_rave()
