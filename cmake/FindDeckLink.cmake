include(FindPackageHandleStandardArgs)

macro(set_component_found _component )
  if (${_component}_LIBRARIES)
    # message(STATUS "  - ${_component} found: ${${_component}_LIBRARIES}.")
    set(${_component}_FOUND TRUE)
  else ()
    message(STATUS "  - ${_component} not found.")
  endif ()
endmacro()

macro(find_component _component _library)

  find_library(${_component}_LIBRARIES NAMES ${_library})

  set_component_found(${_component})

  mark_as_advanced(
    ${_component}_LIBRARIES
  )

  if (${_component}_FOUND)
      list(APPEND DECKLINK_LIBRARIES ${${_component}_LIBRARIES})
  else ()
      # message(STATUS "Required component ${_component} missing.")
  endif ()

endmacro()

if (NOT DECKLINK_LIBRARIES)
  find_component(DeckLinkAPI DeckLinkAPI)
  find_component(DeckLinkPreviewAPI DeckLinkPreviewAPI)

  set(DECKLINK_LIBRARIES ${DECKLINK_LIBRARIES} CACHE STRING "The DeckLink libraries." FORCE)
endif ()

find_package_handle_standard_args(DeckLink DEFAULT_MSG DECKLINK_LIBRARIES)