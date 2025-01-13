cmake_minimum_required(VERSION 3.13)

project(rhi VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Vulkan REQUIRED)

OPTION(RHI_ENABLE_THREAD_RECORDING OFF)
OPTION(USE_D2D_WSI "Build the project using Direct to Display swapchain" OFF)
OPTION(USE_DIRECTFB_WSI "Build the project using DirectFB swapchain" OFF)
OPTION(USE_WAYLAND_WSI "Build the project using Wayland swapchain" OFF)
OPTION(USE_HEADLESS "Build the project using headless extension swapchain" OFF)

IF(UNIX AND NOT APPLE)
	set(LINUX TRUE)
ENDIF()

IF(WIN32)
	IF (NOT Vulkan_FOUND)
		find_library(Vulkan_LIBRARY NAMES vulkan-1 vulkan PATHS ${CMAKE_SOURCE_DIR}/libs/vulkan)
		IF (Vulkan_LIBRARY)
			set(Vulkan_FOUND ON)
			MESSAGE("Using bundled Vulkan library version")
		ENDIF()
	ENDIF()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_WIN32_KHR")
ELSEIF(LINUX)
	IF (NOT Vulkan_FOUND)
		find_library(Vulkan_LIBRARY NAMES vulkan HINTS "$ENV{VULKAN_SDK}/lib" "${CMAKE_SOURCE_DIR}/libs/vulkan" REQUIRED)
		IF (Vulkan_LIBRARY)
			set(Vulkan_FOUND ON)
			MESSAGE("Using bundled Vulkan library version")
		ENDIF()
	ENDIF()
	find_package(Threads REQUIRED)
	IF(USE_D2D_WSI)
		MESSAGE("Using direct to display extension...")
		add_definitions(-D_DIRECT2DISPLAY)
	ELSEIF(USE_DIRECTFB_WSI)
		find_package(DirectFB REQUIRED)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_DIRECTFB_EXT")
		include_directories(${DIRECTFB_INCLUDE_DIR})
	ELSEIF(USE_WAYLAND_WSI)
		find_program(PKG_CONFIG pkg-config)
		if (NOT PKG_CONFIG)
			message(FATAL_ERROR "pkg-config binary not found")
		endif ()
		find_package(Wayland REQUIRED)
		if (NOT WAYLAND_FOUND)
			message(FATAL_ERROR "Wayland development package not found")
		endif ()
		pkg_check_modules(WAYLAND_PROTOCOLS REQUIRED wayland-protocols)
		if (NOT WAYLAND_PROTOCOLS_FOUND)
			message(FATAL_ERROR "Wayland protocols package not found")
		endif ()
		find_program(WAYLAND_SCANNER wayland-scanner)
		if (NOT WAYLAND_SCANNER)
			message(FATAL_ERROR "wayland-scanner binary not found")
		endif ()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_WAYLAND_KHR")
		include_directories(${WAYLAND_INCLUDE_DIR})
		pkg_get_variable(protocol_dir wayland-protocols pkgdatadir)
		execute_process(COMMAND ${WAYLAND_SCANNER} client-header ${protocol_dir}/stable/xdg-shell/xdg-shell.xml ${CMAKE_BINARY_DIR}/xdg-shell-client-protocol.h
				COMMAND ${WAYLAND_SCANNER} private-code ${protocol_dir}/stable/xdg-shell/xdg-shell.xml ${CMAKE_BINARY_DIR}/xdg-shell-protocol.c)
		include_directories(${CMAKE_BINARY_DIR})
	ELSEIF(USE_HEADLESS)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_HEADLESS_EXT")
	ELSE(USE_D2D_WSI)
		find_package(XCB REQUIRED)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_XCB_KHR")
	ENDIF(USE_D2D_WSI)
	# Todo : android?
ENDIF(WIN32)

IF (NOT Vulkan_FOUND)
	message(FATAL_ERROR "Could not find Vulkan library!")
ELSE()
	message(STATUS ${Vulkan_LIBRARY})
ENDIF()

add_subdirectory(third_party)

set(interface_rhi 
	"include/rhi/rhi.h"
	"include/rhi/rhi_struct.h")
set(common_rhi
	
	"src/Utils.h"
	"include/rhi/common/RefCounted.h")

set(src_common
	"src/NoCopyable.h"
	"src/Ref.hpp"
	"src/RHI.cpp"
	"src/UploadAllocator.h"
	"src/UploadAllocator.cpp"
	"src/Commands.h"
	"src/CommandEncoder.h"
	"src/CommandEncoder.cpp"
	"src/CommandAllocator.h"
	"src/CommandAllocator.cpp")
set(src_vk
	"src/vulkan/VMA.cpp"
	"src/vulkan/DeviceVk.h"
	"src/vulkan/DeviceVk.cpp"
	"src/vulkan/ResourceVk.h"
	"src/vulkan/ResourceVk.cpp"
	"src/vulkan/SwapChainVk.h"
	"src/vulkan/SwapChainVk.cpp"
	"src/vulkan/ErrorsVk.h"
	"src/vulkan/CommandListVk.h"
	"src/vulkan/CommandListVk.cpp"
	"src/vulkan/PipelineVk.h"
	"src/vulkan/PipelineVk.cpp"  
	"src/vulkan/CommandQueueVk.h"
	"src/vulkan/CommandQueueVk.cpp"
	"src/vulkan/BufferVk.h"
	"src/vulkan/BufferVk.cpp" )

add_library(rhi "")

target_sources(rhi	PRIVATE
				${interface_rhi}
				${common_rhi}
				${src_common}
				${src_vk} )

target_include_directories(rhi PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> )

set_target_properties(rhi PROPERTIES FOLDER "RHI")

# being a cross-platform target, we enforce standards conformance on MSVC
target_compile_options(rhi PUBLIC "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/permissive->")
target_compile_options(rhi PUBLIC "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/WX->")

IF(WIN32)
	# Nothing here (yet)
ELSE(WIN32)
	target_link_libraries(rhi ${XCB_LIBRARIES} ${Vulkan_LIBRARY} ${Vulkan_LIBRARY} ${DIRECTFB_LIBRARIES} ${WAYLAND_CLIENT_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
ENDIF(WIN32)

target_link_libraries(rhi Vulkan::Vulkan vma)