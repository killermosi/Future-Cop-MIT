#ifndef UTILITIES_OPTIONS_PARAMETERS_H
#define UTILITIES_OPTIONS_PARAMETERS_H

#include <filesystem>
#include <ostream>
#include <string.h>

namespace Utilities {
namespace Options {

/**
 * Handle CLI parameters parsing and basic validation
 *
 * Note: object is intended to be immutable once the parameters gets loaded.
 */
class Parameters {
public:
    static constexpr int RES_WIDTH_LIMIT = 320;
    static constexpr int RES_HEIGHT_LIMIT = 240;

public:
    Parameters() : is_initialized(false) {}
    Parameters(int argc, char *argv[]);
    ~Parameters() { /* empty */ }

    /**
     * This gathers the parameters that the user wants.
     * @warning do not call this when the class is either initialized with arguments or with getParameters already called.
     * @throws std::invalid_argument if an invalid arguement was found.
     * @throws std::runtime_error if the parameters are read twice or more.
     * @param argc This is the number of arguments.
     * @param argv The pointers to the arguements.
     * @param output The help dialogs place.
     * @return True if help is requested, false if help is not requested.
     */
    bool getParameters( int argc, char *argv[], std::ostream &output );

    std::string getBinaryName() const { return this->binary_name; }

public:
    /**
     * Single parameter definition - boolean
     */
    class BoolParam {
    public:
        bool wasModified() const {return modified;}
        bool getValue()    const {return value;   }

        BoolParam()               {/* empty */};
        BoolParam(bool init_value) {value = init_value; modified = true;}
    private:
        bool modified = false;
        bool value    = false;
    };
    /**
     * Single parameter definition - integer
     */
    class IntParam {
    public:
        bool wasModified() const {return modified;}
        int  getValue()    const {return value;   }

        IntParam()              {};
        IntParam(int init_value) {value = init_value; modified = true;}
    private:
        bool modified = false;
        int  value    = 0;
    };
    /**
     * Single parameter definition - string
     */
    class StringParam {
    public:
        bool        wasModified() const {return modified;}
        std::string getValue()    const {return value;   }

        StringParam()                      {};
        StringParam(std::string init_value) {value = init_value; modified = true;}
    private:
        bool modified     = false;
        std::string value = "";
    };
    /**
     * Single parameter definition - path
     */
    class PathParam {
    public:
        bool                  wasModified() const {return modified;}
        std::filesystem::path getValue()    const {return value;   }

        PathParam()                      {};
        PathParam(std::filesystem::path init_value) {value = init_value; modified = true;}
    private:
        bool modified               = false;
        std::filesystem::path value = "";
    };

    // These should be "read only public members", if the code is correct
    const BoolParam&   help          = p_help;         // Help screen
    const BoolParam&   full_screen   = p_full_screen;  // If the game should run in full-screen mode
    const IntParam&    res_width     = p_res_width;    // Display resolution width
    const IntParam&    res_height    = p_res_height;   // Display resolution height
    const PathParam&   config_dir    = p_config_dir;   // Game configuration directory
    const PathParam&   export_dir    = p_export_dir;   // The export path to where the exported resources go.
    const PathParam&   user_dir      = p_user_dir;     // User directory (mods, screenshots, etc)
    const PathParam&   win_data_dir  = p_win_data_dir; // Windows Game data directory (original Future Cop LAPD game data)
    const PathParam&   mac_data_dir  = p_mac_data_dir; // Macintosh Game data directory (original Future Cop LAPD game data)
    const PathParam&   psx_data_dir  = p_psx_data_dir; // Playstation Game data directory (original Future Cop LAPD game data)
    const BoolParam&   load_all_maps = p_load_all_maps;
    const PathParam&   global_path   = p_global_path;
    const PathParam&   mission_path  = p_mission_path;
    const BoolParam&   embedded_map  = p_embedded_map;

// Internal stuff
private:
    bool is_initialized;

    BoolParam p_help;         // Help screen
    BoolParam p_full_screen;  // If the game should run in full-screen mode
    IntParam  p_res_width;    // Display resolution width
    IntParam  p_res_height;   // Display resolution height
    PathParam p_config_dir;   // Game configuration directory
    PathParam p_export_dir;   // The export path to where the exported resources go.
    PathParam p_user_dir;     // User directory (mods, screenshots, etc)
    PathParam p_win_data_dir; // Windows Game data directory (original Future Cop LAPD game data)
    PathParam p_mac_data_dir; // Macintosh Game data directory (original Future Cop LAPD game data)
    PathParam p_psx_data_dir; // Playstation Game data directory (original Future Cop LAPD game data)
    BoolParam p_load_all_maps;
    PathParam p_global_path;
    PathParam p_mission_path;
    BoolParam p_embedded_map;

    // Help
    std::string binary_name;
    virtual void printHelp( std::ostream &output ) const;

    // Options parsing
    virtual void parseOptions(int argc, char *argv[]);

    virtual void parseHelp();
    virtual void parseFullscreen();
    virtual void parseWindow();
    virtual void parseWidth( std::string value );
    virtual void parseHeight( std::string value );
    virtual void parseRes( std::string value );
    virtual void parseConfigDir( std::filesystem::path directory );
    virtual void parseExportDir( std::filesystem::path directory );
    virtual void parseUserDir( std::filesystem::path directory );
    virtual void parseWindowsDataDir( std::filesystem::path directory );
    virtual void parseMacintoshDataDir( std::filesystem::path directory );
    virtual void parsePlaystationDataDir( std::filesystem::path directory );
    virtual void parseLoadAllMaps( std::string value );
    virtual void parseGlobalPath( std::filesystem::path path );
    virtual void parseMissionPath( std::filesystem::path path );
    virtual void parseEmbeddedMap();

    // Errors management
    std::string error_message;
    void storeError(std::string error) {
        // Store only the first encountered error
        if (error_message.empty()) {
            error_message = error;
        }
    }

    /**
     * Check if a string represents an integer
     *
     * @warning Whitespace on both ends will make it invalid.
     * @param str The string that will be tested.
     * @return if str represents an integer.
     */
    bool isPint(const std::string &str) {
        for( auto ch : str ) {
            if( ch < '0' || ch > '9' )
                return false;
        }
        return true;
    }
};

}
}

#endif // UTILITIES_OPTIONS_PARAMETERS_H


