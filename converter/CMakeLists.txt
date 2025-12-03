  find_library(PUEORAWDATALIB pueorawdata $ENV{LIBPUEORAWDATA_DIR}/build)
  find_library(PUEOEVENTLIB pueoEvent $ENV{PUEO_UTIL_INSTALL_DIR}/lib)
  find_package(ROOT REQUIRED COMPONENTS TreePlayer Physics)

  add_executable(convertSingleWaveform convert_single_waveform.cc)
  add_executable(combineWaveformFiles combine_waveform_files.cc)
  target_link_libraries(convertSingleWaveform PUBLIC ${PUEORAWDATALIB} ${PUEOEVENTLIB}  
   ROOT::TreePlayer ROOT::Physics )
  target_include_directories(convertSingleWaveform PUBLIC $ENV{LIBPUEORAWDATA_DIR}/inc $ENV{PUEO_UTIL_INSTALL_DIR}/include)
  target_link_libraries(combineWaveformFiles PUBLIC ${PUEORAWDATALIB} ${PUEOEVENTLIB}  
   ROOT::TreePlayer ROOT::Physics )
  target_include_directories(combineWaveformFiles PUBLIC $ENV{LIBPUEORAWDATA_DIR}/inc $ENV{PUEO_UTIL_INSTALL_DIR}/include)