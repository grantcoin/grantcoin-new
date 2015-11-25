TEMPLATE = app
#TARGET = codecoin-qt
#macx:TARGET = "codecoin-Qt"
VERSION = 0.8.9
INCLUDEPATH += src src/json src/qt
QT += core gui network
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
DEFINES += QT_GUI BOOST_THREAD_USE_LIB BOOST_SPIRIT_THREADSAFE
CONFIG += no_include_pwd
CONFIG += thread

#may need below. cause arg
#
android:DEFINES += BOOST_NO_SCOPED_ENUMS BOOST_NO_CXX11_SCOPED_ENUMS __WORDSIZE=32

isEmpty(COIN_BRAND){
	COIN_BRAND=grantcoin
}
# use: qmake "COIN_BRAND=grantstake" for other than grantcoin
contains(COIN_BRAND, grantcoin) {
    message(Building for Grantcoin)
    DEFINES += BRAND_grantcoin
    TARGET = grantcoin
} else {
contains(COIN_BRAND, grantstake) {
    message(Building for Grantstake)
    DEFINES += BRAND_grantstake
    STAKE = 1
    TARGET = grantstake
}}

#Set up a symlink so QT designer doesn't get confused when editing UI files
!windows:system("ln -sf $$TARGET/codecoin.qrc src/qt/res/codecoin.qrc")
#same thing for icons
!windows:system("ln -sf $$TARGET/icons src/qt/res/icons")

# for boost 1.37, add -mt to the boost libraries
# use: qmake BOOST_LIB_SUFFIX=-mt
# for boost thread win32 with _win32 sufix
# use: BOOST_THREAD_LIB_SUFFIX=_win32-...
# or when linking against a specific BerkelyDB version: BDB_LIB_SUFFIX=-4.8

# Dependency library locations can be customized with:
#    BOOST_INCLUDE_PATH, BOOST_LIB_PATH, BDB_INCLUDE_PATH,
#    BDB_LIB_PATH, OPENSSL_INCLUDE_PATH and OPENSSL_LIB_PATH respectively

# Windows compilation, refer to the link below for detailed instructions
# https://bitcointalk.org/index.php?topic=149479.0
win32 {
    BOOST_LIB_SUFFIX=-mgw49-mt-s-1_55
    BOOST_INCLUDE_PATH=C:/deps/boost1.55-1.55.0+dfsg.orig
    BOOST_LIB_PATH=C:/deps/boost1.55-1.55.0+dfsg.orig/stage/lib
    BDB_INCLUDE_PATH=C:/deps/db5.3-5.3.28/build_unix
    BDB_LIB_PATH=C:/deps/db5.3-5.3.28/build_unix
    OPENSSL_INCLUDE_PATH=C:/deps/openssl-1.0.2d/include
    OPENSSL_LIB_PATH=C:/deps/openssl-1.0.2d
    MINIUPNPC_INCLUDE_PATH=C:/deps/
    MINIUPNPC_LIB_PATH=C:/deps/miniupnpc
}

OBJECTS_DIR = build
MOC_DIR = build
UI_DIR = build

# use: qmake "RELEASE=1"
contains(RELEASE, 1) {
    # Mac: compile for maximum compatibility (10.5, 32-bit)
    macx:QMAKE_CXXFLAGS += -mmacosx-version-min=10.5 -arch i386 -isysroot /Developer/SDKs/MacOSX10.5.sdk
    macx:QMAKE_CFLAGS += -mmacosx-version-min=10.5 -arch i386 -isysroot /Developer/SDKs/MacOSX10.5.sdk
    macx:QMAKE_OBJECTIVE_CFLAGS += -mmacosx-version-min=10.5 -arch i386 -isysroot /Developer/SDKs/MacOSX10.5.sdk
	macx:QMAKE_LFLAGS += -mmacosx-version-min=10.5 -arch i386 -isysroot /Developer/SDKs/MacOSX10.5.sdk

    !win32:!macx {
        # Linux: static link and extra security (see: https://wiki.debian.org/Hardening)
        LIBS += -Wl,-Bstatic -Wl,-z,relro -Wl,-z,now
    }
}

