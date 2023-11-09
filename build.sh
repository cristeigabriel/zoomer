echo "Building..."
gcc main.c -lSDL2 -lX11 -lm -Wall -Wextra -Wpedantic -fanalyzer -O3
#clang main.c -lSDL2 -lX11 -lm -Wall -Wextra -Wpedantic -O3
cp a.out /usr/bin/zoomer
chmod +x /usr/bin/zoomer
echo "Finished."
