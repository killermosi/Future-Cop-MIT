#include "Accessor.h"

#include "Mission/ANMResource.h"
#include "Mission/BMPResource.h"
#include "Mission/DCSResource.h"
#include "Mission/FUNResource.h"
#include "Mission/FontResource.h"
#include "Mission/MSICResource.h"
#include "Mission/NetResource.h"
#include "Mission/ObjResource.h"
#include "Mission/PTCResource.h"
#include "Mission/PYRResource.h"
#include "Mission/RPNSResource.h"
#include "Mission/SNDSResource.h"
#include "Mission/TilResource.h"
#include "Mission/TOSResource.h"
#include "Mission/WAVResource.h"

#define SEARCH( CLASS_NAME, GET_METHOD_NAME, ALL_METHOD_NAME, GET_CONST_METHOD_NAME, ALL_CONST_METHOD_NAME ) \
\
Mission::CLASS_NAME* Accessor::GET_METHOD_NAME( uint32_t resource_id ) {\
    Mission::Resource *resource_r = nullptr;\
    SearchValue search_value = { Mission::CLASS_NAME::IDENTIFIER_TAG, resource_id };\
\
    auto result = search.find(search_value);\
\
    if( result != search.end() )\
        resource_r = (*result).second;\
\
    return dynamic_cast<Mission::CLASS_NAME*>( resource_r );\
}\
const Mission::CLASS_NAME* Accessor::GET_CONST_METHOD_NAME( uint32_t resource_id ) const {\
    Mission::Resource *resource_r = nullptr;\
    SearchValue search_value = { Mission::CLASS_NAME::IDENTIFIER_TAG, resource_id };\
\
    auto result = search.find(search_value);\
\
    if( result != search.end() )\
        resource_r = (*result).second;\
\
    return dynamic_cast<Mission::CLASS_NAME*>( resource_r );\
}\
std::vector<Mission::CLASS_NAME*> Accessor::ALL_METHOD_NAME() {\
    std::vector<Mission::CLASS_NAME*> array;\
\
    for( auto r_it = search.begin(); r_it != search.end(); r_it++ ) {\
        if( (*r_it).first.type == Mission::CLASS_NAME::IDENTIFIER_TAG ) {\
            assert( dynamic_cast<Mission::CLASS_NAME*>( (*r_it).second ) );\
            array.emplace_back( dynamic_cast<Mission::CLASS_NAME*>( (*r_it).second ) );\
        }\
    }\
\
    return array;\
}\
std::vector<const Mission::CLASS_NAME*> Accessor::ALL_CONST_METHOD_NAME() const {\
    std::vector<const Mission::CLASS_NAME*> array;\
\
    for( auto r_it = search.begin(); r_it != search.end(); r_it++ ) {\
        if( (*r_it).first.type == Mission::CLASS_NAME::IDENTIFIER_TAG ) {\
            assert( dynamic_cast<const Mission::CLASS_NAME*>( (*r_it).second ) );\
            array.emplace_back( dynamic_cast<const Mission::CLASS_NAME*>( (*r_it).second ) );\
        }\
    }\
\
    return array;\
}

namespace Data {

bool Accessor::SearchValue::operator< ( const Accessor::SearchValue & operand ) const {
    if( type != operand.type )
        return (type < operand.type);
    else
        return (resource_id < operand.resource_id);
}

Accessor::Accessor() {
}

Accessor::~Accessor() {}

void Accessor::load( Mission::IFF &resource_r ) {
    SearchValue search_value;
    std::vector<Mission::Resource*> array = resource_r.getAllResources();

    for( auto r_it = array.begin(); r_it != array.end(); r_it++ ) {
        search_value.type = (*r_it)->getResourceTagID();
        search_value.resource_id = (*r_it)->getResourceID();

        search[ search_value ] = (*r_it);
    }
}

void Accessor::clear() {
    search.clear();
}

SEARCH(ANMResource,   getANM,  getAllANM, getConstANM, getAllConstANM)
SEARCH(BMPResource,   getBMP,  getAllBMP, getConstBMP, getAllConstBMP)
SEARCH(DCSResource,   getDCS,  getAllDCS, getConstDCS, getAllConstDCS)
SEARCH(FUNResource,   getFUN,  getAllFUN, getConstFUN, getAllConstFUN)
SEARCH(FontResource,  getFNT,  getAllFNT, getConstFNT, getAllConstFNT)
SEARCH(MSICResource, getMISC, getAllMISC, getConstMISC, getAllConstMISC)
SEARCH(NetResource,   getNET,  getAllNET, getConstNET, getAllConstNET)
SEARCH(ObjResource,   getOBJ,  getAllOBJ, getConstOBJ, getAllConstOBJ)
SEARCH(PTCResource,   getPTC,  getAllPTC, getConstPTC, getAllConstPTC)
SEARCH(PYRResource,   getPYR,  getAllPYR, getConstPYR, getAllConstPYR)
SEARCH(RPNSResource, getRPNS, getAllRPNS, getConstRPNS, getAllConstRPNS)
SEARCH(SNDSResource, getSNDS, getAllSNDS, getConstSNDS, getAllConstSNDS)
SEARCH(TilResource,   getTIL,  getAllTIL, getConstTIL, getAllConstTIL)
SEARCH(TOSResource,   getTOS,  getAllTOS, getConstTOS, getAllConstTOS)
SEARCH(WAVResource,   getWAV,  getAllWAV, getConstWAV, getAllConstWAV)

}

#undef SEARCH
