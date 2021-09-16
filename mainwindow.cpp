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
#include "ui_mainwindow.h"

#include <QStyleFactory>
#include <QDebug>
#include <QMimeData>
#include <QMimeDatabase>
#include <QMimeType>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QStandardPaths>
#include <QtConcurrent/QtConcurrentRun>
#include <QTimer>

#include <Magick++.h>

MainWindow::MainWindow(QStringList args,
                       QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , _isBusy(false)
{
    ui->setupUi(this);

    setupTheme();
    setupInfo();
    setupICC();
    progressClear();

    QObject::connect(this, SIGNAL(droppedUrls(QList<QUrl>)),
                     this, SLOT(handleUrls(QList<QUrl>)));
    QObject::connect(this, SIGNAL(convertDone()),
                     this, SLOT(convertedUrls()));
    handleArgs(args);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupTheme()
{
    qApp->setStyle(QStyleFactory::create("fusion"));
    QIcon::setThemeName("hicolor");
    QPalette palette;
    palette.setColor(QPalette::Window, QColor(53,53,53));
    palette.setColor(QPalette::WindowText, Qt::white);
    palette.setColor(QPalette::Base, QColor(15,15,15));
    palette.setColor(QPalette::AlternateBase, QColor(53,53,53));
    palette.setColor(QPalette::Link, Qt::white);
    palette.setColor(QPalette::LinkVisited, Qt::white);
    palette.setColor(QPalette::ToolTipText, Qt::black);
    palette.setColor(QPalette::Text, Qt::white);
    palette.setColor(QPalette::Button, QColor(53,53,53));
    palette.setColor(QPalette::ButtonText, Qt::white);
    palette.setColor(QPalette::BrightText, Qt::red);
    palette.setColor(QPalette::Highlight, QColor(196,110,33));
    palette.setColor(QPalette::HighlightedText, Qt::white);
    palette.setColor(QPalette::Disabled, QPalette::Text, Qt::darkGray);
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, Qt::darkGray);
    qApp->setPalette(palette);
}

void MainWindow::setupInfo()
{
    QString version;
    version.append(QString("<h2 style=\"text-align:center;\">%1 %2</h2><p style=\"text-align:center;\">Klargjør dine bilder for nett (sRGB)</p>").arg(qApp->applicationName()).arg(qApp->applicationVersion()));
    QString info;
    info.append(QString("<p style=\"text-align:center;font-size:small;\">&copy; 2021 <a href=\"https://nettstudio.no\">%1</a>.<br>Alle rettigheter forbeholdt.<br>%2 er <a href=\"https://github.com/nettstudio/fargerom\">åpen kildekode</a> (GPL-3).</p>").arg(qApp->organizationName()).arg(qApp->applicationName()));
    ui->info->setText(info);
    ui->info->setOpenExternalLinks(true);
    ui->version->setText(version);
}

void MainWindow::setupICC()
{
    QFile readRgb(QString(":/profile-rgb.icc"));
    if (readRgb.open(QIODevice::ReadOnly)) {
        _iccRgb = readRgb.readAll();
        readRgb.close();
    }
    QFile readCmyk(QString(":/profile-cmyk.icc"));
    if (readCmyk.open(QIODevice::ReadOnly)) {
        _iccCmyk = readCmyk.readAll();
        readCmyk.close();
    }
    QFile readGray(QString(":/profile-gray.icc"));
    if (readGray.open(QIODevice::ReadOnly)) {
        _iccGray = readGray.readAll();
        readGray.close();
    }
}

void MainWindow::progressBusy()
{
    _isBusy = true;
    ui->progressBar->setFormat(QString("Konverterer ..."));
    ui->progressBar->setMaximum(0);
    ui->progressBar->setValue(0);
}

void MainWindow::progressClear()
{
    _isBusy = false;
    ui->progressBar->setMaximum(1);
    ui->progressBar->setValue(1);
    ui->progressBar->setFormat(QString("Slipp bilder i dette vindu"));
}

void MainWindow::handleUrls(QList<QUrl> urls)
{
    int added = 0;
    for (int i = 0; i < urls.size(); ++i) {
        QString filename = urls.at(i).toLocalFile();
        if (!QFile::exists(filename)) { continue; }
        QMimeDatabase mimeDb;
        QMimeType mimeType = mimeDb.mimeTypeForFile(filename);
        QString mime = mimeType.name();
        if (!mime.startsWith(QString("image/"))) { continue; }
        if (_queue.contains(QUrl::fromLocalFile(filename))) { continue; }
        _queue.append(QUrl::fromLocalFile(filename));
        added++;
    }
    if (added > 0 && !_isBusy) {
        progressBusy();
        QtConcurrent::run(this, &MainWindow::convertUrls, _queue);
        _queue.clear();
    }
}

void MainWindow::convertUrls(QList<QUrl> urls)
{
    for (int i = 0; i < urls.size(); ++i) {
        QString filename = urls.at(i).toLocalFile();
        QFileInfo fileInfo(filename);
        QString suf = QString("sRGB");
        QString ext = fileInfo.suffix().toLower();
        int counter = 1;
        QString filePath = fileInfo.absolutePath();
        if (!fileInfo.isWritable()) {
            filePath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
            if (filePath.isEmpty()) {
                filePath = QDir::homePath();
            }
        }
        QString oFilename = QString("%4/%1_%2.%3").arg(fileInfo.baseName()).arg(suf).arg(ext).arg(filePath);
        if (QFile::exists(oFilename)) {
            while (QFile::exists(QString("%5/%1_%2_kopi%4.%3").arg(fileInfo.baseName()).arg(suf).arg(ext).arg(counter).arg(filePath))) {
                counter++;
            }
            oFilename = QString("%5/%1_%2_kopi%4.%3").arg(fileInfo.baseName()).arg(suf).arg(ext).arg(counter).arg(filePath);
        }

        qDebug() << "CONVERT" << filename << oFilename;

        Magick::Image image;
        try {
            image.read(filename.toStdString());
        }
        catch(Magick::Error &error ) { qWarning() << error.what(); }
        catch(Magick::Warning &warn ) { qWarning() << warn.what(); }
        try {
            QByteArray inputProfileData;
            switch(image.colorSpace()) {
            case Magick::CMYKColorspace:
                inputProfileData = _iccCmyk;
                break;
            case Magick::GRAYColorspace:
                inputProfileData = _iccGray;
                break;
            default:
                inputProfileData = _iccRgb;
            }
            Magick::Blob inputProfile(inputProfileData.data(), inputProfileData.size());
            Magick::Blob outputProfile(_iccRgb.data(), _iccRgb.size());
            image.quiet(true);
            if (image.colorSpace() == Magick::YCbCrColorspace) {
                image.colorSpace(Magick::sRGBColorspace);
            }
            if (image.iccColorProfile().length() == 0) {
                image.profile("ICC", inputProfile);
            }
            image.profile("ICC", outputProfile);
        }
        catch(Magick::Error &error ) { qWarning() << error.what(); }
        catch(Magick::Warning &warn ) { qWarning() << warn.what(); }
        try {
            image.write(oFilename.toStdString());
        }
        catch(Magick::Error &error ) { qWarning() << error.what(); }
        catch(Magick::Warning &warn ) { qWarning() << warn.what(); }
    }
    Q_EMIT convertDone();
}

void MainWindow::convertedUrls()
{
    progressClear();
    if (!_queue.isEmpty()) {
        progressBusy();
        QtConcurrent::run(this, &MainWindow::convertUrls, _queue);
        _queue.clear();
    }
}

void MainWindow::handleArgs(QStringList args)
{
    qDebug() << "handle args" << args;
    QList<QUrl> urls;
    for (int i = 0; i < args.size(); ++i) {
        if (!QFile::exists(args.at(i))) { continue; }
        urls.append(QUrl::fromLocalFile(args.at(i)));
    }
    if (urls.size() > 0) {
        Q_EMIT droppedUrls(urls);
    }
}

void MainWindow::dropEvent(QDropEvent *event)
{
    const QMimeData* mimeData = event->mimeData();
    if (mimeData->hasUrls()) {
        Q_EMIT droppedUrls(mimeData->urls());
    }
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    event->acceptProposedAction();
}