!win32 {
    # for extra security against potential buffer overflows: enable GCCs Stack Smashing Protection
    QMAKE_CXXFLAGS *= -fstack-protector-all
    QMAKE_LFLAGS *= -fstack-protector-all
    # Exclude on Windows cross compile with MinGW 4.2.x, as it will result in a non-working executable!
    # This can be enabled for Windows, when we switch to MinGW >= 4.4.x.
}
# for extra security (see: https://wiki.debian.org/Hardening): this flag is GCC compiler-specific
QMAKE_CXXFLAGS *= -D_FORTIFY_SOURCE=2
# for extra security on Windows: enable ASLR and DEP via GCC linker flags
win32:QMAKE_LFLAGS *= -Wl,--dynamicbase -Wl,--nxcompat
# on Windows: enable GCC large address aware linker flag
win32:QMAKE_LFLAGS *= -Wl,--large-address-aware
# i686-w64-mingw32
win32:QMAKE_LFLAGS *= -static-libgcc -static-libstdc++

# use: qmake "USE_QRCODE=1"
# libqrencode (http://fukuchi.org/works/qrencode/index.en.html) must be installed for support
contains(USE_QRCODE, 1) {
    message(Building with QRCode support)
    DEFINES += USE_QRCODE
    LIBS += -lqrencode
}

# use: qmake "USE_UPNP=1" ( enabled by default; default)
#  or: qmake "USE_UPNP=0" (disabled by default)
#  or: qmake "USE_UPNP=-" (not supported)
# miniupnpc (http://miniupnp.free.fr/files/) must be installed for support
macx || windows {
message(FIXME WARNING upnp support disabled for now!!!)
USE_UPNP=-
}

android || ios: USE_UPNP=-

contains(USE_UPNP, -) {
    message(Building without UPNP support)
} else {
    message(Building with UPNP support)
    count(USE_UPNP, 0) {
        USE_UPNP=1
    }
    DEFINES += USE_UPNP=$$USE_UPNP STATICLIB
    INCLUDEPATH += $$MINIUPNPC_INCLUDE_PATH
    LIBS += $$join(MINIUPNPC_LIB_PATH,,-L,) -lminiupnpc
    win32:LIBS += -liphlpapi
}

# use: qmake "USE_DBUS=1"
contains(USE_DBUS, 1) {
    message(Building with DBUS (Freedesktop notifications) support)
    DEFINES += USE_DBUS
    QT += dbus
}

# use: qmake "USE_IPV6=1" ( enabled by default; default)
#  or: qmake "USE_IPV6=0" (disabled by default)
#  or: qmake "USE_IPV6=-" (not supported)
contains(USE_IPV6, -) {
    message(Building without IPv6 support)
} else {
    count(USE_IPV6, 0) {
        USE_IPV6=1
    }
    DEFINES += USE_IPV6=$$USE_IPV6
}

#leveldb hacks.. Either use CONFIG for qt5, or c++11 flags for QT4
#Don't do this for mac to avoid libc++ compatibility quagmire for now
#macx:QMAKE_LFLAGS += -stdlib=libc++
#macx:QMAKE_CXXFLAGS += -stdlib=libc++
!macx{
greaterThan(QT_MAJOR_VERSION, 4) {
CONFIG += c++11
} else {
QMAKE_CXXFLAGS += -std=c++0x
}}

