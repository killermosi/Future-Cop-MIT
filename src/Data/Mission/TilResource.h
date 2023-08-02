#ifndef MISSION_RESOURCE_TILE_HEADER
#define MISSION_RESOURCE_TILE_HEADER

#include "ModelResource.h"
#include "BMPResource.h"
#include "../../Utilities/GridBase2D.h"
#include "../../Utilities/Collision/Ray.h"
#include "../../Utilities/Collision/Triangle.h"

namespace Data {

namespace Mission {

/**
 * Til Resource Reader.
 *
 * This is the Til resource it holds the resource responsible for holding a chunk of a map.
 * In Future Cop, this resource has 16x16 tiles in it. Its size is 131072x131072.
 */
class TilResource : public ModelResource {
public:
    static const std::string FILE_EXTENSION;
    static const uint32_t IDENTIFIER_TAG;

    struct HeightmapPixel {
        int8_t channel[3];
    };
    struct CullingTile {
        uint16_t primary;

        uint16_t top_left;
        uint16_t top_right;
        uint16_t bottom_left;
        uint16_t bottom_right;
    };
    struct Floor {
        uint16_t tile_amount : 6;
        uint16_t tiles_start : 10;
        
        Floor() {}
        Floor( const uint16_t bitfield ) {
            set( bitfield );
        }
        
        void set( const uint16_t bitfield ) {
            tile_amount = (bitfield >> 0) & ((1 <<  6) - 1);
            tiles_start = (bitfield >> 6) & ((1 << 10) - 1);
        }
    };
    struct Tile {
        uint32_t end_column: 1;
        uint32_t texture_cord_index : 10;
        uint32_t front : 1;
        uint32_t back  : 1;
        uint32_t unknown_1 : 2; // Apperently this holds what this tile would do to the playable character. However, it appears that the action this tile would do to the player is stored elsewhere.
        uint32_t mesh_type : 7;
        uint32_t graphics_type_index : 10;
        
        Tile() {}
        Tile( const uint32_t bitfield ) {
            set( bitfield );
        }
        
        void set( const uint32_t bitfield ) {
            end_column          = (bitfield >>  0) & 1;
            texture_cord_index  = (bitfield >>  1) & ((1 << 10) - 1);
            front               = (bitfield >> 11) & 1;
            back                = (bitfield >> 12) & 1;
            unknown_1           = (bitfield >> 13) & ((1 <<  2) - 1);
            mesh_type           = (bitfield >> 15) & ((1 <<  7) - 1);
            graphics_type_index = (bitfield >> 22) & ((1 << 10) - 1);
        }
    };
    struct DynamicMonoGraphics {
        uint16_t forth : 6;
        uint16_t third : 6;
        uint16_t second_lower : 4;
        
        DynamicMonoGraphics() {}
        DynamicMonoGraphics( const uint16_t bitfield ) {
            set( bitfield );
        }
        
        void set( const uint16_t bitfield ) {
            forth        = (bitfield >>  0) & ((1 << 6) - 1);
            third        = (bitfield >>  6) & ((1 << 6) - 1);
            second_lower = (bitfield >> 12) & ((1 << 4) - 1);
        }
    };
    struct DynamicColorGraphics {
        uint16_t third  : 8;
        uint16_t second : 8;
        
        DynamicColorGraphics() {}
        DynamicColorGraphics( const uint16_t bitfield ) {
            set( bitfield );
        }
        
        void set( const uint16_t bitfield ) {
            third  = (bitfield >>  0) & ((1 << 8) - 1);
            second = (bitfield >>  8) & ((1 << 8) - 1);
        }
    };
    struct TileGraphics {
        uint16_t shading : 8; // Lighting information, but they do change meaning depending type bitfield
        uint16_t texture_index : 3; // Holds the index of the texture the tile references.
        uint16_t animated : 1;
        uint16_t semi_transparent : 1;
        uint16_t rectangle : 1; // Indicates rectangle/square?
        uint16_t type : 2; // Tells how the tile will be drawn.
        
        TileGraphics() {}
        TileGraphics( const uint16_t bitfield ) {
            set( bitfield );
        }
        
        void set( const uint16_t bitfield ) {
            shading          = (bitfield >>  0) & ((1 << 8) - 1);
            texture_index    = (bitfield >>  8) & ((1 << 3) - 1);
            animated         = (bitfield >> 11) & 1;
            semi_transparent = (bitfield >> 12) & 1;
            rectangle        = (bitfield >> 13) & 1;
            type             = (bitfield >> 14) & ((1 << 2) - 1);
        }
        
        uint16_t get() const {
            return ((uint16_t)shading << 0) |
                ((uint16_t)texture_index << 8) |
                ((uint16_t)animated << 11) |
                ((uint16_t)semi_transparent << 12) |
                ((uint16_t)rectangle << 13) |
                ((uint16_t)type << 14);
        }
    };
    struct InfoSCTA {
        static constexpr float units_to_seconds = 1. / 300.;
        static constexpr float seconds_to_units = 300.;

