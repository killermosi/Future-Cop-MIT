#include "ImageDraw.h"

namespace Graphics::SDL2::GLES2::Internal {

ImageDraw::ImageDraw() {}

ImageDraw::~ImageDraw() {}

void ImageDraw::draw(Graphics::SDL2::GLES2::Camera& camera) {}

void ImageDraw::updateImageData(const Texture2D *const, const ImageData *const image_r, const ImageData& image_data) {}

void ImageDraw::removeImageData(const Texture2D *const, const ImageData *const image_r) {}

void ImageDraw::updateExternalImageData(const ExternalImage *const external_image_r, const ExternalImageData& external_image_data) {}

void ImageDraw::removeExternalImageData(const ExternalImage *const external_image_r) {}

}
