#! /bin/bash

source ./install_helpers.sh "" || exit 1

# Name of package
PKG_NAME="cmake"

# Path to one file of installed package to test for existing installation
PKG_INSTALLED_FILE="$SWEET_LOCAL_SOFTWARE_DST_DIR/bin/make"

# URL to source code to fetch it
PKG_URL_SRC="cmake-3.12.3.tar.gz"

if [ "`uname -s`" != "Linux" ] && [ "`uname -s`" != "Darwin" ]; then
	echo "This script only supports make on Linux systems"
	exit 1
fi

config_package $@

config_configure_make_default_install

config_make_default_install

config_success
