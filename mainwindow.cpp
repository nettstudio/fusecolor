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
#include <QDirIterator>
#include <QSettings>
#include <QMessageBox>

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
    QObject::connect(this, SIGNAL(showWarning(QString,QString)),
                     this, SLOT(handleWarning(QString,QString)));
    QObject::connect(this, SIGNAL(stopProgress()),
                     this, SLOT(progressClear()));

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
    version.append(QString("<h2 style=\"text-align:center;\">Color Converter %1</h2>").arg(qApp->applicationVersion()));
    QString info;
    info.append(QString("<p style=\"text-align:center;font-size:small;\">&copy; 2021 <a href=\"https://nettstudio.no\">%1</a>. All rights reserved.</p>").arg(qApp->organizationName()));
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

    QIcon itemIcon(":/fargerom.png");

    QString noProfileText = tr("Select output");
    ui->selectedProfile->addItem(itemIcon, noProfileText);
    ui->selectedProfile->insertSeparator(1);

    populateColorProfiles(colorSpaceRGB, ui->selectedProfile, false);
    populateColorProfiles(colorSpaceCMYK, ui->selectedProfile, false);
    populateColorProfiles(colorSpaceGRAY, ui->selectedProfile, false);

    ui->selectedIntent->addItem(itemIcon, tr("Undefined"),
                             UndefinedRenderingIntent);
    ui->selectedIntent->addItem(itemIcon, tr("Saturation"),
                             SaturationRenderingIntent);
    ui->selectedIntent->addItem(itemIcon, tr("Perceptual"),
                             PerceptualRenderingIntent);
    ui->selectedIntent->addItem(itemIcon, tr("Absolute"),
                             AbsoluteRenderingIntent);
    ui->selectedIntent->addItem(itemIcon, tr("Relative"),
                             RelativeRenderingIntent);
    ui->selectedIntent->setCurrentIndex(2);
}

void MainWindow::progressBusy()
{
    _isBusy = true;
    ui->progressBar->setFormat(tr("Converting ..."));
    ui->progressBar->setMaximum(0);
    ui->progressBar->setValue(0);
}

void MainWindow::progressClear()
{
    _isBusy = false;
    ui->progressBar->setMaximum(1);
    ui->progressBar->setValue(1);
    ui->progressBar->setFormat(tr("Ready!"));
}

