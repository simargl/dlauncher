/*  Author: simargl <https://github.org/simargl>
 *  License: GPL v3
 */

// g++ `fltk-config --use-images --cxxflags` dlauncher.cxx `fltk-config --use-images --ldflags` -o dlauncher; ./dlauncher

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Scroll.H>
#include <FL/Fl_PNG_Image.H>
Fl_Window *window;
Fl_Button *button;
bool hasFocus = true;
Fl_Color bcolor = 0x2d2d2d00;
Fl_Color fcolor = 0x15539e00;
std::string icons_dir;

struct DesktopFile {
    std::string name;
    std::string exec;
    std::string icon;
};

void createConfigDirAndFile(const std::string& configFile) {
    std::string configDir = configFile.substr(0, configFile.find_last_of('/'));
    // Create .config/dlauncher directory if it doesn't exist
    if (!std::filesystem::exists(configDir)) {
        if (mkdir(configDir.c_str(), 0700) != 0) {
            std::cerr << "Failed to create directory: " << configDir << std::endl;
            return;
        }
    }
    std::ofstream file(configFile);
    if (file.is_open()) {
        // Write default configuration
        file << "icons_dir = /usr/share/icons/hicolor/48x48/apps/" << std::endl;
        file.close();
    } else {
        std::cerr << "Failed to create config file: " << configFile << std::endl;
    }
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
                        // Remove leading/trailing whitespaces
                        icons_dir.erase(0, icons_dir.find_first_not_of(" \t"));
                        icons_dir.erase(icons_dir.find_last_not_of(" \t") + 1);
                    }
                }
            }
            file.close();
        } else {
            // Config file doesn't exist, create it
            createConfigDirAndFile(configFile);
            // Use default icons_dir
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
    bool nameFound = false;
    bool execFound = false;
    bool iconFound = false;

    while (std::getline(file, line)) {
        if (line.find("NoDisplay=true") != std::string::npos) {
            return;
        }
        if (!nameFound && line.find("Name=") != std::string::npos) {
            desktopFile.name = line.substr(5); // Skip "Name="
            nameFound = true;
        } else if (!execFound && line.find("Exec=") != std::string::npos) {
            // Skip placeholders like %U, %f, etc., and append ' &' to the exec field
            std::string execLine = line.substr(5); // Skip "Exec="
            size_t pos = execLine.find_first_of("%");
            if (pos != std::string::npos)
                execLine.erase(pos - 1);
            desktopFile.exec = execLine + " &";
            execFound = true;
        } else if (!iconFound && line.find("Icon=") != std::string::npos) {
            desktopFile.icon = line.substr(5); // Skip "Icon="
            iconFound = true;
        }
    }
    file.close();

    // Append ".png" to icon field
    if (!desktopFile.icon.empty()) {
        desktopFile.icon += ".png";
    }

    // Search for matching icon file
    if (!desktopFile.icon.empty()) {
        std::filesystem::path iconPath;
        for (const auto& entry : std::filesystem::recursive_directory_iterator(icons_dir)) {
            if (entry.is_regular_file() && entry.path().filename() == desktopFile.icon) {
                iconPath = entry.path();
                break;
            }
        }
        if (iconPath == "") {
            std::cout<<"not found icon: "<<icons_dir+desktopFile.icon.c_str()<<std::endl;
            iconPath = "/usr/share/icons/hicolor/48x48/apps/application-x-generic.png";
        }
        if (!iconPath.empty()) {
            desktopFile.icon = iconPath.string();
        }
    }

    // Add the parsed desktop file if name and exec were found
    if (nameFound && execFound) {
        desktopFiles.push_back(desktopFile);
    }
}


std::vector<DesktopFile> parseDesktopFiles(const std::string& directory) {
    std::vector<DesktopFile> desktopFiles;
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        parseDesktopFile(entry.path(), desktopFiles);
    }
    return desktopFiles;
}

class Fl_HoverButton : public Fl_Button {
  public:
    Fl_HoverButton(int x, int y, int w, int h, const char *label = 0)
        : Fl_Button(x, y, w, h, label) {}
    int handle(int event) override {
        switch (event) {
        case FL_ENTER:
            color(fcolor);
            redraw();
            return 1;
        case FL_LEAVE:
            color(bcolor);
            redraw();
            return 1;
        }
        return Fl_Button::handle(event);
    }
};

void buttonCallback(Fl_Widget* widget, void* command) {
    std::cout<<"executing command: "<<static_cast<const char*>(command)<<std::endl;
    system(static_cast<const char*>(command));
    exit(0);
}

void check_if_focus_was_lost(void*) {
    hasFocus = Fl::focus(); // Check if any window has focus
    if (!hasFocus) {
        exit(0);
    }
    Fl::repeat_timeout(0.5, check_if_focus_was_lost, nullptr);
}

int main(int argc, char **argv) {
    readConfigFile();
    std::vector<DesktopFile> parsedFiles = parseDesktopFiles("/usr/share/applications");
    window = new Fl_Window(520, 500, "dlauncher");
    window->color(bcolor);
    window->clear_border();
    Fl_Scroll scroll(0, 0, 520, 500);
    scroll.color(bcolor);
    int y = 0;
    int x = 0;
    for (const auto& file : parsedFiles) {
        button = new Fl_HoverButton(x, y, 120, 120, file.name.c_str());
        button->align(FL_ALIGN_CENTER | FL_ALIGN_INSIDE | FL_ALIGN_IMAGE_OVER_TEXT | FL_ALIGN_WRAP);
        button->box(FL_FLAT_BOX);
        Fl_PNG_Image *pngImage = new Fl_PNG_Image(file.icon.c_str());
        button->image(pngImage->copy(48, 48));
        button->color(bcolor);
        button->labelsize(16);
        button->labelcolor(FL_LIGHT2);
        button->selection_color(bcolor);
        button->clear_visible_focus();
        button->callback(buttonCallback, (void*)file.exec.c_str());
        if (x <= 240) {
            x = x + 120;
        } else {
            y = y + 120;
            x = 0;
        }
    }
    Fl::focus(window);
    Fl::visual(FL_DOUBLE | FL_INDEX);
    check_if_focus_was_lost(nullptr);
    window->end();
    window->show(argc, argv);
    /*for (const auto& file : parsedFiles) {
        std::cout << "Name: " << file.name << std::endl;
        std::cout << "Exec: " << file.exec << std::endl;
        std::cout << "Icon: " << file.icon << std::endl;
        std::cout << std::endl;
    }*/
    return Fl::run();
}
