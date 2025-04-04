#ifndef GRAPHICS_GLES2_INTERNAL_DRAW_2D_H
#define GRAPHICS_GLES2_INTERNAL_DRAW_2D_H

#include "Texture2D.h"
#include "../Camera.h"
#include "FontSystem.h"

#include <cstdint>
#include <map>
#include <set>
#include <vector>

namespace Graphics {

class ExternalImage;
class ImageBase;

namespace SDL2::GLES2 {

class ExternalImage;
class Image;

namespace Internal {

class Draw2D {
public:
    FontSystem *text_draw_routine_p;

    struct Vertex {
        float x, y;
        float u, v;
        uint32_t color_rgba;

        Vertex() {}
        Vertex( float set_x, float set_y, float set_u, float set_v,
            uint32_t set_color_rgba ) :
            x( set_x ), y( set_y ), u( set_u ), v( set_v ),
            color_rgba( set_color_rgba )
        {
        }
        void set( float set_x, float set_y, float set_u, float set_v,
            uint32_t set_color_rgba ) {
            x = set_x, y = set_y, u = set_u, v = set_v;
            color_rgba = set_color_rgba;
        }
    };

    static constexpr uint8_t DEFAULT_COLOR[4] = { 0xFF, 0xFF, 0xFF, 0xFF };

    struct Color {
        uint32_t color_rgba;
        uint8_t* toPointer() { return reinterpret_cast<uint8_t*>(&color_rgba); }
        void set( uint8_t r, uint8_t b, uint8_t g, uint8_t a ) {
            uint8_t *color = toPointer();
            color[0] = r;
            color[1] = b;
            color[2] = g;
            color[3] = a;
        }
    };

    struct ImageBaseData {
        glm::vec2 positions[2];
        glm::vec4 color;
        bool visable;
    };

    struct ImageData : public ImageBaseData {
        glm::vec2 texture_coords[2];
    };

    struct DynamicImageData : public ImageBaseData {
        Texture2D *texture_2d;
        GLsizei width, height;
    };

    static const GLchar* default_vertex_shader;
    static const GLchar* default_fragment_shader;

protected:
    Program program;
    std::vector<Shader::Attribute> attributes;
    std::vector<Shader::Varying>   varyings;
    Shader  vertex_shader;
    Shader  fragment_shader;

    VertexAttributeArray vertex_array;
    GLuint texture_uniform_id;
    GLuint matrix_uniform_id;

    GLuint vertex_buffer_object;
    size_t max_triangles;
    Vertex *buffer_p;

    std::map<const Texture2D *const, std::map<const Image *const, ImageData>> images;
    std::map<const Graphics::ImageBase *const, DynamicImageData> dynamic_images;

public:
    std::set<GLES2::Text2DBuffer*> text_2d_buffers;

    Draw2D();
    virtual ~Draw2D();

    /**
     * This method gets the default vertex shader depending on the GL version.
     * @warning make sure the correct context is binded, or else you would get an improper shader.
     * @return a pointer to a vertex shader.
     */
    static const GLchar* getDefaultVertexShader();

    /**
     * This method gets the default fragment shader depending on the GL version.
     * @warning make sure the correct context is binded, or else you would get an improper shader.
     * @return a pointer to a fragment shader.
     */
    static const GLchar* getDefaultFragmentShader();

    /**
     * This sets up and compiles this shader from memory.
     * This is to be used for internal shaders.
     * @param shader_source The memory pointer to the source code of the shader.
     */
    void setVertexShader( const GLchar *const shader_source = getDefaultVertexShader() );

    /**
     * This loads a shader from a text file, and compiles it.
     * @param shader_source The memory pointer to the source code of the shader.
     */
    int loadVertexShader( const char *const file_path );

    /**
     * This sets up and compiles this shader from memory.
     * This is to be used for internal shaders.
     * @param shader_source The memory pointer to the source code of the shader.
     */
    void setFragmentShader( const GLchar *const shader_source = getDefaultFragmentShader() );

    /**
     * This loads a shader from a text file, and compiles it.
     * @param shader_source The memory pointer to the source code of the shader.
     */
    int loadFragmentShader( const char *const file_path );

    /**
     * Link every shader to the program.
     * @return false if one of the shaders are not loaded.
     */
    int compileProgram();

    /**
     * @return vertex buffer size in bytes.
     */
    static size_t getVertexSize() { return sizeof( Vertex ); }

    const VertexAttributeArray *const getVertexAttributeArray() const { return &vertex_array; };

    /**
     * Draw all the images to be render
     * @param camera This parameter gets the matrix
     */
    void draw(Graphics::SDL2::GLES2::Camera& camera);

    void updateImageData(const Texture2D *const internal_texture_r, const Image *const image_r, const ImageData& image_data);

    void removeImageData(const Texture2D *const internal_texture_r, const Image *const image_r);

    void updateDynamicImageData(const Graphics::ImageBase *const image_base_r, const DynamicImageData& dynamic_image_data);

    void uploadDynamicImageData(const Graphics::ImageBase *const image_base_r, const Utilities::Image2D& image_2d, GLenum image_gl_format);

    void removeDynamicImageData(const Graphics::ImageBase *const image_base_r);

    void clear();
};

}

}

}



#endif // GRAPHICS_GLES2_INTERNAL_DRAW_2D_H
