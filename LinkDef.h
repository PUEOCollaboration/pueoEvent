#ifdef __ROOTCLING__
#define ROOT_PARSER
#elif __CINT__
#define ROOT_PARSER
#endif

#ifdef ROOT_PARSER

#pragma link off all globals;
#pragma link off all classes;



#pragma link C++ namespace     pueo;
#pragma link C++ namespace     pueo::k;

#pragma link C++ namespace     pueo::ring;
#pragma link C++ enum          pueo::ring::ring_t;


#pragma link C++ namespace     pueo::pol;
#pragma link C++ enum          pueo::pol::pol_t;

#pragma link C++ namespace     pueo::Locations;
#pragma link C++ namespace pueo::version;


#pragma link C++ class pueo::GeomTool-;
#pragma link C++ class pueo::RawEvent+;
#pragma link C++ class pueo::Dataset+;
#pragma link C++ class pueo::TruthEvent+;
#pragma link C++ class pueo::UsefulEvent+;
#pragma link C++ class pueo::RawHeader+;
#pragma link C++ namespace pueo::nav;
#pragma link C++ class pueo::nav::Position+;
#pragma link C++ class pueo::nav::Attitude+;
#pragma link C++ class pueo::nav::Sat+;
#pragma link C++ class pueo::nav::Sats+;
#pragma link C++ class pueo::nav::SunSensor+;
#pragma link C++ class pueo::nav::SunSensors+;


#pragma read \
  targetClass = "pueo::RawEvent"\
  sourceClass = "pueo::RawEvent"\
  target = "data" \
  version =  "[1]" \
  source = " std::vector<int16_t> data[208]; "\
  code = "{ int iin = 0; for (int i = 0; i < 216; i++) { if (i> 160 && i < 168) { continue; } std::copy(onfile.data[iin].begin(), onfile.data[iin].end(), data[i].begin()); iin++; }}"


#pragma read \
  targetClass = "pueo::UsefulEvent"\
  sourceClass = "pueo::UsefulEvent"\
  target = "volts" \
  version =  "[1]" \
  source = " std::vector<double> volts[208]; "\
  code = "{ int iin = 0; for (int i = 0; i < 208; i++) { std::copy(onfile.volts[iin].begin(), onfile.volts[iin].end(), volts[i].begin()); iin++; }}"


#else
#error "for compilation"
#endif