        int_fast32_t  frame_count;
        uint_fast32_t duration_per_frame;
        uint_fast32_t animated_uv_offset;
        uint_fast32_t source_uv_offset;

        std::string getString() const;

        int_fast32_t getFrameCount() const { return std::abs( frame_count ); }
        float getSecondsPerFrame() const { return duration_per_frame * units_to_seconds; }
        float getSecondsPerCycle() const { return getSecondsPerFrame() * getFrameCount(); }
        float getDurationToSeconds() const { return seconds_to_units / duration_per_frame; }

        bool isMemorySafe() const { return frame_count >= 0; }

        /**
         * This method sets the variables inside the struct to be memory safe.
         * @return false if an element is found to be unstable.
         */
        bool setMemorySafe( size_t source_size, size_t animated_size );
    };
    
    static const std::string TILE_TYPE_COMPONENT_NAME;
    static constexpr size_t AMOUNT_OF_TILES = 16;
    static constexpr float  SPAN_OF_TIL = AMOUNT_OF_TILES / 2;
    static constexpr float MAX_HEIGHT = 6.0f; // The highest value is actually MAX_HEIGHT - SAMPLE_HEIGHT due to the span of the pixels being [127, -128].
    static constexpr float MIN_HEIGHT = -MAX_HEIGHT;
    static constexpr float SAMPLE_HEIGHT = (2.0 * MAX_HEIGHT) / 256.0f; // 256 values is contained in a pixel.
private:
    Utilities::GridBase2D<HeightmapPixel> point_cloud_3_channel; // I liked the Point Cloud Name. These are 3 channel signed bytes.

    uint16_t culling_distance; // This affects the radius of the circle where the culling happens
    CullingTile culling_top_left;
    CullingTile culling_top_right;
    CullingTile culling_bottom_left;
    CullingTile culling_bottom_right;

    glm::i8vec2 uv_animation;

    uint16_t texture_reference; // This is an unknown number, but it affects all the textures in the resource. One change will mess up the tiles.
    Floor mesh_reference_grid[ AMOUNT_OF_TILES ][ AMOUNT_OF_TILES ];

    uint16_t mesh_library_size;
    std::vector<Tile> mesh_tiles; // These are descriptions of tiles that are used to make up the map format. The 32 bit numbers are packed with information

    std::vector<glm::u8vec2> texture_cords; // They contain the UV's for the tiles, they are often read as quads
    std::vector<Utilities::PixelFormatColor::GenericColor> colors;
    std::vector<uint16_t> tile_graphics_bitfield;

    std::vector<InfoSCTA> SCTA_info;
    std::vector<glm::u8vec2> scta_texture_cords;
    
    uint32_t slfx_bitfield;
    
    struct TextureInfo {
        std::string name;
    };
    TextureInfo texture_info[8]; // There can only be 2*2*2 or 8 texture resource IDs.
    
    std::vector<Utilities::Collision::Triangle> all_triangles; // This stores all the triangles in the Til Resource.
public:
    static constexpr size_t TEXTURE_INFO_AMOUNT = sizeof( texture_info ) / sizeof( texture_info[0] );
    
    TilResource();
    TilResource( const TilResource &obj );

    virtual std::string getFileExtension() const;

    virtual uint32_t getResourceTagID() const;

    Utilities::Image2D getImage() const;
    
    void makeEmpty();

    virtual bool parse( const ParseSettings &settings = Data::Mission::Resource::DEFAULT_PARSE_SETTINGS );

    virtual Resource * duplicate() const;

    bool loadTextures( const std::vector<BMPResource*> &textures );

    virtual int write( const std::string& file_path, const Data::Mission::IFFOptions &iff_options = IFFOptions() ) const;

    virtual Utilities::ModelBuilder * createModel() const { return createModel( false ); }
    virtual Utilities::ModelBuilder * createCulledModel() const { return createModel( true ); }

    virtual Utilities::ModelBuilder * createModel( bool is_culled, Utilities::Logger &logger = Utilities::logger ) const;
    
    Utilities::ModelBuilder * createPartial( unsigned int texture_index, bool is_culled = false, float x_offset = 0.0f, float z_offset = 0.0f, Utilities::Logger &logger = Utilities::logger ) const;
    
    void createPhysicsCell( unsigned int x, unsigned int z );
    
    float getRayCast3D( const Utilities::Collision::Ray &ray ) const;
    float getRayCast2D( float x, float y ) const;
    float getRayCastDownward( float x, float y, float from_highest_point ) const;

    const std::vector<Utilities::Collision::Triangle>& getAllTriangles() const;
    Utilities::Image2D getHeightMap( unsigned int rays_per_tile = 4 ) const;
    
    glm::i8vec2 getUVAnimation() const { return uv_animation; }
    const std::vector<InfoSCTA>& getInfoSCTA() const { return SCTA_info; }
    const std::vector<glm::u8vec2>& getSCTATextureCords() const { return scta_texture_cords; }

    static std::vector<TilResource*> getVector( IFF &mission_file );
    static const std::vector<TilResource*> getVector( const IFF &mission_file );
};

}

}

#endif // MISSION_RESOURCE_TILE_HEADER
