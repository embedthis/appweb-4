#   
#   ./buildConfig.make -- Build configuration file.
#   
#   WARNING: DO NOT EDIT. This file is generated by configure.
#   
#   Use ./configure --help for available options.
#   
################################################################################
#
#   Key Product Settigns
#
BLD_APPWEB_PRODUCT=1
BLD_PRODUCT=appweb
BLD_NAME=Embedthis Appweb
BLD_VERSION=3.4.5
BLD_NUMBER=0
BLD_NUMBER_ONLY=0
BLD_MAJOR_VERSION=3
BLD_MINOR_VERSION=4
BLD_PATCH_VERSION=5
BLD_VNUM=3004005
BLD_TYPE=DEBUG
BLD_TUNE=MPR_TUNE_SIZE
BLD_DEFAULTS=standard
BLD_COMPONENTS= ar cc make nm strip ranlib doxygen regexp mpr matrixssl openssl ssl php sqlite ejs appweb
BLD_EMBEDTHIS=0

#
#   Other Product Settings
#
BLD_COMPANY=Embedthis
BLD_DEBUG=1
BLD_DIRS=build obj lib modules bin src test all doc samples projects package releases
BLD_CLEAN_INSTALL=0
BLD_LICENSE=gpl
BLD_COMMERCIAL=0

#
#   Host and Build System Settings.
#
BLD_HOST_SYSTEM=x86_64-apple-macosx
BLD_BUILD_SYSTEM=x86_64-apple-macosx
BLD_CROSS=0

#
#   Host System Settings 
#
BLD_HOST_OS=MACOSX
BLD_HOST_CPU_ARCH=MPR_CPU_IX64
BLD_HOST_CPU=x86_64
BLD_HOST_CPU_UPPER=X86_64
BLD_HOST_CPU_MODEL=
BLD_HOST_DIST=Apple
BLD_HOST_DIST_VER=10.13.6
BLD_HOST_UNIX=1
BLD_HOST_WIN=0

#
#   Build System Settings for Build Tools
#
BLD_BUILD_OS=MACOSX
BLD_BUILD_CPU_ARCH=MPR_CPU_IX64
BLD_BUILD_CPU=x86_64
BLD_BUILD_CPU_UPPER=X86_64
BLD_BUILD_CPU_MODEL=
BLD_BUILD_UNIX=1
BLD_BUILD_WIN=0

#
#   System and Installation Directories
#
BLD_ROOT_PREFIX=/
BLD_PREFIX=/usr
BLD_CFG_PREFIX=/etc/appweb
BLD_BIN_PREFIX=/usr/lib/appweb/bin
BLD_DOC_PREFIX=/usr/lib/appweb/doc
BLD_INC_PREFIX=/usr/lib/appweb/inc
BLD_JEM_PREFIX=/usr/lib/appweb/jems
BLD_LIB_PREFIX=/usr/lib/appweb/lib
BLD_LOG_PREFIX=/var/log/appweb
BLD_MAN_PREFIX=/usr/lib/appweb/man
BLD_MOD_PREFIX=/usr/lib/appweb/modules
BLD_PRD_PREFIX=/usr/lib/appweb
BLD_SAM_PREFIX=/usr/lib/appweb/samples
BLD_SRC_PREFIX=/usr/src/appweb-3.4.5
BLD_VER_PREFIX=/usr/lib/appweb
BLD_WEB_PREFIX=/var/www/appweb-default

#
#   Absolute native library and module directories
#
BLD_BUILD_LIB_DIR=/Users/mob/dev/appweb-3/lib
BLD_BUILD_MOD_DIR=/Users/mob/dev/appweb-3/modules

