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
  include = "cstdint" \
  target = " \
event_second,\
readout_time,\
event_number,\
L2_mask,\
trig_type,\
event_time,\
last_pps,\
llast_pps,\
deadtime_counter,\
deadtime_counter_last_pps,\
deadtime_counter_llast_pps\
  " \
  version =  "[2]" \
  source = "\
Int_t run;\
UInt_t triggerTime;\
UInt_t readoutTime;\
UInt_t readoutTimeNs;\
ULong_t eventNumber;\
UInt_t L2Mask;\
UInt_t trigType;\
UInt_t trigTime;\
UInt_t lastPPS;\
UInt_t lastLastPPS;\
UShort_t deadTime;\
UShort_t deadTimeLastPPS;\
UShort_t deadTimeLastLastPPS;\
  "\
  code = "{\
newObj->run = (uint32_t) onfile.run;\
event_second = (int32_t) onfile.triggerTime;\
readout_time = TTimeStamp((time_t) onfile.readoutTime, onfile.readoutTimeNs);\
event_number = static_cast<uint32_t>(onfile.eventNumber); \
L2_mask = static_cast<uint32_t>(onfile.L2Mask);\
trig_type = static_cast<uint32_t>(onfile.trigType);\
event_time = static_cast<uint32_t>(onfile.trigTime);\
last_pps = static_cast<uint32_t>(onfile.lastPPS);\
llast_pps = static_cast<uint32_t>(onfile.lastLastPPS);\
deadtime_counter = static_cast<uint32_t>(onfile.deadTime);\
deadtime_counter_last_pps = static_cast<uint32_t>(onfile.deadTimeLastPPS);\
deadtime_counter_llast_pps = static_cast<uint32_t>(onfile.deadTimeLastLastPPS);\
  }"

#else
#error "for compilation"
#endif
