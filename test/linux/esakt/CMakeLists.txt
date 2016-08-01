
set(SOURCES esakt.c)
add_executable(esakt ${SOURCES})
target_link_libraries(esakt soem)

if(WIN32)
   target_link_libraries(esakt wpcap.lib Packet.lib Ws2_32.lib Winmm.lib)
elseif(UNIX)
   target_link_libraries(esakt pthread rt)
endif()

if(NOT DEFINED HOST_INSTALL)
   install(TARGETS esakt DESTINATION bin)
endif()