#
#   Standard Feature Selection
#
BLD_FEATURE_ASSERT=1
BLD_FEATURE_CMD=1
BLD_FEATURE_COMPLETE_NATIVE=1
BLD_FEATURE_COMPLETE_CROSS=0
BLD_FEATURE_DOC=1
BLD_FEATURE_DEVICE=PocketPC2003
BLD_FEATURE_FLOATING_POINT=1
BLD_FEATURE_LEGACY_API=0
BLD_FEATURE_MULTITHREAD=1
BLD_FEATURE_NUM_TYPE=double
BLD_FEATURE_NUM_TYPE_STRING=double
BLD_FEATURE_SAMPLES=1
BLD_FEATURE_TEST=1
BLD_FEATURE_STATIC=0

#
#   Appweb Feature Selection
#
BLD_APPWEB_PRODUCT=1
BLD_FEATURE_ACCESS_LOG=1
BLD_FEATURE_ANGEL=1
BLD_FEATURE_AUTH_DIGEST=1
BLD_FEATURE_AUTH=1
BLD_FEATURE_AUTH_FILE=1
BLD_FEATURE_AUTH_PAM=0
BLD_FEATURE_CGI=1
BLD_FEATURE_CHUNK=1
BLD_FEATURE_CONFIG_FILE=0
BLD_FEATURE_DIR=1
BLD_FEATURE_EJS_ALL_IN_ONE=1
BLD_FEATURE_EJS_AUTO_COMPILE=1
BLD_FEATURE_EJS_CROSS_COMPILER=1
BLD_FEATURE_EJS_DB=1
BLD_FEATURE_EJS_DOC=0
BLD_FEATURE_EJS_E4X=1
BLD_FEATURE_EJS_LANG=EJS_LANG_FIXED
BLD_FEATURE_EJS_WEB=1
BLD_FEATURE_EGI=1
BLD_FEATURE_CONFIG=template/standard
BLD_FEATURE_CONFIG_PARSE=1
BLD_FEATURE_FILE=1
BLD_FEATURE_HTTP=1
BLD_FEATURE_HTTP_CLIENT=1
BLD_FEATURE_LOG=1
BLD_FEATURE_NET=1
BLD_FEATURE_NUM_TYPE_DOUBLE=1
BLD_FEATURE_RANGE=1
BLD_FEATURE_RUN_AS_SERVICE=1
BLD_FEATURE_SEND=1
BLD_FEATURE_SERVER_ROOT=0
BLD_FEATURE_UPLOAD=1
BLD_FEATURE_XML=1

BLD_HTTP_PORT=7777
BLD_SSL_PORT=4443


