#include "Manager.h"

const std::string Data::Manager::global = "global";

const std::string Data::Manager::griffith_park        = "griffith-park";
const std::string Data::Manager::zuma_beach           = "zuma-beach";
const std::string Data::Manager::la_brea_tar_pits     = "la-brea-tar-pits";
const std::string Data::Manager::venice_beach         = "venice-beach";
const std::string Data::Manager::hells_gate_prison    = "hells-gate-prison";
const std::string Data::Manager::studio_city          = "studio-city";
const std::string Data::Manager::lax_spaceport        = "lax-spaceport";
const std::string Data::Manager::lax_spaceport_part_2 = "lax-spaceport-part-2";
const std::string Data::Manager::long_beach           = "long-beach";

const std::string *const Data::Manager::crime_war_iffs[] =
    { &Data::Manager::griffith_park, &Data::Manager::zuma_beach, &Data::Manager::la_brea_tar_pits, &Data::Manager::venice_beach,
      &Data::Manager::hells_gate_prison, &Data::Manager::studio_city, &Data::Manager::lax_spaceport, &Data::Manager::lax_spaceport_part_2, &Data::Manager::long_beach };
const size_t Data::Manager::AMOUNT_OF_CRIME_WAR_IDS = sizeof(Data::Manager::crime_war_iffs) / sizeof( Data::Manager::crime_war_iffs[0] );

const std::string Data::Manager::pa_urban_jungle   = "pa-urban-jungle";
const std::string Data::Manager::pa_venice_beach   = "pa-venice-beach";
const std::string Data::Manager::pa_hollywood_keys = "pa-hollywood-keys";
const std::string Data::Manager::pa_proving_ground = "pa-proving-grounds";
const std::string Data::Manager::pa_bug_hunt       = "pa-bug-hunt";
const std::string Data::Manager::pa_la_centina     = "pa-la-centina";

const std::string *const Data::Manager::precinct_assault_iffs[] =
    { &Data::Manager::pa_urban_jungle, &Data::Manager::pa_venice_beach, &Data::Manager::pa_hollywood_keys, &Data::Manager::pa_proving_ground,
      &Data::Manager::pa_bug_hunt, &Data::Manager::pa_la_centina };
const size_t Data::Manager::AMOUNT_OF_PRECINCT_ASSAULT_IDS = sizeof(Data::Manager::precinct_assault_iffs) / sizeof( Data::Manager::precinct_assault_iffs[0] );

const std::string *const Data::Manager::map_iffs[] =
    { &Data::Manager::griffith_park, &Data::Manager::zuma_beach, &Data::Manager::la_brea_tar_pits, &Data::Manager::venice_beach,
      &Data::Manager::hells_gate_prison, &Data::Manager::studio_city, &Data::Manager::lax_spaceport, &Data::Manager::lax_spaceport_part_2, &Data::Manager::long_beach,
      &Data::Manager::pa_urban_jungle, &Data::Manager::pa_venice_beach, &Data::Manager::pa_hollywood_keys, &Data::Manager::pa_proving_ground,
      &Data::Manager::pa_bug_hunt, &Data::Manager::pa_la_centina
    };
const size_t Data::Manager::AMOUNT_OF_IFF_IDS = Data::Manager::AMOUNT_OF_PRECINCT_ASSAULT_IDS + Data::Manager::AMOUNT_OF_CRIME_WAR_IDS;

Data::Manager::IFFEntry::IFFEntry() : paths(), iff_p(),
                                importance( NOT_NEEDED ) {
    for( unsigned i = 0; i < Platform::ALL; i++ )
        this->iff_p[ i ] = nullptr;
}

Data::Manager::IFFEntry::IFFEntry( const IFFEntry& obj ) : paths(),
                                   iff_p(), importance( obj.importance ) {
    for( unsigned i = 0; i < Platform::ALL; i++ )
        this->paths[ i ] = obj.paths[ i ];
    for( unsigned i = 0; i < Platform::ALL; i++ )
        this->iff_p[ i ] = obj.iff_p[ i ];
}