#Try to match platform leveldb build defines
android:DEFINES += LEVELDB_PLATFORM_POSIX OS_ANDROID
#unix:DEFINES += OS_LINUX LEVELDB_PLATFORM_POSIX NDEBUG
#unix:DEFINES += OS_LINUX LEVELDB_PLATFORM_POSIX LEVELDB_ATOMIC_PRESENT NDEBUG
# no scoped enums again..
unix{
DEFINES += LEVELDB_PLATFORM_POSIX BOOST_NO_SCOPED_ENUMS
macx{
DEFINES += OS_MACOSX
} else {
DEFINES += OS_LINUX LEVELDB_ATOMIC_PRESENT BOOST_NO_CXX11_SCOPED_ENUMS
}}
windows:DEFINES += LEVELDB_PLATFORM_WINDOWS OS_WINDOWS BOOST_NO_SCOPED_ENUMS BOOST_NO_CXX11_SCOPED_ENUMS

INCLUDEPATH += src/leveldb/include src/leveldb/helpers src/leveldb

contains(BITCOIN_NEED_QT_PLUGINS, 1) {
    DEFINES += BITCOIN_NEED_QT_PLUGINS
    QTPLUGIN += qcncodecs qjpcodecs qtwcodecs qkrcodecs qtaccessiblewidgets
}

# regenerate src/build.h
!win32|contains(USE_BUILD_INFO, 1) {
    genbuild.depends = FORCE
    genbuild.commands = cd $$PWD; /bin/sh share/genbuild.sh src/build.h
    genbuild.target = src/build.h
    PRE_TARGETDEPS += src/build.h
    QMAKE_EXTRA_TARGETS += genbuild
    DEFINES += HAVE_BUILD_INFO
}

QMAKE_CXXFLAGS_WARN_ON = -fdiagnostics-show-option -Wall -Wextra -Wformat -Wformat-security -Wno-unused-parameter -Wstack-protector

# Input
DEPENDPATH += src src/json src/qt
HEADERS += src/qt/codecoingui.h \
    src/qt/transactiontablemodel.h \
    src/qt/addresstablemodel.h \
    src/qt/optionsdialog.h \
    src/qt/sendcoinsdialog.h \
    src/qt/coincontroldialog.h \
    src/qt/coincontroltreewidget.h \
    src/qt/addressbookpage.h \
    src/qt/signverifymessagedialog.h \
    src/qt/aboutdialog.h \
    src/qt/editaddressdialog.h \
    src/qt/bitcoinaddressvalidator.h \
    src/alert.h \
    src/addrman.h \
    src/base58.h \
    src/bignum.h \
    src/checkpoints.h \
    src/codecoin.h \
    src/coincontrol.h \
    src/compat.h \
    src/sync.h \
    src/util.h \
    src/hash.h \
    src/uintBIG.h \
    src/kernel.h \
    src/pbkdf2.h \
    src/serialize.h \
    src/main.h \
    src/undo.h \
    src/net.h \
    src/key.h \
    src/db.h \
    src/walletdb.h \
    src/script.h \
    src/init.h \
    src/bloom.h \
    src/mruset.h \
    src/checkqueue.h \
    src/json/json_spirit_writer_template.h \
    src/json/json_spirit_writer.h \
    src/json/json_spirit_value.h \
    src/json/json_spirit_utils.h \
    src/json/json_spirit_stream_reader.h \
    src/json/json_spirit_reader_template.h \
    src/json/json_spirit_reader.h \
    src/json/json_spirit_error_position.h \
    src/json/json_spirit.h \
    src/qt/clientmodel.h \
    src/qt/guiutil.h \
    src/qt/transactionrecord.h \
    src/qt/guiconstants.h \
    src/qt/optionsmodel.h \
    src/qt/monitoreddatamapper.h \
    src/qt/transactiondesc.h \
    src/qt/transactiondescdialog.h \
    src/qt/bitcoinamountfield.h \
    src/wallet.h \
    src/keystore.h \
    src/qt/transactionfilterproxy.h \
    src/qt/transactionview.h \
    src/qt/walletmodel.h \
    src/codecoinrpc.h \
    src/qt/overviewpage.h \
    src/qt/csvmodelwriter.h \
    src/crypter.h \
    src/qt/sendcoinsentry.h \
    src/qt/qvalidatedlineedit.h \
    src/qt/codecoinunits.h \
    src/qt/qvaluecombobox.h \
    src/qt/askpassphrasedialog.h \
    src/protocol.h \
    src/qt/notificator.h \
    src/qt/paymentserver.h \
    src/allocators.h \
    src/ui_interface.h \
    src/qt/rpcconsole.h \
    src/qt/virtualkeyboard.h \
    src/qt/multisigaddressentry.h \
    src/qt/multisiginputentry.h \
    src/qt/multisigdialog.h \
    src/version.h \
    src/netbase.h \
    src/clientversion.h \
    src/txdb.h \
    src/leveldb.h \
    src/threadsafety.h \
    src/limitedmap.h \
    src/qt/macnotificationhandler.h \
    src/qt/splashscreen.h \
    src/$${TARGET}.h \
    src/build.h

