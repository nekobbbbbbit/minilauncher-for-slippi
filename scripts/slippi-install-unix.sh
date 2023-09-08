#!/bin/sh
CMAKE_FLAGS="-DLINUX_LOCAL_DEV=true"
PLAYBACK_CODES_PATH="./Data/PlaybackGeckoCodes"
DATA_SYS_PATH="./Data/Sys"
PWD_=$(pwd)
THREADS=$(($(nproc)/2))
_ROOT="$0"
_PATH="$1"
_IS_INSTALL="$2"
_INSTALL_LOCATION="$3"
SU="sudo"

if command -v doas > /dev/null
then
	SU="doas"
fi

if [ "$_IS_INSTALL" = "install" ]
then
	CMAKE_FLAGS="$CMAKE_FLAGS -DINSTALLMODE=true"
	#echo $CMAKE_FLAGS
	#exit
fi

if [ ! "$_PATH" ]
then
	echo "Please specify the path to Ishiiruka, or pass \"git\"."
	exit 1
fi

if [ ! "$_INSTALL_LOCATION" ]
then
	_INSTALL_LOCATION="/usr/local/bin"
fi

if [ "$_PATH" == "git" ]
then
	echo "== Cloning Slippi Ishiiruka..."
	# Point to my own fork for now until I provide .patch files
	git clone --recurse-submodules -j$THREADS "https://github.com/nekobbbbbbit/Ishiiruka.git" work > /dev/null || exit
	cd work
else
	cd "$_PATH"
fi

echo "NOTE: Build logs are stored in build.log"

build_slippi()
{
	BUILD_DIR="$1"
	CMAKE_F="$2"
	NEW_BIN_NAME="$3"
	GLOBAL_CFG_DIR="$4"
	mkdir -p "$BUILD_DIR"; cd "$BUILD_DIR"
	
	MSG="Error occured compiling! Check build.log"
	DATE=$(date "+%Y-%m-%d %H:%M:%S")
	echo ">>>>>> BEGIN BUILD LOG ON $DATE" >> "$PWD_/build.log"
	echo ">>> cmake $CMAKE_F .." >> "$PWD_/build.log"
	cmake $CMAKE_F .. >> "$PWD_/build.log" 2>&1 || exit 1
	echo "== Building $NEW_BIN_NAME"
	echo ">>> make -j$THREADS" >> "$PWD_/build.log"
	make -j$THREADS >> "$PWD_/build.log" 2>&1 || exit 1
	echo ">>>>>> END BUILD LOG ON $DATE" >> "$PWD_/build.log"
	cd ..
	# Copy the Sys folder in
	if [ "$_IS_INSTALL" = "install" ]
	then
		rm -rf "$HOME/.config/$GLOBAL_CFG_DIR"
		echo "== Installing config to \"~/.config/$GLOBAL_CFG_DIR\"..."
		cp -r ${DATA_SYS_PATH} "$HOME/.config/$GLOBAL_CFG_DIR"
		# ???
		cp -r ${DATA_SYS_PATH} "$HOME/.config/$GLOBAL_CFG_DIR/Sys"
	else
		rm -rf $BUILD_DIR/Binaries/Sys
		echo "== Copying config..."
		cp -r ${DATA_SYS_PATH} $BUILD_DIR/Binaries
	fi
	touch $BUILD_DIR/Binaries/portable.txt	
	
	if [ "$NEW_BIN_NAME" ]
	then
		mv $BUILD_DIR/Binaries/dolphin-emu "$BUILD_DIR/Binaries/$NEW_BIN_NAME"
	fi
}

#####################################################
# First build the netplay version
build_slippi "build_netplay" "$CMAKE_FLAGS" "slippi-netplay-dolphin" "SlippiOnline"
#####################################################
build_slippi "build_playback" "$CMAKE_FLAGS -DIS_PLAYBACK=true" "slippi-playback-dolphin" "SlippiPlayback"
if [ "$_IS_INSTALL" = "install" ]
then
	rm -rf "$HOME/.config/SlippiPlayback/Sys/GameSettings" # Delete netplay codes
	cp -r "${PLAYBACK_CODES_PATH}/." "$HOME/.config/SlippiPlayback/Sys/GameSettings"
else
	rm -rf "build_playback/Binaries/Sys/GameSettings" # Delete netplay codes
	cp -r "${PLAYBACK_CODES_PATH}/." "build_playback/Binaries/Sys/GameSettings"
fi
#####################################################

if [ "$_IS_INSTALL" = "install" ]
then
	echo "== Installing to \"$_INSTALL_LOCATION\"..."
	${SU} cp "build_netplay/Binaries/slippi-netplay-dolphin" "$_INSTALL_LOCATION"
	${SU} cp "build_playback/Binaries/slippi-playback-dolphin" "$_INSTALL_LOCATION"
else
	echo "Hint: run \"$_ROOT $_PATH install\" to install."
fi

cd "$PWD_"
exit 0