Data::Manager::IFFEntry::~IFFEntry() {
    // DO NOT DELETE ANYTHING. deleting iff_p is IFFEntryStorage's job.
}

void Data::Manager::IFFEntry::set( const IFFEntry& obj ) {
    importance = obj.importance;
    for( unsigned i = 0; i < Platform::ALL; i++ )
        this->paths[ i ] = obj.paths[ i ];
    for( unsigned i = 0; i < Platform::ALL; i++ )
        this->iff_p[ i ] = obj.iff_p[ i ];
}

void Data::Manager::IFFEntry::setPath( Platform platform, const std::string &path ) {
    if( platform == Platform::ALL )
    {
        for( auto &i : paths )
            i = path;
    }
    else
    if( platform < Platform::ALL )
        paths[ platform ] = path;
}

Data::Mission::IFF* Data::Manager::IFFEntry::getIFF( Platform platform ) {
    if( platform < Platform::ALL )
        return iff_p[ platform ];
    else
        return nullptr;
}

const Data::Mission::IFF* Data::Manager::IFFEntry::getIFF( Platform platform ) const {
    return const_cast<Data::Manager::IFFEntry*>( this )->getIFF( platform );
}
Data::Manager::IFFEntryStorage::IFFEntryStorage() {
}

Data::Manager::IFFEntryStorage::IFFEntryStorage( const IFFEntry& obj ) : IFFEntry( obj ) {
}

Data::Manager::IFFEntryStorage::~IFFEntryStorage() {
    for( auto &i : iff_p )
        if( i != nullptr )
            delete i;
}

void Data::Manager::IFFEntryStorage::set( const IFFEntry& obj ) {
    IFFEntry::set( obj );
}

bool Data::Manager::IFFEntryStorage::load( Platform platform ) {
    if( platform < Platform::ALL && this->iff_p[ platform ] == nullptr ) {
        this->iff_p[ platform ] = new Mission::IFF;
        
        auto result = this->iff_p[ platform ]->open( paths[ platform ] );
        
        if( result == 1 )
            return true;
        else {
            delete this->iff_p[ platform ];
            
            this->iff_p[ platform ] = nullptr;
            
            return false;
        }
    }
    else
        return true; // Already allocated.
}

bool Data::Manager::IFFEntryStorage::unload( Platform platform ) {
    if( platform < Platform::ALL && this->iff_p[ platform ] != nullptr ) {
        delete this->iff_p[ platform ];
        this->iff_p[ platform ] = nullptr;
        return true;
    }
    else
        return true; // Already unloaded.
}

Data::Manager::Manager() : /*thread_lock(),*/ entries(), currently_loaded_platforms( {false} ),
                           platforms_to_support( {false} ), current_importance_to_load( NEEDED ) {
}

Data::Manager::~Manager() {
}

bool Data::Manager::hasEntry( const std::string &name ) {
    bool is_found;

    // thread_lock.lock();

    is_found = entries.find( name ) != entries.end();

    // thread_lock.unlock();

    return is_found;
}

Data::Manager::IFFEntry Data::Manager::getIFFEntry( const std::string &name ) {
    IFFEntry entry;

    // thread_lock.lock();

    entry = entries[ name ];

    // thread_lock.unlock();

    return entry;
}

#include <iostream>

bool Data::Manager::setIFFEntry( const std::string &name, const IFFEntry &entry ) {
    // thread_lock.lock();

    if( entries.find( name ) == entries.end() )
        entries[ name ] = IFFEntryStorage( entry );
    else {
        entries.at( name ).set( entry );
    }

    // thread_lock.unlock();

    return true;
}

