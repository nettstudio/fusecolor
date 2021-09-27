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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QUrl>
#include <QList>
#include <QByteArray>
#include <QStringList>
#include <QComboBox>
#include <QMap>

#include <lcms2.h>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:

    enum colorSpace {
        colorSpaceUnknown,
        colorSpaceRGB,
        colorSpaceCMYK,
        colorSpaceGRAY
    };
    enum ICCTag {
        ICCDescription,
        ICCManufacturer,
        ICCModel,
        ICCCopyright
    };
    enum RenderingIntent {
        UndefinedRenderingIntent,
        SaturationRenderingIntent,
        PerceptualRenderingIntent,
        AbsoluteRenderingIntent,
        RelativeRenderingIntent
    };

    MainWindow(QStringList args, QWidget *parent = nullptr);
    ~MainWindow();

Q_SIGNALS:
    void droppedUrls(QList<QUrl> urls);
    void convertDone();
    void showWarning(const QString &title, const QString &msg);
    void stopProgress();

private Q_SLOTS:
    void setupTheme();
    void setupInfo();
    void setupICC();
    void progressBusy();
    void progressClear();
    void handleUrls(QList<QUrl> urls);
    void convertUrls(QList<QUrl> urls);
    void convertedUrls();
    void handleArgs(QStringList args);
    void handleWarning(const QString &title, const QString &msg);
    QByteArray fileToByteArray(const QString &filename);
    bool isValidImage(const QString &filename);
    bool isValidProfile(QByteArray buffer);
    colorSpace getFileColorspace(cmsHPROFILE profile);
    colorSpace getFileColorspace(const QString &filename);
    colorSpace getFileColorspace(QByteArray buffer);
    QString getProfileTag(cmsHPROFILE profile, ICCTag tag);
    QString getProfileTag(const QString &filename, ICCTag tag);
    QString getProfileTag(QByteArray buffer, ICCTag tag);
    void populateColorProfiles(colorSpace cs, QComboBox *box, bool proof);
    QMap<QString, QString> getProfiles(colorSpace colorspace);

private:
    Ui::MainWindow *ui;
    QList<QUrl> _queue;
    bool _isBusy;
    QByteArray _iccRgb;
    QByteArray _iccCmyk;
    QByteArray _iccGray;

protected:
    void dropEvent(QDropEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
};
#endif // MAINWINDOW_H
