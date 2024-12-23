/*  Author: simargl <https://github.org/simargl>
 *  License: GPL v3
 */

// g++ `fltk-config --use-images --cxxflags` main.cpp image_loader.cpp `fltk-config --use-images --ldflags` -o dlauncher; ./dlauncher

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Scroll.H>
#include "image_loader.hpp"
#include "main.hpp"

Fl_Window *window;
Fl_Button *button;
bool hasFocus = true;
Fl_Color bcolor = 0x1f1f1f00;
Fl_Color fcolor = 0x36363500;
std::string icons_dir;

void createConfigDirAndFile(const std::string& configFile) {
    std::string configDir = configFile.substr(0, configFile.find_last_of('/'));
    if (!std::filesystem::exists(configDir) && mkdir(configDir.c_str(), 0700) != 0) {
        std::cerr << "Failed to create directory: " << configDir << std::endl;
        return;
    }
    std::ofstream file(configFile);
    if (file.is_open()) file << "icons_dir = /usr/share/icons/hicolor/48x48/apps/" << std::endl;
    else std::cerr << "Failed to create config file: " << configFile << std::endl;
}

void readConfigFile() {
    const char* homeDir = getenv("HOME");
    if (homeDir != nullptr) {
        std::string configFile = std::string(homeDir) + "/.config/dlauncher/config";
        std::ifstream file(configFile);
        if (file.is_open()) {
            std::string line;
            while (std::getline(file, line)) {
                if (line.find("icons_dir") != std::string::npos) {
                    auto pos = line.find("=");
                    if (pos != std::string::npos) {
                        icons_dir = line.substr(pos + 1);
                        icons_dir.erase(0, icons_dir.find_first_not_of(" \t"));
                        icons_dir.erase(icons_dir.find_last_not_of(" \t") + 1);
                    }
                }
            }
            file.close();
        } else {
            createConfigDirAndFile(configFile);
            icons_dir = "/usr/share/icons/hicolor/48x48/apps/";
        }
    }
}

void parseDesktopFile(const std::filesystem::path& filePath, std::vector<DesktopFile>& desktopFiles) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filePath << std::endl;
        return;
    }
    DesktopFile desktopFile;
    std::string line;
    bool nameFound = false, execFound = false, iconFound = false;
    while (std::getline(file, line)) {
        if (line.find("NoDisplay=true") != std::string::npos) return;
        if (!nameFound && line.find("Name=") != std::string::npos) {
            desktopFile.name = line.substr(5);
            nameFound = true;
        } else if (!execFound && line.find("Exec=") == 0 && line.find("Exec=") != std::string::npos) {
            std::string execLine = line.substr(5);
            size_t pos = execLine.find_first_of("%");
            if (pos != std::string::npos) execLine.erase(pos - 1);
            desktopFile.exec = execLine + " &";
            execFound = true;
        } else if (!iconFound && line.find("Icon=") != std::string::npos) {
            desktopFile.icon = line.substr(5);
            iconFound = true;
        }
    }
    file.close();
    if (!desktopFile.icon.empty()) desktopFile.icon += ".png";
    if (!desktopFile.icon.empty()) {
        std::filesystem::path iconPath;
        for (const auto& entry : std::filesystem::recursive_directory_iterator(icons_dir))
            if (entry.is_regular_file() && entry.path().filename() == desktopFile.icon) {
                iconPath = entry.path();
                break;
            }
        if (iconPath == "") std::cout<<"not found icon: "<<icons_dir+desktopFile.icon.c_str()<<std::endl;
        if (!iconPath.empty()) desktopFile.icon = iconPath.string();
    }
    if (nameFound && execFound) desktopFiles.push_back(desktopFile);
}

std::vector<DesktopFile> parseDesktopFiles(const std::string& directory) {
    std::vector<DesktopFile> desktopFiles;
    for (const auto& entry : std::filesystem::directory_iterator(directory)) parseDesktopFile(entry.path(), desktopFiles);
    return desktopFiles;
}

class Fl_HoverButton : public Fl_Button {
public:
    Fl_HoverButton(int x, int y, int w, int h, const char *label = 0) : Fl_Button(x, y, w, h, label) {}
    int handle(int event) override {
        switch (event) {
            case FL_ENTER: color(fcolor); redraw(); return 1;
            case FL_LEAVE: color(bcolor); redraw(); return 1;
        }
        return Fl_Button::handle(event);
    }
};

void buttonCallback(Fl_Widget* widget, void* command) {
    std::cout<<"executing command: "<<static_cast<const char*>(command)<<std::endl;
    int result = std::system(static_cast<const char*>(command));
    if (result == -1) std::cerr << "Failed to execute command: " << command << std::endl;
    exit(0);
}

void check_if_focus_was_lost(void*) {
    hasFocus = Fl::focus();
    if (!hasFocus) exit(0);
    Fl::repeat_timeout(0.5, check_if_focus_was_lost, nullptr);
}

int main(int argc, char **argv) {
    readConfigFile();
    std::vector<DesktopFile> parsedFiles = parseDesktopFiles("/usr/share/applications");
    window = new Fl_Window(520, 500, "dlauncher");
    window->color(bcolor);
    window->clear_border();
    window->position(0, Fl::h() - window->h());
    Fl_Scroll scroll(0, 0, 515, 500);
    scroll.color(bcolor);
    scroll.scrollbar.color(bcolor, FL_LIGHT1);
    int y = 0, x = 0;
    for (const auto& file : parsedFiles) {
        button = new Fl_HoverButton(x, y, 120, 120, file.name.c_str());
        button->align(FL_ALIGN_CENTER | FL_ALIGN_INSIDE | FL_ALIGN_IMAGE_OVER_TEXT | FL_ALIGN_WRAP);
        button->box(FL_FLAT_BOX);
        Fl_Image* image = ImageLoader::loadImage(file.icon);
        button->image(image);
        button->color(bcolor);
        button->labelsize(16);
        button->labelcolor(FL_LIGHT2);
        button->selection_color(bcolor);
        button->clear_visible_focus();
        button->callback(buttonCallback, (void*)file.exec.c_str());
        if (x <= 240) x = x + 120;
        else { y = y + 120; x = 0; }
    }
    Fl::focus(window);
    Fl::visual(FL_DOUBLE | FL_INDEX);
    check_if_focus_was_lost(nullptr);
    window->end();
    window->show(argc, argv);
    return Fl::run();
}
