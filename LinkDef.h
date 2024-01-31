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




#else
#error "for compilation"
#endif
