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

#pragma link C++ class pueo::hsk::Sensor+;
#pragma link C++ class pueo::daqhsk::DaqHsk+;
#pragma link C++ class pueo::daqhsk::Surf+;
#pragma link C++ class pueo::daqhsk::Beam+;
#pragma link C++ class pueo::Timemark+;

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

#pragma read \
  targetClass = "pueo::RawHeader"\
  sourceClass = "pueo::RawHeader"\
  target = "run" \
  version =  "[2]" \
  source = "Int_t run;"

#pragma read \
  targetClass = "pueo::RawHeader"\
  sourceClass = "pueo::RawHeader"\
  target = "event_second" \
  version =  "[2]" \
  source = "UInt_t triggerTime;"

#pragma read \
  targetClass = "pueo::RawHeader"\
  sourceClass = "pueo::RawHeader"\
  target = "event_second" \
  version =  "[2]" \
  source = "UInt_t triggerTime;"

#pragma read \
  targetClass = "pueo::RawHeader"\
  sourceClass = "pueo::RawHeader"\
  target = "readout_time" \
  version =  "[2]" \
  source = "UInt_t readoutTime; UInt_t readoutTimeNs;"\
  code="{readout_time = TTimeStamp((time_t) onfile.readoutTime, onfile.readoutTimeNs);}"

#pragma read \
  targetClass = "pueo::RawHeader"\
  sourceClass = "pueo::RawHeader"\
  target = "event_number" \
  version =  "[2]" \
  source = "ULong_t eventNumber;"

#pragma read \
  targetClass = "pueo::RawHeader"\
  sourceClass = "pueo::RawHeader"\
  target = "L2_mask" \
  version =  "[2]" \
  source = "UInt_t L2Mask;"

#pragma read \
  targetClass = "pueo::RawHeader"\
  sourceClass = "pueo::RawHeader"\
  target = "trig_type" \
  version =  "[2]" \
  source = "UInt_t trigType;"

#pragma read \
  targetClass = "pueo::RawHeader"\
  sourceClass = "pueo::RawHeader"\
  target = "event_time" \
  version =  "[2]" \
  source = "UInt_t trigTime;"

#pragma read \
  targetClass = "pueo::RawHeader"\
  sourceClass = "pueo::RawHeader"\
  target = "last_pps" \
  version =  "[2]" \
  source = "UInt_t lastPPS;"

#pragma read \
  targetClass = "pueo::RawHeader"\
  sourceClass = "pueo::RawHeader"\
  target = "llast_pps" \
  version =  "[2]" \
  source = "UInt_t lastLastPPS;"

#pragma read \
  targetClass = "pueo::RawHeader"\
  sourceClass = "pueo::RawHeader"\
  target = "deadtime_counter" \
  version =  "[2]" \
  source = "UShort_T deadTime;"

#pragma read \
  targetClass = "pueo::RawHeader"\
  sourceClass = "pueo::RawHeader"\
  target = "deadtime_counter_last_pps" \
  version =  "[2]" \
  source = "UShort_T deadTimeLastPPS;"

#pragma read \
  targetClass = "pueo::RawHeader"\
  sourceClass = "pueo::RawHeader"\
  target = "deadtime_counter_llast_pps" \
  version =  "[2]" \
  source = "UShort_T deadTimeLastLastPPS;"

#else
#error "for compilation"
#endif
