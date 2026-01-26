{
  // gSystem->Load("${PUEO_UTIL_INSTALL_DIR}/lib64/libRootFftwWrapper.so");
  // gSystem->Load("${PUEO_UTIL_INSTALL_DIR}/lib64/libAntarcticaRoot.so"); 
  gSystem->Load("${PUEO_UTIL_INSTALL_DIR}/lib64/libpueoEvent.so"); 

  gStyle->SetNumberContours(255);
  gStyle->SetPalette(kInvertedDarkBodyRadiator);
}