void Data::Manager::autoSetEntries( const std::string &base_path, Platform platform ) {
    std::string MACINT_PATH = base_path;
    std::string PSX_CW_PATH = base_path;
    std::string PSX_PA_PATH = base_path;
    std::string WINDOW_PATH = base_path;

    if( platform == Platform::ALL ) {
        MACINT_PATH += "Macintosh/";
        PSX_CW_PATH += "Playstation/";
        PSX_PA_PATH += "Playstation/";
        WINDOW_PATH += "Windows/";
    }

    MACINT_PATH += "missions/";
    PSX_CW_PATH += "cw/";
    PSX_PA_PATH += "pa/";
    WINDOW_PATH += "missions/";

    const bool MAKE_MAC_PATH = (platform == Platform::MACINTOSH)   | (platform == Platform::ALL);
    const bool MAKE_PSX_PATH = (platform == Platform::PLAYSTATION) | (platform == Platform::ALL);
    const bool MAKE_WIN_PATH = (platform == Platform::WINDOWS)     | (platform == Platform::ALL);

    IFFEntry entry = getIFFEntry( global );
    entry.importance = Importance::NEEDED; // The global IFF is always used for loading the IFF mission files.
    if( MAKE_MAC_PATH )
        entry.setPath( Platform::MACINTOSH,   MACINT_PATH + "GlblData" );
    if( MAKE_PSX_PATH )
        entry.setPath( Platform::PLAYSTATION, PSX_CW_PATH + "fe.mis" );
    if( MAKE_WIN_PATH )
        entry.setPath( Platform::WINDOWS,     WINDOW_PATH + "GlblData" );
    setIFFEntry( global, entry );

    entry = getIFFEntry( griffith_park );
    if( MAKE_MAC_PATH )
        entry.setPath( Platform::MACINTOSH,   MACINT_PATH + "M2C" );
    if( MAKE_PSX_PATH )
        entry.setPath( Platform::PLAYSTATION, PSX_CW_PATH + "m2c.mis" );
    if( MAKE_WIN_PATH )
        entry.setPath( Platform::WINDOWS,     WINDOW_PATH + "M2C" );
    setIFFEntry( griffith_park, entry );

    entry = getIFFEntry( zuma_beach );
    if( MAKE_MAC_PATH )
        entry.setPath( Platform::MACINTOSH,   MACINT_PATH + "M3A" );
    if( MAKE_PSX_PATH )
        entry.setPath( Platform::PLAYSTATION, PSX_CW_PATH + "m3a.mis" );
    if( MAKE_WIN_PATH )
        entry.setPath( Platform::WINDOWS,     WINDOW_PATH + "M3A" );
    setIFFEntry( zuma_beach, entry );

    entry = getIFFEntry( la_brea_tar_pits );
    if( MAKE_MAC_PATH )
        entry.setPath( Platform::MACINTOSH,   MACINT_PATH + "M3B" );
    if( MAKE_PSX_PATH )
        entry.setPath( Platform::PLAYSTATION, PSX_CW_PATH + "m3b.mis" );
    if( MAKE_WIN_PATH )
        entry.setPath( Platform::WINDOWS,     WINDOW_PATH + "M3B" );
    setIFFEntry( la_brea_tar_pits, entry );

    entry = getIFFEntry( venice_beach );
    if( MAKE_MAC_PATH )
        entry.setPath( Platform::MACINTOSH,   MACINT_PATH + "OV" );
    if( MAKE_PSX_PATH )
        entry.setPath( Platform::PLAYSTATION, PSX_CW_PATH + "ov.mis" );
    if( MAKE_WIN_PATH )
        entry.setPath( Platform::WINDOWS,     WINDOW_PATH + "OV" );
    setIFFEntry( venice_beach, entry );

    entry = getIFFEntry( hells_gate_prison );
    if( MAKE_MAC_PATH )
        entry.setPath( Platform::MACINTOSH,   MACINT_PATH + "M1A1" );
    if( MAKE_PSX_PATH )
        entry.setPath( Platform::PLAYSTATION, PSX_CW_PATH + "m1a1.mis" );
    if( MAKE_WIN_PATH )
        entry.setPath( Platform::WINDOWS,     WINDOW_PATH + "M1A1" );
    setIFFEntry( hells_gate_prison, entry );

    entry = getIFFEntry( studio_city );
    if( MAKE_MAC_PATH )
        entry.setPath( Platform::MACINTOSH,   MACINT_PATH + "Un" );
    if( MAKE_PSX_PATH )
        entry.setPath( Platform::PLAYSTATION, PSX_CW_PATH + "un.mis" );
    if( MAKE_WIN_PATH )
        entry.setPath( Platform::WINDOWS,     WINDOW_PATH + "Un" );
    setIFFEntry( studio_city, entry );

    entry = getIFFEntry( lax_spaceport );
    if( MAKE_MAC_PATH )
        entry.setPath( Platform::MACINTOSH,   MACINT_PATH + "LAX1" );
    if( MAKE_PSX_PATH )
        entry.setPath( Platform::PLAYSTATION, PSX_CW_PATH + "lax1.mis" );
    if( MAKE_WIN_PATH )
        entry.setPath( Platform::WINDOWS,     WINDOW_PATH + "LAX1" );
    setIFFEntry( lax_spaceport, entry );

    entry = getIFFEntry( lax_spaceport_part_2 );
    if( MAKE_MAC_PATH )
        entry.setPath( Platform::MACINTOSH,   MACINT_PATH + "LAX2" );
    if( MAKE_PSX_PATH )
        entry.setPath( Platform::PLAYSTATION, PSX_CW_PATH + "lax2.mis" );
    if( MAKE_WIN_PATH )
        entry.setPath( Platform::WINDOWS,     WINDOW_PATH + "LAX2" );
    setIFFEntry( lax_spaceport_part_2, entry );

    entry = getIFFEntry( long_beach );
    if( MAKE_MAC_PATH )
        entry.setPath( Platform::MACINTOSH,   MACINT_PATH + "M4A1" );
    if( MAKE_PSX_PATH )
        entry.setPath( Platform::PLAYSTATION, PSX_CW_PATH + "m4a1.mis" );
    if( MAKE_WIN_PATH )
        entry.setPath( Platform::WINDOWS,     WINDOW_PATH + "M4A1" );
    setIFFEntry( long_beach, entry );

    entry = getIFFEntry( pa_urban_jungle );
    if( MAKE_MAC_PATH )
        entry.setPath( Platform::MACINTOSH,   MACINT_PATH + "ConFt" );
    if( MAKE_PSX_PATH )
        entry.setPath( Platform::PLAYSTATION, PSX_PA_PATH + "conft.mis" );
    if( MAKE_WIN_PATH )
        entry.setPath( Platform::WINDOWS,     WINDOW_PATH + "ConFt" );
    setIFFEntry( pa_urban_jungle, entry );

    entry = getIFFEntry( pa_venice_beach );
    if( MAKE_MAC_PATH )
        entry.setPath( Platform::MACINTOSH,   MACINT_PATH + "OVMP" );
    if( MAKE_PSX_PATH )
        entry.setPath( Platform::PLAYSTATION, PSX_PA_PATH + "ovmp.mis" );
    if( MAKE_WIN_PATH )
        entry.setPath( Platform::WINDOWS,     WINDOW_PATH + "OVMP" );
    setIFFEntry( pa_venice_beach, entry );

    entry = getIFFEntry( pa_hollywood_keys );
    if( MAKE_MAC_PATH )
        entry.setPath( Platform::MACINTOSH,   MACINT_PATH + "HK" );
    if( MAKE_PSX_PATH )
        entry.setPath( Platform::PLAYSTATION, PSX_PA_PATH + "hk.mis" );
    if( MAKE_WIN_PATH )
        entry.setPath( Platform::WINDOWS,     WINDOW_PATH + "HK" );
    setIFFEntry( pa_hollywood_keys, entry );

    entry = getIFFEntry( pa_proving_ground );
    if( MAKE_MAC_PATH )
        entry.setPath( Platform::MACINTOSH,   MACINT_PATH + "Slim" );
    if( MAKE_PSX_PATH )
        entry.setPath( Platform::PLAYSTATION, PSX_PA_PATH + "slim.mis" );
    if( MAKE_WIN_PATH )
        entry.setPath( Platform::WINDOWS,     WINDOW_PATH + "Slim" );
    setIFFEntry( pa_proving_ground, entry );

    entry = getIFFEntry( pa_bug_hunt );
    if( MAKE_MAC_PATH )
        entry.setPath( Platform::MACINTOSH,   MACINT_PATH + "JOKE" );
    if( MAKE_PSX_PATH )
        entry.setPath( Platform::PLAYSTATION, PSX_PA_PATH + "joke.mis" );
    if( MAKE_WIN_PATH )
        entry.setPath( Platform::WINDOWS,     WINDOW_PATH + "JOKE" );
    setIFFEntry( pa_bug_hunt, entry );

    entry = getIFFEntry( pa_la_centina );
    if( MAKE_MAC_PATH )
        entry.setPath( Platform::MACINTOSH,   MACINT_PATH + "Mp" );
    if( MAKE_PSX_PATH )
        entry.setPath( Platform::PLAYSTATION, PSX_PA_PATH + "mp.mis" ); // Playstation does not have this
    if( MAKE_WIN_PATH )
        entry.setPath( Platform::WINDOWS,     WINDOW_PATH + "Mp" );
    setIFFEntry( pa_la_centina, entry );
}

