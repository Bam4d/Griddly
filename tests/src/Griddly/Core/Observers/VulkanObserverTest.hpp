#pragma once 
#define STB_IMAGE_STATIC
#define STB_IMAGE_WRITE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image.h>

#include <stb/stb_image_write.h>

inline std::unique_ptr<uint8_t[]> loadExpectedImage(std::string filename) {
  int width, height, channels;

  stbi_uc* pixels = stbi_load(filename.c_str(), &width, &height, &channels, STBI_rgb);

  if (!pixels) {
    throw std::runtime_error("Failed to load texture image.");
  }

  auto spriteSize = width * height * channels;

  std::unique_ptr<uint8_t[]> spriteData(pixels);

  return std::move(spriteData);
}

inline int write_image(std::string filename, uint8_t* imageData, int stride, int width, int height) {
  return stbi_write_png(filename.c_str(), width, height, 3, imageData, stride);
}