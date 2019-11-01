# Copyright 2011,2012 Free Software Foundation, Inc.
#
# This file is part of GNU Radio
#
# GNU Radio is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
#
# GNU Radio is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with GNU Radio; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.

########################################################################
# Setup library
########################################################################
include(GrPlatform) #define LIB_SUFFIX

include_directories(${Boost_INCLUDE_DIR})
link_directories(${Boost_LIBRARY_DIRS})
list(APPEND scatter_sources
    fir129_impl.cc
    fir129_block_ctrl_impl.cpp
    McFsource_impl.cc
    McFsource_block_ctrl_impl.cpp
    specsense2k_impl.cc
    specsense2k_block_ctrl_impl.cpp
    )


set(scatter_sources "${scatter_sources}" PARENT_SCOPE)
if(NOT scatter_sources)
	MESSAGE(STATUS "No C++ sources... skipping lib/")
	return()
endif(NOT scatter_sources)

add_library(gnuradio-scatter SHARED ${scatter_sources})
target_link_libraries(gnuradio-scatter ${Boost_LIBRARIES} ${GNURADIO_ALL_LIBRARIES} ${ETTUS_LIBRARIES} gnuradio-fft)
set_target_properties(gnuradio-scatter PROPERTIES DEFINE_SYMBOL "gnuradio_scatter_EXPORTS")

if(APPLE)
    set_target_properties(gnuradio-scatter PROPERTIES
        INSTALL_NAME_DIR "${CMAKE_INSTALL_PREFIX}/lib"
    )
endif(APPLE)

########################################################################
# Install built library files
########################################################################
install(TARGETS gnuradio-scatter
    LIBRARY DESTINATION lib${LIB_SUFFIX} # .so/.dylib file
    ARCHIVE DESTINATION lib${LIB_SUFFIX} # .lib file
    RUNTIME DESTINATION bin              # .dll file
)

########################################################################
# Build and register unit test
########################################################################
include(GrTest)

include_directories(${CPPUNIT_INCLUDE_DIRS})

list(APPEND test_scatter_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/test_scatter.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/qa_scatter.cc
)

add_executable(test-scatter ${test_scatter_sources})

target_link_libraries(
  test-scatter
  ${GNURADIO_RUNTIME_LIBRARIES}
  ${Boost_LIBRARIES}
  ${CPPUNIT_LIBRARIES}
  ${ETTUS_LIBRARIES}
  ${PC_ETTUS_LIBDIR}
  gnuradio-scatter
)



GR_ADD_TEST(test_scatter test-scatter)
