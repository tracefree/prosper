#pragma once

struct AllocatedImage;

struct Texture {
    std::unique_ptr<AllocatedImage> image;
};