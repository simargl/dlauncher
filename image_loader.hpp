#pragma once

#include <FL/Fl_PNG_Image.H>
#include <FL/Fl_Image.H>
#include <string>
#include <filesystem>

class ImageLoader {
public:
    static Fl_Image* loadImage(const std::string& filename);
};
