#! /bin/bash

source ./install_helpers.sh "" || exit 1


# Name of package
PKG_NAME="libfreetype"

# Path to one file of installed package to test for existing installation
PKG_INSTALLED_FILE="$SWEET_LOCAL_SOFTWARE_DST_DIR/lib/libfreetype.so"

# URL to source code to fetch it
PKG_URL_SRC="freetype-2.8.tar.gz"

config_package $@ || exit 1

config_configure_make_default_install || exit 1

config_success || exit 1