SOURCES += src/qt/codecoin.cpp \
    src/qt/codecoingui.cpp \
    src/qt/transactiontablemodel.cpp \
    src/qt/addresstablemodel.cpp \
    src/qt/optionsdialog.cpp \
    src/qt/sendcoinsdialog.cpp \
    src/qt/coincontroldialog.cpp \
    src/qt/coincontroltreewidget.cpp \
    src/qt/addressbookpage.cpp \
    src/qt/signverifymessagedialog.cpp \
    src/qt/aboutdialog.cpp \
    src/qt/editaddressdialog.cpp \
    src/qt/bitcoinaddressvalidator.cpp \
    src/version.cpp \
    src/sync.cpp \
    src/util.cpp \
    src/hash.cpp \
    src/netbase.cpp \
    src/key.cpp \
    src/script.cpp \
    src/main.cpp \
    src/undo.cpp \
    src/$${TARGET}.cpp \
    src/init.cpp \
    src/net.cpp \
    src/bloom.cpp \
    src/checkpoints.cpp \
    src/addrman.cpp \
    src/db.cpp \
    src/walletdb.cpp \
    src/json/json_spirit_writer.cpp \
    src/json/json_spirit_value.cpp \
    src/json/json_spirit_reader.cpp \
    src/qt/clientmodel.cpp \
    src/qt/guiutil.cpp \
    src/qt/transactionrecord.cpp \
    src/qt/optionsmodel.cpp \
    src/qt/monitoreddatamapper.cpp \
    src/qt/transactiondesc.cpp \
    src/qt/transactiondescdialog.cpp \
    src/qt/codecoinstrings.cpp \
    src/qt/bitcoinamountfield.cpp \
    src/wallet.cpp \
    src/keystore.cpp \
    src/qt/transactionfilterproxy.cpp \
    src/qt/transactionview.cpp \
    src/qt/walletmodel.cpp \
    src/codecoinrpc.cpp \
    src/rpcdump.cpp \
    src/rpcnet.cpp \
    src/rpcmining.cpp \
    src/rpcwallet.cpp \
    src/rpcblockchain.cpp \
    src/rpcrawtransaction.cpp \
    src/qt/overviewpage.cpp \
    src/qt/csvmodelwriter.cpp \
    src/crypter.cpp \
    src/qt/sendcoinsentry.cpp \
    src/qt/qvalidatedlineedit.cpp \
    src/qt/codecoinunits.cpp \
    src/qt/qvaluecombobox.cpp \
    src/qt/askpassphrasedialog.cpp \
    src/protocol.cpp \
    src/qt/notificator.cpp \
    src/qt/paymentserver.cpp \
    src/qt/rpcconsole.cpp \
    src/qt/virtualkeyboard.cpp \
    src/qt/multisigaddressentry.cpp \
    src/qt/multisiginputentry.cpp \
    src/qt/multisigdialog.cpp \
    src/noui.cpp \
    src/leveldb.cpp \
    src/txdb.cpp \
    src/qt/splashscreen.cpp

