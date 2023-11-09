# ! WARNING !
# 
# This script is run using super-user access. Do not run this if you feel it has been maliciously
# modified.
#
# If you do not possess super-user access, you may be unable to use this program. 
# Contact your administrator for help and/or assistance.


echo "Building..."

# Generate config.h from config.def.h if one does not exist.
if [ ! -f "config.h" ]; then
	# Ensure config.def.h exists, otherwise fail.
	if [ ! -f "config.def.h" ]; then
		echo "config.def.h not found. Please create it. You can get the template from:"
		echo "https://github.com/TheEmeraldFalcon/zoomer"
		echo "Build Failed. No config.h or config.def.h."
		exit 1
	fi

	echo "Copying config.def.h to config.h. Please modify config.h to your liking."
	cp config.def.h config.h
	chmod +rwx config.h
fi

gcc main.c -lSDL2 -lX11 -lm -Wall -Wextra -Wpedantic -fanalyzer -O3
#clang main.c -lSDL2 -lX11 -lm -Wall -Wextra -Wpedantic -O3
cp a.out /usr/bin/zoomer
chmod +x /usr/bin/zoomer
echo "Finished."
