/*
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
*/

#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QApplication::setApplicationName(QString("fargerom"));
    QApplication::setOrganizationName(QString("NettStudio AS"));
    QApplication::setOrganizationDomain(QString("nettstudio.no"));
    QApplication::setApplicationVersion(QString("1.0.0 RC1"));
    QStringList args = QApplication::arguments();

    MainWindow w(args);
    w.show();
    return a.exec();
}