void MainWindow::handleUrls(QList<QUrl> urls)
{
    int added = 0;
    for (int i = 0; i < urls.size(); ++i) {
        QString filename = urls.at(i).toLocalFile();
        if (!QFile::exists(filename)) { continue; }
        if (!isValidImage(filename)) { continue; }
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
    qDebug() << "convertUrls" << urls;

    QString outputColorProfile = ui->selectedProfile->itemData(ui->selectedProfile->currentIndex()).toString();

    if (outputColorProfile.isEmpty()) {
        Q_EMIT showWarning(tr("Missing output profile"),
                           tr("No output profile selected, unable to convert."));
        Q_EMIT stopProgress();
        return;
    }

    QByteArray outputProfileData = fileToByteArray(outputColorProfile);
    if (!isValidProfile(outputProfileData)) {
        Q_EMIT showWarning(tr("Missing output profile"),
                           tr("No output profile selected, unable to convert."));
        Q_EMIT stopProgress();
        return;
    }

    RenderingIntent intent = (RenderingIntent)ui->selectedIntent->itemData(ui->selectedIntent->currentIndex()).toInt();

    QString suf;
    switch (getFileColorspace(outputProfileData)) {
    case colorSpaceCMYK:
        suf = QString("CMYK");
        break;
    case colorSpaceGRAY:
        suf = QString("GRAY");
        break;
    default:
        suf = QString("RGB");
    }

    for (int i = 0; i < urls.size(); ++i) {
        QString filename = urls.at(i).toLocalFile();
        QFileInfo fileInfo(filename);
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
            while (QFile::exists(QString("%5/%1_%2_copy%4.%3").arg(fileInfo.baseName()).arg(suf).arg(ext).arg(counter).arg(filePath))) {
                counter++;
            }
            oFilename = QString("%5/%1_%2_copy%4.%3").arg(fileInfo.baseName()).arg(suf).arg(ext).arg(counter).arg(filePath);
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
            Magick::Blob outputProfile(outputProfileData.data(), outputProfileData.size());
            image.quiet(true);
            if (image.colorSpace() == Magick::YCbCrColorspace) {
                image.colorSpace(Magick::sRGBColorspace);
            }
            switch (intent) {
            case SaturationRenderingIntent:
                image.renderingIntent(Magick::SaturationIntent);
                break;
            case PerceptualRenderingIntent:
                image.renderingIntent(Magick::PerceptualIntent);
                break;
            case AbsoluteRenderingIntent:
                image.renderingIntent(Magick::AbsoluteIntent);
                break;
            case RelativeRenderingIntent:
                image.renderingIntent(Magick::RelativeIntent);
                break;
            default:;
            }

            image.blackPointCompensation(ui->blackPoint->isChecked());

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

void MainWindow::handleWarning(const QString &title, const QString &msg)
{
    QMessageBox::warning(this, title, msg);
}

QByteArray MainWindow::fileToByteArray(const QString &filename)
{
    if (QFile::exists(filename)) {
        QFile file(filename);
        if (file.open(QIODevice::ReadOnly)) {
            QByteArray profile =  file.readAll();
            file.close();
            if (profile.size() > 0) {
                return profile;
            }
        }
    }
    return QByteArray();
}

bool MainWindow::isValidImage(const QString &filename)
{
    QStringList supported;
    supported << "image/jpeg" << "image/png" << "image/tiff";
    if (QFile::exists(filename)) {
        QMimeDatabase db;
        QMimeType type = db.mimeTypeForFile(filename);
        if (supported.contains(type.name())) { return true; }
    }
    return false;
}

bool MainWindow::isValidProfile(QByteArray buffer)
{
    if (buffer.size() > 0) {
        if (getFileColorspace(buffer) != colorSpaceUnknown) { return true; }
    }
    return false;
}

MainWindow::colorSpace MainWindow::getFileColorspace(cmsHPROFILE profile)
{
    colorSpace cs = colorSpaceUnknown;
    if (profile) {
        if (cmsGetColorSpace(profile) == cmsSigRgbData) {
            cs = colorSpaceRGB;
        } else if (cmsGetColorSpace(profile) == cmsSigCmykData) {
            cs = colorSpaceCMYK;
        } else if (cmsGetColorSpace(profile) == cmsSigGrayData) {
            cs = colorSpaceGRAY;
        }
    }
    cmsCloseProfile(profile);
    return cs;
}

MainWindow::colorSpace MainWindow::getFileColorspace(const QString &filename)
{
    if (QFile::exists(filename)) {
        return getFileColorspace(cmsOpenProfileFromFile(filename.toStdString().c_str(), "r"));
    }
    return colorSpaceUnknown;
}

MainWindow::colorSpace MainWindow::getFileColorspace(QByteArray buffer)
{
    if (buffer.size()>0) {
        return getFileColorspace(cmsOpenProfileFromMem(buffer.data(),
                                                       static_cast<cmsUInt32Number>(buffer.size())));
    }
    return colorSpaceUnknown;
}

QString MainWindow::getProfileTag(cmsHPROFILE profile, MainWindow::ICCTag tag)
{
    QString result;
    if (profile) {
        cmsUInt32Number size = 0;
        cmsInfoType cmsSelectedType;
        switch(tag) {
        case ICCManufacturer:
            cmsSelectedType = cmsInfoManufacturer;
            break;
        case ICCModel:
            cmsSelectedType = cmsInfoModel;
            break;
        case ICCCopyright:
            cmsSelectedType = cmsInfoCopyright;
            break;
        default:
            cmsSelectedType = cmsInfoDescription;
        }
        size = cmsGetProfileInfoASCII(profile, cmsSelectedType,
                                      "en", "US", nullptr, 0);
        if (size > 0) {
            std::vector<char> buffer(size);
            cmsUInt32Number newsize = cmsGetProfileInfoASCII(profile, cmsSelectedType,
                                          "en", "US", &buffer[0], size);
            if (size == newsize) {
                result = buffer.data();
            }
        }
    }
    cmsCloseProfile(profile);
    return result;

}

QString MainWindow::getProfileTag(const QString &filename, MainWindow::ICCTag tag)
{
    if (QFile::exists(filename)) {
        return getProfileTag(cmsOpenProfileFromFile(filename.toStdString().c_str(), "r"), tag);
    }
    return "";
}

QString MainWindow::getProfileTag(QByteArray buffer, MainWindow::ICCTag tag)
{
    if (buffer.size()>0) {
        return getProfileTag(cmsOpenProfileFromMem(buffer.data(),
                                                   static_cast<cmsUInt32Number>(buffer.size())),tag);
    }
    return "";
}

void MainWindow::populateColorProfiles(MainWindow::colorSpace cs, QComboBox *box, bool proof)
{
    Q_UNUSED(proof)
    if (!box) { return; }

    /*QString defaultProfile;
    int defaultIndex = -1;
    QSettings settings;
    settings.beginGroup("profiles");
    QString profileType;
    if (isMonitor) {
        profileType = "monitor";
    } else {
        profileType = QString::number(colorspace);
    }
    if (!settings.value(profileType).toString().isEmpty()) {
        defaultProfile = settings.value(profileType).toString();
    }
    settings.endGroup();*/

    QMap<QString,QString> profiles = getProfiles(cs);
    if (profiles.size() > 0) {
        //box->clear();
        QIcon itemIcon(":/fargerom.png");
        /*QString noProfileText = tr("Select ...");
        if (proof) {
            noProfileText = tr("None");
        }
        box->addItem(itemIcon, noProfileText);
        box->insertSeparator(1);*/

        int it = 0;
        QMapIterator<QString, QString> profile(profiles);
        while (profile.hasNext()) {
            profile.next();
            box->addItem(itemIcon, profile.key(), profile.value());
            //if (profile.value() == defaultProfile) { defaultIndex = it+2; }
            ++it;
        }
        /*if (defaultIndex >= 0) {
            box->setCurrentIndex(defaultIndex);
        }*/
    }

}

QMap<QString, QString> MainWindow::getProfiles(MainWindow::colorSpace colorspace)
{
    QMap<QString,QString> output;
    QStringList folders;
    folders << QDir::rootPath() + "/WINDOWS/System32/spool/drivers/color";
    folders << "/Library/ColorSync/Profiles";
    folders << QDir::homePath() + "/Library/ColorSync/Profiles";
    folders << "/usr/share/color/icc";
    folders << "/usr/local/share/color/icc";
    folders << qApp->applicationDirPath() + "/../share/color/icc";
    folders << QDir::homePath() + "/.color/icc";
    folders << QDir::homePath() + "/.config/Cyan/icc";
    folders << qApp->applicationDirPath() + "/profiles";
    folders << qApp->applicationDirPath() + "/icc";

    for (int i = 0; i < folders.size(); ++i) {
        QStringList filter;
        filter << "*.icc" << "*.icm";
        QDirIterator it(folders.at(i), filter, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            QString iccFile = it.next();
            QString profile = getProfileTag(iccFile, ICCDescription);
            if (iccFile.isEmpty() || profile.isEmpty()) { continue; }
            if (getFileColorspace(iccFile) != colorspace) { continue; }
            output[profile] = iccFile;
        }
    }
    return output;
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