contains(STAKE, 1){
HEADERS += src/kernel.h src/kernelrecord.h \
    src/kernel.h \
    src/qt/mintingview.h \
    src/qt/mintingtablemodel.h \
    src/qt/mintingfilterproxy.h \
    src/kernelrecord.h 
SOURCES += src/kernel.cpp src/kernelrecord.cpp \
    src/qt/mintingview.cpp \
    src/qt/mintingtablemodel.cpp \
    src/qt/mintingfilterproxy.cpp 
}

contains(HASHX11, 1){
SOURCES += src/cubehash.c src/luffa.c src/aes_helper.c \
    src/echo.c src/shavite.c src/simd.c src/blake.c \
    src/bmw.c src/groestl.c src/jh.c src/keccak.c src/skein.c
}

contains(HASHSCRYPT, 1){
HEADERS += src/scrypt.h
SOURCES += src/scrypt.cpp
}

#leveldb sources
SOURCES += src/leveldb/db/builder.cc src/leveldb/db/c.cc src/leveldb/db/dbformat.cc src/leveldb/db/db_impl.cc \
    src/leveldb/db/db_iter.cc src/leveldb/db/filename.cc src/leveldb/db/log_reader.cc src/leveldb/db/log_writer.cc \
    src/leveldb/db/memtable.cc src/leveldb/db/repair.cc src/leveldb/db/table_cache.cc src/leveldb/db/version_edit.cc \
    src/leveldb/db/version_set.cc src/leveldb/db/write_batch.cc src/leveldb/table/block_builder.cc \
    src/leveldb/table/block.cc src/leveldb/table/filter_block.cc src/leveldb/table/format.cc src/leveldb/table/iterator.cc \
    src/leveldb/table/merger.cc src/leveldb/table/table_builder.cc src/leveldb/table/table.cc \
    src/leveldb/table/two_level_iterator.cc src/leveldb/util/arena.cc \
    src/leveldb/util/db_bloom.cc src/leveldb/util/cache.cc src/leveldb/util/coding.cc src/leveldb/util/comparator.cc \
    src/leveldb/util/crc32c.cc src/leveldb/util/env.cc src/leveldb/util/env_posix.cc src/leveldb/util/env_win.cc \
    src/leveldb/util/filter_policy.cc src/leveldb/util/db_hash.cc src/leveldb/util/histogram.cc src/leveldb/util/logging.cc \
    src/leveldb/util/options.cc src/leveldb/util/status.cc 

unix:SOURCES += src/leveldb/port/port_posix.cc
windows:SOURCES += src/leveldb/port/port_win.cc

#leveldb libmemv
SOURCES += src/leveldb/helpers/memenv/memenv.cc

# TODO: figure out how to dereference COIN_BRAND properly
RESOURCES += src/qt/res/$$TARGET/codecoin.qrc

FORMS += src/qt/forms/sendcoinsdialog.ui \
    src/qt/forms/coincontroldialog.ui \
    src/qt/forms/addressbookpage.ui \
    src/qt/forms/signverifymessagedialog.ui \
    src/qt/forms/aboutdialog.ui \
    src/qt/forms/editaddressdialog.ui \
    src/qt/forms/transactiondescdialog.ui \
    src/qt/forms/overviewpage.ui \
    src/qt/forms/sendcoinsentry.ui \
    src/qt/forms/askpassphrasedialog.ui \
    src/qt/forms/rpcconsole.ui \
    src/qt/forms/multisigaddressentry.ui \
    src/qt/forms/multisiginputentry.ui \
    src/qt/forms/multisigdialog.ui

contains(USE_QRCODE, 1) {
HEADERS += src/qt/qrcodedialog.h
SOURCES += src/qt/qrcodedialog.cpp
FORMS += src/qt/forms/qrcodedialog.ui
}

contains(CODECOIN_QT_TEST, 1) {
SOURCES += src/qt/test/test_main.cpp \
    src/qt/test/uritests.cpp
HEADERS += src/qt/test/uritests.h
DEPENDPATH += src/qt/test
QT += testlib
TARGET = codecoin-qt_test
DEFINES += BITCOIN_QT_TEST
  macx: CONFIG -= app_bundle
}