void Data::Manager::togglePlatform( Platform platform, bool can_be_loaded ) {
    // thread_lock.lock();

    switch( platform ) {
        case MACINTOSH:
            platforms_to_support.toggle[ Platform::MACINTOSH ]   = can_be_loaded;
            break;
        case PLAYSTATION:
            platforms_to_support.toggle[ Platform::PLAYSTATION ] = can_be_loaded;
            break;
        case WINDOWS:
            platforms_to_support.toggle[ Platform::WINDOWS ]     = can_be_loaded;
            break;
        default:
            platforms_to_support.toggle[ Platform::MACINTOSH ]   = can_be_loaded;
            platforms_to_support.toggle[ Platform::PLAYSTATION ] = can_be_loaded;
            platforms_to_support.toggle[ Platform::WINDOWS ]     = can_be_loaded;
    }

    // thread_lock.unlock();
}

int Data::Manager::setLoad( Importance importance, unsigned core_amount ) {
    int number_loaded = 0;
    Platform platforms[] = { Platform::MACINTOSH, Platform::PLAYSTATION, Platform::WINDOWS };

    // thread_lock.lock();

    for( auto &p : platforms ) {
        if( !platforms_to_support.toggle[ p ] ) {
            if( currently_loaded_platforms.toggle[ p ] ) {
                for( auto &i : entries ) {
                    if( i.second.getIFF( p ) != nullptr )
                        i.second.unload( p );
                }
            }
        }
        else {
            for( auto &i : entries ) {
                if( i.second.importance <= importance ) {
                    if( i.second.getIFF( p ) == nullptr ) {
                        i.second.load( p );
                        number_loaded++;
                    }
                }
                else
                if( i.second.getIFF( p ) != nullptr )
                    i.second.unload( p );
            }
        }
    }

    // After loading and unloading the resources are done.
    currently_loaded_platforms = platforms_to_support;

    // thread_lock.unlock();

    return number_loaded;
}

int Data::Manager::reload( unsigned core_amount ) {
    int number_reload = 0;
    Platform platforms[] = { Platform::MACINTOSH, Platform::PLAYSTATION, Platform::WINDOWS };

    // thread_lock.lock();

    for( auto &i : entries ) {
        for( auto &d : platforms ) {
            if( i.second.getIFF( d ) != nullptr ) {
                i.second.unload( d );
                i.second.load( d );
                number_reload++;
            }
        }
    }

   //  thread_lock.unlock();

    return number_reload;
}

void Data::Manager::listIDs( std::ostream &stream ) {
    stream << "Printing all map IDs\n";
    for( size_t i = 0; i < AMOUNT_OF_IFF_IDS; i++ ) {
        stream << " " << *map_iffs[ i ] << "\n";
    }
    stream << std::endl;
}

Data::Manager::Platform Data::Manager::getPlatformFromString( const std::string &name ) {
    for( auto i : name ) {
        auto character = toupper( i );

        if( character == 'M' )
            return Platform::MACINTOSH;
        else
        if( character == 'P' )
            return Platform::PLAYSTATION;
        else
        if( character == 'W' )
            return Platform::WINDOWS;
    }

    return Platform::WINDOWS;
}
