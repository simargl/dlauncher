# dlauncher
Application launcher made in c++ and fltk with colors matching adwaita dark theme

icon search path by default is /usr/share/icons/hicolor/48x48/apps/

configuration file is $HOME/.config/dlauncher/config

buttons with hover effect

![Screenshot](https://raw.githubusercontent.com/simargl/dlauncher/main/screenshots/dlauncher.png)

## Installation

Install dlauncher with:

```bash
  apt install libfltk1.3-dev -y
  make; ./dlauncher
```

## Note

For using in labwc add following to .config/labwc/rc.xml

```xml
  <windowRules>
      <windowRule title="*dlauncher*">
        <action name="MoveToEdge" direction="down" snapWindows="no" />
        <action name="MoveToEdge" direction="left" snapWindows="no" />
      </windowRule>
  </windowRules>
```