contains(USE_SSE2, 1) {
    DEFINES += USE_SSE2
    gccsse2.input  = SOURCES_SSE2
    gccsse2.output = $$PWD/build/${QMAKE_FILE_BASE}.o
    gccsse2.commands = $(CXX) -c $(CXXFLAGS) $(INCPATH) -o ${QMAKE_FILE_OUT} ${QMAKE_FILE_NAME} -msse2 -mstackrealign
    QMAKE_EXTRA_COMPILERS += gccsse2
    SOURCES_SSE2 += src/scrypt-sse2.cpp
}

# use: qmake "STATICBUILD=1"
contains(STATICBUILD, 1) {
    !macx:CONFIG += static
    !macx:QMAKE_LFLAGS *= -static
}

# Todo: Remove this line when switching to Qt5, as that option was removed
CODECFORTR = UTF-8

# for lrelease/lupdate
# also add new translations to src/qt/codecoin.qrc under translations/
TRANSLATIONS = $$files(src/qt/locale/codecoin_*.ts)

isEmpty(QMAKE_LRELEASE) {
    win32:QMAKE_LRELEASE = $$[QT_INSTALL_BINS]\\lrelease.exe
    else:QMAKE_LRELEASE = $$[QT_INSTALL_BINS]/lrelease
}
isEmpty(QM_DIR):QM_DIR = $$PWD/src/qt/locale
# automatically build translations, so they can be included in resource file
TSQM.name = lrelease ${QMAKE_FILE_IN}
TSQM.input = TRANSLATIONS
TSQM.output = $$QM_DIR/${QMAKE_FILE_BASE}.qm
TSQM.commands = $$QMAKE_LRELEASE ${QMAKE_FILE_IN} -qm ${QMAKE_FILE_OUT}
TSQM.CONFIG = no_link
QMAKE_EXTRA_COMPILERS += TSQM

