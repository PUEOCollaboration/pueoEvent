{
  gSystem->Load("${PUEO_UTIL_INSTALL_DIR}/lib/libRootFftwWrapper.so");
  gSystem->Load("${PUEO_UTIL_INSTALL_DIR}/lib/libAntarcticaRoot.so"); 
  gSystem->Load("${PUEO_UTIL_INSTALL_DIR}/lib/libpueoEvent.so"); 

  gStyle->SetNumberContours(255);
  gStyle->SetPalette(kInvertedDarkBodyRadiator);
}
