configure_file(
  "com.ubuntu.developer.doflah.haca_nhlscope-settings.ini"
  "${CMAKE_BINARY_DIR}/src/com.ubuntu.developer.doflah.haca_nhlscope-settings.ini"
)

# Install the scope ini file
install(
  FILES
    "com.ubuntu.developer.doflah.haca_nhlscope-settings.ini"
  DESTINATION ${SCOPE_INSTALL_DIR}
)

# Install the scope images
install(
  FILES
    "icon.png"
    "logo.png"
    "screenshot.png"
  DESTINATION
    "${SCOPE_INSTALL_DIR}"
)