# "Other files" to show in Qt Creator
OTHER_FILES += README.md \
    doc/*.rst \
    doc/*.txt \
    doc/*.md \
    src/qt/res/bitcoin-qt.rc \
    src/test/*.cpp \
    src/test/*.h \
    src/qt/test/*.cpp \
    src/qt/test/*.h

# platform specific defaults, if not overridden on command line
isEmpty(BOOST_LIB_SUFFIX) {
#    macx:BOOST_LIB_SUFFIX = -mt
    windows:BOOST_LIB_SUFFIX = -mgw44-mt-1_53
    android:BOOST_LIB_SUFFIX = -gcc-mt-1_53
}

isEmpty(BOOST_THREAD_LIB_SUFFIX) {
    BOOST_THREAD_LIB_SUFFIX = $$BOOST_LIB_SUFFIX
}

isEmpty(BDB_LIB_PATH) {
    !linux:BDB_LIB_PATH = deps/lib
#    macx:BDB_LIB_PATH = /opt/local/lib/db53
    android:BDB_LIB_PATH = deps/lib
}

isEmpty(BDB_LIB_SUFFIX) {
    macx:BDB_LIB_SUFFIX = -5.3
}

isEmpty(BDB_INCLUDE_PATH) {
    !linux:BDB_INCLUDE_PATH = deps/include
#    macx:BDB_INCLUDE_PATH = /opt/local/include/db53
    android:BDB_INCLUDE_PATH = deps/include
}

isEmpty(BOOST_LIB_PATH) {
#    macx:BOOST_LIB_PATH = /opt/local/lib
}

isEmpty(BOOST_INCLUDE_PATH) {
#    macx:BOOST_INCLUDE_PATH = /opt/local/include
}

win32:DEFINES += WIN32
win32:RC_FILE = src/qt/res/codecoin-qt.rc

win32:!contains(MINGW_THREAD_BUGFIX, 0) {
    # At least qmake's win32-g++-cross profile is missing the -lmingwthrd
    # thread-safety flag. GCC has -mthreads to enable this, but it doesn't
    # work with static linking. -lmingwthrd must come BEFORE -lmingw, so
    # it is prepended to QMAKE_LIBS_QT_ENTRY.
    # It can be turned off with MINGW_THREAD_BUGFIX=0, just in case it causes
    # any problems on some untested qmake profile now or in the future.
    DEFINES += _MT
    QMAKE_LIBS_QT_ENTRY = -lmingwthrd $$QMAKE_LIBS_QT_ENTRY
}

!win32:!macx {
    DEFINES += LINUX
    !android:LIBS += -lrt
    # _FILE_OFFSET_BITS=64 lets 32-bit fopen transparently support large files.
    DEFINES += _FILE_OFFSET_BITS=64
}

macx:HEADERS += src/qt/macdockiconhandler.h src/qt/macnotificationhandler.h
macx:OBJECTIVE_SOURCES += src/qt/macdockiconhandler.mm src/qt/macnotificationhandler.mm
macx:LIBS += -framework Foundation -framework ApplicationServices -framework AppKit -framework CoreServices
macx:DEFINES += MAC_OSX MSG_NOSIGNAL=0 HAVE_CXX_STDHEADERS
macx:ICON = src/qt/res/icons/$${TARGET}.icns
macx:QMAKE_CFLAGS_THREAD += -pthread
macx:QMAKE_LFLAGS_THREAD += -pthread
macx:QMAKE_CXXFLAGS_THREAD += -pthread
macx:QMAKE_INFO_PLIST = share/qt/Info.plist
macx:OPENSSL_INCLUDE_PATH = /opt/local/include

# Set libraries and includes at end, to use platform-defined defaults if not overridden
INCLUDEPATH += $$BOOST_INCLUDE_PATH $$BDB_INCLUDE_PATH $$OPENSSL_INCLUDE_PATH $$QRENCODE_INCLUDE_PATH
LIBS += $$join(BOOST_LIB_PATH,,-L,) $$join(BDB_LIB_PATH,,-L,) $$join(OPENSSL_LIB_PATH,,-L,) $$join(QRENCODE_LIB_PATH,,-L,)
LIBS += -lssl -lcrypto -ldb_cxx$$BDB_LIB_SUFFIX
# -lgdi32 has to happen after -lcrypto (see  #681)
win32:LIBS += -lws2_32 -lshlwapi -lmswsock -lole32 -loleaut32 -luuid -lgdi32
LIBS += -lboost_system$$BOOST_LIB_SUFFIX -lboost_filesystem$$BOOST_LIB_SUFFIX -lboost_program_options$$BOOST_LIB_SUFFIX -lboost_thread$$BOOST_THREAD_LIB_SUFFIX
win32:LIBS += -lboost_chrono$$BOOST_LIB_SUFFIX
macx:LIBS += -lboost_chrono$$BOOST_LIB_SUFFIX

contains(RELEASE, 1) {
    !win32:!macx {
        # Linux: turn dynamic linking back on for c/c++ runtime libraries
        LIBS += -Wl,-Bdynamic
    }
}

system($$QMAKE_LRELEASE -silent $$TRANSLATIONS)

#DISTFILES += \
#    android/gradle/wrapper/gradle-wrapper.jar \
#    android/AndroidManifest.xml \
#    android/gradlew.bat \
#    android/res/values/libs.xml \
#    android/build.gradle \
#    android/gradle/wrapper/gradle-wrapper.properties \
#    android/gradlew

#ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android

DISTFILES += \
    android/AndroidManifest.xml \
    android/gradle/wrapper/gradle-wrapper.jar \
    android/gradlew \
    android/res/values/libs.xml \
    android/build.gradle \
    android/gradle/wrapper/gradle-wrapper.properties \
    android/gradlew.bat

ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android
