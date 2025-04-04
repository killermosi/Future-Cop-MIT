#include "ImageDraw2D.h"

#include "../Environment.h"
#include "../Image.h"

namespace Graphics::SDL2::Software::Internal {

void ImageDraw2D::draw(Window::RenderingRect &rendering_rect) const {
    for( auto i : this->images ) {
        if(!i->internal.is_visable)
            continue;

        if(i->internal.positions[1].x < i->internal.positions[0].x)
            std::swap(i->internal.positions[0].x, i->internal.positions[1].x);
        if(i->internal.positions[1].y < i->internal.positions[0].y)
            std::swap(i->internal.positions[0].y, i->internal.positions[1].y);

        glm::vec2 pos_0 = i->internal.positions[0] * rendering_rect.env_r->window_p->inv_factor;
        glm::vec2 pos_1 = i->internal.positions[1] * rendering_rect.env_r->window_p->inv_factor;
        glm::vec2 scale = pos_1 - pos_0;

        glm::i32vec2 screen_pos_0 = glm::clamp(
            glm::i32vec2(pos_0),
            glm::i32vec2(rendering_rect.area.start_x, rendering_rect.area.start_y),
            glm::i32vec2(rendering_rect.area.end_x,   rendering_rect.area.end_y) );

        glm::i32vec2 screen_pos_1 = glm::clamp(
            glm::i32vec2(pos_1),
            glm::i32vec2(rendering_rect.area.start_x, rendering_rect.area.start_y),
            glm::i32vec2(rendering_rect.area.end_x,   rendering_rect.area.end_y) );

        Window::DifferredPixel default_pixel;
        default_pixel.colors[0] = 0xff * i->internal.color.r;
        default_pixel.colors[1] = 0xff * i->internal.color.g;
        default_pixel.colors[2] = 0xff * i->internal.color.b;
        default_pixel.colors[3] = 0;
        default_pixel.depth  = 0;
        default_pixel.depth -= 2;

        float alpha;

        for(auto y = screen_pos_0.y; y != screen_pos_1.y; y++) {
            float a = (pos_1.y - y) / scale.y;
            float p =  i->internal.texture_coords[1].y * (1. - a) + i->internal.texture_coords[0].y * a;
            default_pixel.texture_coordinates[1] = 0xff * p;

            for(auto x = screen_pos_0.x; x != screen_pos_1.x; x++) {
                Window::DifferredPixel original_pixel = rendering_rect.differred_buffer.getValue(x - rendering_rect.area.start_x, y - rendering_rect.area.start_y);

                if(original_pixel.depth > default_pixel.depth)
                    continue;

                float a = (pos_1.x - x) / scale.x;
                float p =  i->internal.texture_coords[1].x * (1. - a) + i->internal.texture_coords[0].x * a;
                default_pixel.texture_coordinates[0] = 0xff * p;

                Window::DifferredPixel source_pixel = default_pixel;
                alpha = i->internal.color.a;

                if(i->internal.cbmp_index != 0) {
                    auto slot = rendering_rect.env_r->textures[i->internal.cbmp_index];
                    auto texture_pixel = slot.texture_p->getValue( default_pixel.texture_coordinates[0], default_pixel.texture_coordinates[1] );

                    if(static_cast<unsigned>(texture_pixel.data[3]) == 0)
                        continue;

                    source_pixel.colors[0] = (static_cast<unsigned>(default_pixel.colors[0]) * static_cast<unsigned>(texture_pixel.data[0])) >> 8;
                    source_pixel.colors[1] = (static_cast<unsigned>(default_pixel.colors[1]) * static_cast<unsigned>(texture_pixel.data[1])) >> 8;
                    source_pixel.colors[2] = (static_cast<unsigned>(default_pixel.colors[2]) * static_cast<unsigned>(texture_pixel.data[2])) >> 8;

                    alpha *= 1.f / 255.f * texture_pixel.data[3];
                }

                source_pixel.colors[0] = source_pixel.colors[0] * (1.f - alpha) + original_pixel.colors[0] * alpha;
                source_pixel.colors[1] = source_pixel.colors[1] * (1.f - alpha) + original_pixel.colors[1] * alpha;
                source_pixel.colors[2] = source_pixel.colors[2] * (1.f - alpha) + original_pixel.colors[2] * alpha;

                rendering_rect.differred_buffer.setValue(x - rendering_rect.area.start_x, y - rendering_rect.area.start_y, source_pixel);
            }
        }
    }
}

}
