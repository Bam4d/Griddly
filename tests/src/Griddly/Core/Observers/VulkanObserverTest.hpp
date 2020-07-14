#define STB_IMAGE_STATIC
#define STB_IMAGE_WRITE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image.h>
#include <stb/stb_image_write.h>

#include <memory>

#include "Griddly/Core/GDY/Objects/Object.hpp"
#include "Griddly/Core/Grid.hpp"
#include "Mocks/Griddly/Core/GDY/Objects/MockObject.cpp"
#include "Mocks/Griddly/Core/MockGrid.cpp"

using ::testing::Return;

namespace griddly {

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

inline std::shared_ptr<MockObject> mockObject(uint32_t playerId, uint32_t objectId, std::string objectName, DiscreteOrientation orientation = DiscreteOrientation()) {
  auto mockObjectPtr = std::shared_ptr<MockObject>(new MockObject());

  EXPECT_CALL(*mockObjectPtr, getPlayerId()).WillRepeatedly(Return(playerId));
  EXPECT_CALL(*mockObjectPtr, getObjectId()).WillRepeatedly(Return(objectId));
  EXPECT_CALL(*mockObjectPtr, getObjectName()).WillRepeatedly(Return(objectName));
  EXPECT_CALL(*mockObjectPtr, getObjectOrientation()).WillRepeatedly(Return(orientation));
  EXPECT_CALL(*mockObjectPtr, getObjectRenderTileName()).WillRepeatedly(Return(objectName+std::to_string(0)));

  return mockObjectPtr;
}
}