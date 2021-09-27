#
# Copyright (c) 2021 NettStudio AS. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>
#

VERSION = 2.0.0
QT       += core gui concurrent

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

DESTDIR = build
OBJECTS_DIR = $${DESTDIR}/.obj
MOC_DIR = $${DESTDIR}/.moc
RCC_DIR = $${DESTDIR}/.qrc

QMAKE_TARGET_COMPANY = "NettStudio AS"
QMAKE_TARGET_PRODUCT = "color-converter"
QMAKE_TARGET_DESCRIPTION = "Image Color Converter"
QMAKE_TARGET_COPYRIGHT = "Copyright NettStudio AS"

CONFIG(release, debug|release):DEFINES += QT_NO_DEBUG_OUTPUT
win32: RC_ICONS += fargerom.ico

SOURCES += \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    mainwindow.h

FORMS += \
    mainwindow.ui

RESOURCES += \
    assets.qrc

QT_CONFIG -= no-pkg-config
CONFIG += link_pkgconfig
MAGICK_CONFIG = Magick++
!isEmpty(MAGICK): MAGICK_CONFIG = $${MAGICK}

PKG_CONFIG_BIN = pkg-config
!isEmpty(CUSTOM_PKG_CONFIG): PKG_CONFIG_BIN = $${CUSTOM_PKG_CONFIG}

PKGCONFIG += $${MAGICK_CONFIG} lcms2
LIBS += `$${PKG_CONFIG_BIN} --libs --static $${MAGICK_CONFIG}`

DEFINES += VERSION_APP=\\\"$${VERSION}\\\"
