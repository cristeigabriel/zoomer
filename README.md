# zoomer
A screenshot and zoom-in tool for Linux inspired by Tsoding's 'boomer'. Also allows dragging the capture if you have multiple monitors, as shown below:

# Showcase
![](static/showcase.gif)

# How to install
```
$: ./build.sh
$: sudo cp a.out /usr/bin/zoomer
$: sudo chmod +x /usr/bin/zoomer
```

# How to use
Run `zoomer -h`

# Controls
|Key|Usage|
|:--|:--|
|g|Toggles grid view|
|Alt + Mouse wheel up/down|Increases/decreases grid alpha|
|Mouse wheel up/down|Scrolls up/down|
|Click + drag left/right/up/down|Drags around focus area|

# Dependencies
- libsdl2
- libx11

# Clarifications
This is written by myself, and only the idea has been taken from Tsoding.
