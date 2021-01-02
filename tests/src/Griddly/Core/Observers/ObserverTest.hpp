#pragma once
#include "Griddly/Core/TestUtils/common.hpp"

namespace griddly {

auto mockAvatarObjectPtr = std::shared_ptr<MockObject>(new MockObject());
auto mockSinglePlayerObject1Ptr = mockObject("mo1", 1, 0);
auto mockSinglePlayerObject2Ptr = mockObject("mo2", 1, 1);
auto mockSinglePlayerObject3Ptr = mockObject("mo3", 1, 2);

auto mockSinglePlayerObjects = std::unordered_set<std::shared_ptr<Object>>{mockSinglePlayerObject1Ptr, mockSinglePlayerObject2Ptr, mockSinglePlayerObject3Ptr};

const std::unordered_map<glm::ivec2, TileObjects> mockSinglePlayerGridData = {
    {{0, 0}, {{0, mockSinglePlayerObject1Ptr}}},
    {{1, 0}, {{0, mockSinglePlayerObject1Ptr}}},
    {{2, 0}, {{0, mockSinglePlayerObject1Ptr}}},
    {{3, 0}, {{0, mockSinglePlayerObject1Ptr}}},
    {{4, 0}, {{0, mockSinglePlayerObject1Ptr}}},

    {{0, 1}, {{0, mockSinglePlayerObject1Ptr}}},
    {{1, 1}, {{0, mockSinglePlayerObject2Ptr}}},
    {{2, 1}, {{}}},
    {{3, 1}, {{0, mockSinglePlayerObject3Ptr}}},
    {{4, 1}, {{0, mockSinglePlayerObject1Ptr}}},

    {{0, 2}, {{0, mockSinglePlayerObject1Ptr}}},
    {{1, 2}, {{0, mockSinglePlayerObject2Ptr}}},
    {{2, 2}, {{0, mockAvatarObjectPtr}}},
    {{3, 2}, {{0, mockSinglePlayerObject3Ptr}}},
    {{4, 2}, {{0, mockSinglePlayerObject1Ptr}}},

    {{0, 3}, {{0, mockSinglePlayerObject1Ptr}}},
    {{1, 3}, {{0, mockSinglePlayerObject3Ptr}}},
    {{2, 3}, {{}}},
    {{3, 3}, {{0, mockSinglePlayerObject2Ptr}}},
    {{4, 3}, {{0, mockSinglePlayerObject1Ptr}}},

    {{0, 4}, {{0, mockSinglePlayerObject1Ptr}}},
    {{1, 4}, {{0, mockSinglePlayerObject1Ptr}}},
    {{2, 4}, {{0, mockSinglePlayerObject1Ptr}}},
    {{3, 4}, {{0, mockSinglePlayerObject1Ptr}}},
    {{4, 4}, {{0, mockSinglePlayerObject1Ptr}}},
};

const std::unordered_set<glm::ivec2> mockSinglePlayerUpdatedLocations = {
        {0, 0},
        {0, 1},
        {0, 2},
        {0, 3},
        {0, 4},
        {1, 0},
        {1, 1},
        {1, 2},
        {1, 3},
        {1, 4},
        {2, 0},
        {2, 1},
        {2, 2},
        {2, 3},
        {2, 4},
        {3, 0},
        {3, 1},
        {3, 2},
        {3, 3},
        {3, 4},
        {4, 0},
        {4, 1},
        {4, 2},
        {4, 3},
        {4, 4},
};
}  // namespace griddly