#ifndef MAIN_H
#define MAIN_H

#include <string>
#include <vector>

struct DesktopFile {
    std::string name, exec, icon;
};

void createConfigDirAndFile(const std::string& configFile);
void readConfigFile();
void parseDesktopFile(const std::filesystem::path& filePath, std::vector<DesktopFile>& desktopFiles);
std::vector<DesktopFile> parseDesktopFiles(const std::string& directory);
void buttonCallback(Fl_Widget* widget, void* command);
void check_if_focus_was_lost(void*);

#endif
