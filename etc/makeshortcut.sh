#!/bin/bash

MENU_DIRECTORY_PATH=${HOME}/.local/share/desktop-directories/tizen-sdk-menu.directory

SHORTCUT_FILE_PATH=
EXEC_FILE_PATH=
ICON_PATH=

NAME=
COMMENT=

parseParameter()
{
	while [ $# -gt 0 ]
	do
		case $1 in
			-f|--filepath)
				shift
				SHORTCUT_FILE_PATH=$1
				;;
			-e|--executefilepath)
				shift
				EXEC_FILE_PATH=$1
				;;
			-i|--iconpath)
				shift
				ICON_PATH=$1
				;;
			-n|--name)
				shift
				NAME=$1
				;;
			-c|--comment)
				shift
				COMMENT=$1
				;;
			*)
				exit 12
				;;
		esac
		shift
	done
}

##exist unconditionally
checkParameter()
{	
	if [ ! -e ${EXEC_FILE_PATH} ]; then
		exit 13
	fi
	
	if [ ! -e ${ICON_PATH} ]; then
		exit 13
	fi
}

makeShortcut()
{
	checkParameter
	
	UBUNTU_VER=`awk 'BEGIN {FS="="}; /DISTRIB_RELEASE.*/ {print $2}' /etc/lsb-release`

	case ${UBUNTU_VER} in
		11.04)
			categories="Tizen SDK;Development;"
			;;
		11.10)
			categories="Tizen SDK;Development;"
			;;
		*)
			categories="Tizen SDK"
			;;
	esac

	if [ -e ${SHORTCUT_FILE_PATH} ]; then
		rm -rf ${SHORTCUT_FILE_PATH}
	fi

	## Create .desktop file 
	mkdir -p ${HOME}/.local/share/applications
	cat >> ${SHORTCUT_FILE_PATH} << END
[Desktop Entry]
Encoding=UTF-8
Version=1.0
Type=Application
Terminal=false
Comment=${COMMENT} 
Name=${NAME}
Categories=$categories
StartupNotify=true
NoDisplay=false
Icon=${ICON_PATH}
Exec=${EXEC_FILE_PATH}
END

	if [ ! -e $MENU_DIRECTORY_PATH ]; then
		mkdir -p $MENU_DIRECTORY_PATH
	fi
	## Register .desktop file
	xdg-desktop-menu install ${MENU_DIRECTORY_PATH} ${SHORTCUT_FILE_PATH}
}

parseParameter "$@"
makeShortcut
