# dlauncher
Application launcher made in c++ and fltk with colors matching adwaita dark theme

icon search path by default is /usr/share/icons/hicolor/48x48/apps

buttons with hover effect

![Screenshot](https://raw.githubusercontent.com/simargl/dlauncher/main/screenshots/dlauncher.png)

## Installation

Install dlauncher with:

```bash
  apt install libfltk1.3-dev -y
  g++ `fltk-config --use-images --cxxflags` dlauncher.cxx `fltk-config --use-images --ldflags` -o dlauncher; ./dlauncher
```