#
#   File extensions 
#
BLD_BUILD_ARCH=.a
BLD_BUILD_EXE=
BLD_BUILD_OBJ=.o
BLD_BUILD_PIOBJ=.o
BLD_BUILD_CLASS=.class
BLD_BUILD_SHLIB=.dylib
BLD_BUILD_SHOBJ=.dylib

    #
	#	Configuration for Native Compilation on the Build System
	#

	BUILD_NATIVE_OR_COMPLETE_CROSS=1
	BUILD_CROSS_OR_COMPLETE_NATIVE=1

	#
	#   O/S and CPU settings
	#
	MACOSX=1
	BLD_OS=MACOSX
	BLD_CPU_ARCH=MPR_CPU_IX64
	BLD_CPU=x86_64
	BLD_CPU_UPPER=X86_64
	BLD_CPU_MODEL=
	BLD_DIST=Apple
	BLD_DIST_VER=10.13.6
	BLD_UNIX_LIKE=1
	BLD_WIN_LIKE=0

	#
	#   Compiler and linker flags
	#
	BLD_CFLAGS=
	BLD_DFLAGS=
	BLD_IFLAGS=
	BLD_LDFLAGS=
	BLD_JFLAGS=
	BLD_CPPFLAGS=

	#
	#   File extensions
	#
	BLD_ARCH=.a
	BLD_EXE=
	BLD_CLASS=.class
	BLD_SHLIB=.dylib
	BLD_SHOBJ=.dylib
	BLD_LIB=.dylib
	BLD_OBJ=.o
	BLD_PIOBJ=.o

	#
	#   Output directories 
	#
    BLD_BIN_NAME=bin
    BLD_LIB_NAME=lib
    BLD_OBJ_NAME=obj
    BLD_MOD_NAME=modules
    BLD_INC_NAME=src/include
	BLD_TOOLS_DIR=${BLD_TOP}/build/bin
	BLD_BIN_DIR=${BLD_TOP}/bin
	BLD_OBJ_DIR=${BLD_TOP}/obj
	BLD_MOD_DIR=${BLD_TOP}/modules
	BLD_JEM_DIR=${BLD_TOP}/jlocal
	BLD_LIB_DIR=${BLD_TOP}/lib
	BLD_INC_DIR=${BLD_TOP}/src/include
	BLD_ABS_BIN_DIR=/Users/mob/dev/appweb-3/bin
	BLD_ABS_LIB_DIR=/Users/mob/dev/appweb-3/lib
	BLD_ABS_OBJ_DIR=/Users/mob/dev/appweb-3/obj
	BLD_ABS_MOD_DIR=/Users/mob/dev/appweb-3/modules
	BLD_ABS_JEM_DIR=/Users/mob/dev/appweb-3/jlocal
	BLD_ABS_INC_DIR=/Users/mob/dev/appweb-3/src/include

	#
	#   Native Compilation Features
	#
    BLD_FEATURE_ROMFS=0

	#
	#   Setup environment variables
	#
	export PATH:=/Users/mob/dev/appweb-3/bin:/Users/mob/dev/appweb-3/modules:/Users/mob/dev/appweb-3/lib:/Users/mob/dev/appweb-3/lib:/Users/mob/dev/appweb-3/build/bin::$(PATH)

	#
	#   AR
	#
	BLD_FEATURE_AR=1
	BLD_AR=/usr/bin/ar
	BLD_AR_BUILTIN=0
	BLD_AR_WITH=1

	#
	#   CC
	#
	BLD_FEATURE_CC=1
	BLD_CC=/usr/bin/cc
	BLD_CC_BUILTIN=0
	BLD_CC_WITH=1
	BLD_CC_CYGWIN=0
	BLD_CC_DIAB=0
	BLD_CC_DOUBLE_BRACES=1
	BLD_CC_DYN_LOAD=1
	BLD_CC_MTUNE=1
	BLD_CC_MMU=1
	BLD_CC_WARN_UNUSED=1
	BLD_CC_EDITLINE=1
	BLD_CC_PAM=1
	BLD_CC_STACK_PROTECTOR=1

	#
	#   MAKE
	#
	BLD_FEATURE_MAKE=1
	BLD_MAKE=/usr/bin/make
	BLD_MAKE_BUILTIN=0
	BLD_MAKE_WITH=1

	#
	#   NM
	#
	BLD_FEATURE_NM=1
	BLD_NM=/usr/bin/nm
	BLD_NM_BUILTIN=0
	BLD_NM_WITH=1

	#
	#   STRIP
	#
	BLD_FEATURE_STRIP=1
	BLD_STRIP=/usr/bin/strip
	BLD_STRIP_BUILTIN=0
	BLD_STRIP_WITH=1

	#
	#   RANLIB
	#
	BLD_FEATURE_RANLIB=1
	BLD_RANLIB=/usr/bin/ranlib
	BLD_RANLIB_BUILTIN=0
	BLD_RANLIB_WITH=1

	#
	#   DOXYGEN
	#
	BLD_FEATURE_DOXYGEN=1
	BLD_DOXYGEN=/usr/local/bin/doxygen
	BLD_DOXYGEN_BUILTIN=0
	BLD_DOXYGEN_WITH=1

	#
	#   REGEXP
	#
	BLD_FEATURE_REGEXP=1
	BLD_REGEXP=src/mpr/mprPcre.c
	BLD_REGEXP_BUILTIN=1
	BLD_REGEXP_LIBS=pcre
	BLD_REGEXP_WITH=1

	#
	#   MPR
	#
	BLD_FEATURE_MPR=1
	BLD_MPR=src/mpr
	BLD_MPR_BUILTIN=1
	BLD_MPR_LIBS=mpr
	BLD_MPR_WITH=1

	#
	#   MATRIXSSL
	#
	BLD_FEATURE_MATRIXSSL=0

	#
	#   OPENSSL
	#
	BLD_FEATURE_OPENSSL=1
	BLD_OPENSSL=/Users/mob/.paks/openssl/openssl/1.1.0h
	BLD_OPENSSL_BUILTIN=0
	BLD_OPENSSL_IFLAGS=-I/Users/mob/.paks/openssl/openssl/1.1.0h/include
	BLD_OPENSSL_IMPORTS= ${BLD_TOP}/../../.paks/openssl/openssl/1.1.0h/libssl.1.1.dylib ${BLD_TOP}/../../.paks/openssl/openssl/1.1.0h/libssl.a ${BLD_TOP}/../../.paks/openssl/openssl/1.1.0h/libssl.dylib ${BLD_TOP}/../../.paks/openssl/openssl/1.1.0h/libssl.pc ${BLD_TOP}/../../.paks/openssl/openssl/1.1.0h/libcrypto.1.1.dylib ${BLD_TOP}/../../.paks/openssl/openssl/1.1.0h/libcrypto.a ${BLD_TOP}/../../.paks/openssl/openssl/1.1.0h/libcrypto.dylib ${BLD_TOP}/../../.paks/openssl/openssl/1.1.0h/libcrypto.pc
	BLD_OPENSSL_LIBS=ssl crypto
	BLD_OPENSSL_WITH=1

	#
	#   SSL
	#
	BLD_FEATURE_SSL=1
	BLD_SSL=src/ssl
	BLD_SSL_BUILTIN=1
	BLD_SSL_DEPENDENCIES= openssl mpr
	BLD_SSL_LIBS=mprssl ssl crypto mpr
	BLD_SSL_WITH=1
	BLD_SSL_WITHLIBS=ssl crypto mpr

	#
	#   PHP
	#
	BLD_FEATURE_PHP=0

	#
	#   SQLITE
	#
	BLD_FEATURE_SQLITE=1
	BLD_SQLITE=/Users/mob/dev/appweb-3/src/ejs
	BLD_SQLITE_BUILTIN=1
	BLD_SQLITE_DEPENDENCIES=mpr
	BLD_SQLITE_LIBS=sqlite3 mpr
	BLD_SQLITE_WITH=1
	BLD_SQLITE_WITHLIBS=mpr

	#
	#   EJS
	#
	BLD_FEATURE_EJS=1
	BLD_EJS=/usr/local/bin/ejs
	BLD_EJS_BUILTIN=0
	BLD_EJS_DEPENDENCIES=mpr
	BLD_EJS_LIBS=ajs pcre mprssl ssl crypto mpr sqlite3
	BLD_EJS_OPTIONAL_DEPENDENCIES=ssl sqlite
	BLD_EJS_WITH=1
	BLD_EJS_WITHLIBS=pcre mprssl ssl crypto mpr sqlite3

	#
	#   APPWEB
	#
	BLD_FEATURE_APPWEB=1
	BLD_APPWEB=/Users/mob/dev/appweb-3/src/server
	BLD_APPWEB_BUILTIN=1
	BLD_APPWEB_DEPENDENCIES=mpr
	BLD_APPWEB_IFLAGS=-I/Users/mob/dev/appweb-3/src/server/include
	BLD_APPWEB_LIBS=appweb mprssl ssl crypto mpr
	BLD_APPWEB_OPTIONAL_DEPENDENCIES=ssl
	BLD_APPWEB_WITH=1
	BLD_APPWEB_WITHLIBS=mprssl ssl crypto mpr


EXPORT_OBJECTS ?= 1
ifeq ($(EXPORT_OBJECTS),0)
  BLD_OBJ_DIR := .
endif

