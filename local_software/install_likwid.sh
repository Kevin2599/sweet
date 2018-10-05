#! /bin/bash

source ./install_helpers.sh "" || exit 1


# Name of package
PKG_NAME="likwid"

# Path to one file of installed package to test for existing installation
PKG_INSTALLED_FILE="$SWEET_LOCAL_SOFTWARE_DST_DIR/bin/likwid-topology"

# URL to source code to fetch it
PKG_URL_SRC="likwid-4.3.2.tar.gz"

config_package $@ || exit 1

M_DST="${SWEET_LOCAL_SOFTWARE_DST_DIR}"
M_DST=${M_DST//\//\\/}

sed -i "s/^PREFIX =.*/PREFIX = "${M_DST}"/" config.mk

#sed -i "s/INSTALL_CHOWN = -g root -o root/INSTALL_CHOWN = /" config.mk

sed -i "s/^ACCESSMODE = /&direct#/" config.mk

# Don't build daemon
sed -i "s/^BUILDDAEMON = /&false#/" config.mk

# Don't build setFreq
sed -i "s/^BUILDFREQ = /&false#/" config.mk

config_make_clean || exit 1

config_make_default_install || exit 1

config_success || exit 1
