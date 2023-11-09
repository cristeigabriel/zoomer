# zoomer
A screenshot and zoom-in tool for Linux inspired by Tsoding's 'boomer'. Also allows dragging the capture if you have multiple monitors, as shown below:

# Showcase
![](static/showcase.gif)

# How to install
```
$: sudo ./build.sh
```
# How to use
Run `zoomer -h`

# Controls
|Key|Usage|
|:--|:--|
|g|Toggles grid view|
|Alt + Mouse wheel up/down|Increases/decreases grid alpha|
|Mouse wheel up/down|Scrolls up/down|
|Right click + drag left/right/up/down|Drags around focus area|
|WASD and Arrow Keys (modifiable)|Navigate the focus area|
|1-9|Swap focus area to n-th monitor|
|q|Quit Zoomer|

# Dependencies
- libsdl2
- libx11

# To-Do
- [ ] External config file format, so rebuild isn't necessary after modification.
- [ ] Start Zoomer showing the currently active monitor.
- [ ] Zoom where the mouse is, instead of the middle of the screen.

# Clarifications
This is a modified version of https://github.com/cristeigabriel/zoomer that I made for personal use.
Original Message: This is written by myself, and only the idea has been taken from Tsoding.
