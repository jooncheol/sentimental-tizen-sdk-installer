/*
 * Sentimental Tizen SDK Installer
 * A Qt based Unofficial Tizen SDK Installer by MeeGo fan
 *
 * Author: JoonCheol Park <jooncheol at gmail dot com>
 * Copyright (C) 2012 JoonCheol Park
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#include <sys/stat.h>
#ifdef __linux__
#include <sys/vfs.h>
#endif
#include "installwizard.h"
#include <QCoreApplication>
#include <QSettings>
#include <QIcon>
#include <QLabel>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QPainter>
#include <QDebug>
#include <QTextEdit>
#include <QFont>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QTextCodec>
#include <QRegExpValidator>
#include <QRegExp>
#include <QtNetwork/QNetworkProxy>
#include <QNetworkRequest>
#include <QUrl>
#include <QNetworkReply>
#include <QTreeView>
#include <QMessageBox>
#include <QHeaderView>
#include <QPushButton>
#include <QFileDialog>
#include <QDir>
#include <sys/statvfs.h>
#include <sys/param.h>
#include <sys/mount.h>
#include <QDesktopServices>
#include "quazipfile.h"

static QTextCodec *gCodec = NULL;
InstallWizard::InstallWizard(const QString &tizenPackageIndexUrl, bool withMeeGo, QWidget *parent)
    : QWizard(parent)
{
    mPackageIndex = new TizenPackageIndex();
    mPackageListUrl = QUrl(tizenPackageIndexUrl);

    setPage(Intro, new IntroPage);
    setPage(InstallLicense, new InstallLicensePage);
    setPage(InstallConfigureNetwork, new InstallConfigurePageNetwork);
    setPage(InstallConfigure, new InstallConfigurePage);
    setPage(InstallConfigureLocation, new InstallConfigurePageLocation);
    setPage(Installing, new InstallingPage);
    setPage(InstallComplete, new InstallCompletePage);
    setPage(UninstallConfigure, new UninstallConfigurePage);
    setPage(Uninstalling, new UninstallingPage);
    setPage(UninstallComplete, new UninstallCompletePage);
    setStartId(Intro);
    //setStartId(InstallConfigure);

    if(withMeeGo)
        setWindowTitle(tr("Sentimental Tizen SDK Installer"));
    else
        setWindowTitle(tr("Tizen SDK Installer"));
    setWindowIcon(QIcon(":/images/tizen-sdk-installmanager.png"));
    setMinimumWidth(700);

    gCodec = QTextCodec::codecForLocale();
    qDebug() << tizenPackageIndexUrl;

    mMeeGo = NULL;
    if(withMeeGo) {
        mMeeGo = new MeeGo(200, 200, this);
        mMeeGo->move(0, height()-mMeeGo->height());
        mMeeGo->show();
    }


    mHasInstalledPackages = false;
    // finding installed package index
    loadInstalledPackageList();

#ifdef Q_OS_LINUX
    mPKProxy = NULL;
    mPKTransactionProxy = NULL;
#endif

}

InstallWizard::~InstallWizard()
{
    // remove cache dir;
    QString cacheDir = QDesktopServices::storageLocation(QDesktopServices::TempLocation)+QDir::separator()+"tizensdk";
    qDebug() << "remove " << cacheDir;
    BasePage::removeDir(cacheDir);
}
void InstallWizard::resizeEvent(QResizeEvent *event)
{
    QWizard::resizeEvent(event);
    if(mMeeGo)
        mMeeGo->move(0, height()-mMeeGo->height());
}
void InstallWizard::loadInstalledPackageList()
{
    QFile f(installedPackageIndexPath());
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString data = f.readAll();
        f.close();

        if(!packageIndex()->parse(data, false)) {
            QMessageBox::warning(this, tr("Warnings"),
                tr("Failed to parse installed package index file: %s").arg(installedPackageIndexPath()));
                return;
        }
        mHasInstalledPackages = true;
    }
}

void InstallWizard::openEditor(const QString &path)
{
        QStringList args;
        args << path;
#ifdef Q_WS_MAC
        QProcess::startDetached("open", args);
#endif
#ifdef Q_WS_X11
        QProcess::startDetached("gedit", args);
#endif
#ifdef Q_WS_WIN
        QProcess::startDetached("notepad.exe", args);
#endif
}
void InstallWizard::accept()
{
    if(field("tizeninstaller.showreleasenote").toBool())
        openEditor(field("tizeninstaller.installpath").toString()+QDir::separator()+"RELEASE_NOTE.txt");
    if(field("tizeninstaller.showinstalllog").toBool())
        openEditor(QDesktopServices::storageLocation(QDesktopServices::DataLocation)+QDir::separator()+"install-log.txt");
    if(field("tizeninstaller.showuninstalllog").toBool())
        openEditor(QDesktopServices::storageLocation(QDesktopServices::DataLocation)+QDir::separator()+"uninstall-log.txt");
    QDialog::accept();
}

QString InstallWizard::installedPackageIndexPath()
{
        QString path;
#ifdef Q_OS_LINUX
        QString dirPath =
                QDesktopServices::storageLocation(QDesktopServices::HomeLocation)+QDir::separator()
                +".TizenSDK"+QDir::separator()+"info";
        QDir dir(dirPath);
        if(!dir.exists())
            QDir::root().mkpath(dirPath);
        path =
                QDesktopServices::storageLocation(QDesktopServices::HomeLocation)+QDir::separator()+".TizenSDK"+QDir::separator()+"info"+QDir::separator()+"installedpackage.list";
#else
        path =
                QDesktopServices::storageLocation(QDesktopServices::DataLocation)+QDir::separator()+"InstalledPackages.list";
#endif
        return path;
}
#ifdef Q_OS_LINUX
PKTransaction* InstallWizard::newPKTransaction(bool verbose)
{
    if(!QDBusConnection::systemBus().isConnected()) {
        if(verbose)
            QMessageBox::warning(this, tr("Error"),
                             tr("Could not connect to DBus system bus"));
        return NULL;
    }
    if(!mPKProxy) {
        qDebug() << "connect packagekit";
        int retry = 1;
        do {
            mPKProxy = new QDBusInterface("org.freedesktop.PackageKit",
                             "/org/freedesktop/PackageKit",
                             "org.freedesktop.PackageKit",
                             QDBusConnection::systemBus());
        } while(!mPKProxy->isValid() && retry--);

        if(!mPKProxy->isValid() && verbose)
            QMessageBox::warning(this, tr("Error"),
                             tr("Could not connect to PackageKit"));

        connect(mPKProxy, SIGNAL(destroyed()), this, SLOT(slotPKDestroyed()));
    }

    if(mPKDistroName=="") {
        QStringList distroInfo = mPKProxy->property("DistroId").toString().split(";");
        mPKDistroName = distroInfo.at(0);
        mPKDistroVersion = distroInfo.at(1);
    }

    /*
    QDBusMessage ret1 = mPKProxy->call("CanAuthorize", "org.freedesktop.packagekit.package-install");
    if(ret1.type()!=QDBusMessage::ReplyMessage) {
        QMessageBox::warning(this, tr("Error"),
                     tr("Could not get transaction id from PackageKit"));
        return false;
    }
    qDebug() << ret1.arguments();
    */

    QDBusMessage ret = mPKProxy->call("GetTid");
    if(ret.type()!=QDBusMessage::ReplyMessage) {
        if(verbose)
            QMessageBox::warning(this, tr("Error"),
                     tr("Could not get transaction id from PackageKit"));
        return NULL;
    }
    QString transactionID = ret.arguments().first().toString();
    qDebug() << "new packagekit tid" << transactionID;
    if(!mPKTransactionProxy) {
        delete mPKTransactionProxy;
    }
    mPKTransactionProxy = new PKTransaction("org.freedesktop.PackageKit",
                        transactionID, QDBusConnection::systemBus());
    if(!mPKTransactionProxy->isValid()) {
        if(verbose)
            QMessageBox::warning(this, tr("Error"),
                     tr("Could not create to PackageKit Transaction"));
        return NULL;
    }
    return mPKTransactionProxy;
}
QDBusInterface* InstallWizard::pkProxy()
{
    return mPKProxy;
}
PKTransaction* InstallWizard::pkTransactionProxy()
{
    return mPKTransactionProxy;
}
#endif
void InstallWizard::slotPKDestroyed()
{
#ifdef Q_OS_LINUX
    qDebug() << __func__;
    mPKProxy->deleteLater();
#endif
}

BasePage::BasePage(QWidget *parent)
    : QWizardPage(parent), mCurrentReply(0)
{
    setTitle("title");
    mStep = 1;
    mBannerType = InstallerBannerIntro;
    mSaveMode = false;
    mHashCheck = false;
    mHash = new QCryptographicHash(QCryptographicHash::Md5);
    mProcess = NULL;
}
TizenPackageIndex* BasePage::packageIndex()
{
    return ((InstallWizard*)wizard())->packageIndex();
}
void BasePage::setBannerType(InstallerBannerType bannerType, int step)
{
    mBannerType = bannerType;
    mStep = step;
}
QPixmap BasePage::banner()
{
    //qDebug() << mBannerType;
    if(mBannerType==InstallerBannerInstall) {
        QPixmap pixmap = QPixmap(QString(":/images/install_manager_graphicmotif_00%1.png").arg(mStep));
        return pixmap;
    } else if(mBannerType==InstallerBannerUninstall) {
        QPixmap pixmap = QPixmap(QString(":/images/uninstaller_graphicmotif_00%1.png").arg(mStep));
        return pixmap;
    }
    QPixmap pixmap = QPixmap(":/images/install_manager_graphicmotif_001.png");
    /*
    QPainter p(&pixmap);
    p.fillRect(0, 270, 150, 200, Qt::black);
    */
    return pixmap;
}
void BasePage::setBanner()
{
#ifdef Q_WS_MAC
    setPixmap(QWizard::BackgroundPixmap, banner());
#else
    setPixmap(QWizard::WatermarkPixmap, banner());
#endif
}
QStandardItemModel* BasePage::createPackageItemModel()
{
    QStandardItemModel *model = new QStandardItemModel();
    model->setHeaderData(0, Qt::Horizontal, tr("Name"), Qt::DisplayRole);
    model->setColumnCount(4);
    model->setHorizontalHeaderItem(0, new QStandardItem(tr("Name")));
    model->setHorizontalHeaderItem(1, new QStandardItem(tr("Version")));
    model->setHorizontalHeaderItem(2, new QStandardItem(tr("Status")));
    model->setHorizontalHeaderItem(3, new QStandardItem(tr("Size")));
    for( int i=0; i<packageIndex()->count(); i++ ) {
        TizenPackage *package = packageIndex()->at(i);
        if(package->attribute()=="root") {
            QStandardItem *item = new QStandardItem(package->name());
            item->setCheckable(true);
            item->setCheckState(Qt::Checked);
            item->setColumnCount(3);
            item->setToolTip(package->description());
            item->setData("tizen");
            QList<QStandardItem*> columns;
            columns << item;
            columns << new QStandardItem(package->version());
            columns << new QStandardItem(package->statusString());
            columns << new QStandardItem(TizenPackageIndex::aboutSize(package->totalSize()));
            for(int k=0; k<columns.count(); k++)
                columns.at(k)->setEditable(false);
            model->appendRow(columns);
            /* Making sub tree items
            XXX Package List from Tizen repository server is little bit strange.
            e.g 'usb-connection-for-ssh' depends on 'tizen-ide'. I think that
            'tizen-ide' have to depends on the 'usb-connection-for-ssh'. And the
            root package item which depends on 'usb-connection-for-ssh'
            is also depends on 'tizen-ide'. Since it looks lack consistency for
            the dependencies between packages, we may can skip making multi-depth
            sub items.
            */
            for(int j=0; j<package->count(); j++) {
                TizenPackage *subPackage = package->at(j);
                QStandardItem *subitem = new QStandardItem(subPackage->name());
                subitem->setCheckable(true);
                subitem->setCheckState(Qt::Checked);
                subitem->setEnabled(false);
                subitem->setToolTip(subPackage->description());
                subitem->setData("tizen");
                QList<QStandardItem*> columns;
                columns << subitem;
                columns << new QStandardItem(subPackage->version());
                columns << new QStandardItem(subPackage->statusString());
                columns << new QStandardItem(TizenPackageIndex::aboutSize((uint64_t)subPackage->size()));
                for(int k=0; k<columns.count(); k++)
                    columns.at(k)->setEditable(false);
                item->appendRow(columns);
            }
        }
    }
    return model;
}
void BasePage::setupNetworkAccessManager()
{
    mAccessManager = new QNetworkAccessManager(this);
    qDebug() <<"proxy:"<< field("tizeninstaller.network.proxy");
    qDebug() <<"host:"<< field("tizeninstaller.network.proxy-host");
    qDebug() <<"port:"<< field("tizeninstaller.network.proxy-port");
    qDebug() <<"user:"<< field("tizeninstaller.network.proxy-user");
    qDebug() <<"passwd:"<< field("tizeninstaller.network.proxy-passwd");
    QNetworkProxy::ProxyType proxyType = QNetworkProxy::NoProxy;
    int p = field("tizeninstaller.network.proxy").toInt();
    if(p == 1)
        proxyType = QNetworkProxy::HttpProxy;
    else if(p == 2)
        proxyType = QNetworkProxy::Socks5Proxy;
    QNetworkProxy networkProxy(proxyType,
                    field("tizeninstaller.network.proxy-host").toString().trimmed(),
                    field("tizeninstaller.network.proxy-port").toInt(),
                    field("tizeninstaller.network.proxy-user").toString().trimmed(),
                    field("tizeninstaller.network.proxy-passwd").toString().trimmed());
    mAccessManager->setProxy(networkProxy);
}
void BasePage::cancelDownload()
{
    if(mCurrentReply) {
        mCurrentReply->close();
        delete mCurrentReply;
        mCurrentReply = NULL;
    }
}
void BasePage::downloadText(const QUrl &url)
{
    if(!mAccessManager) {
        qDebug() << "Please call :setupNetworkAccessManager() before you call this function";
        return;
    }
    if(mCurrentReply) {
        mCurrentReply->close();
        delete mCurrentReply;
    }
    QNetworkRequest request(url);
    mCurrentReply = mAccessManager->get(request);
    connect(mCurrentReply, SIGNAL(readyRead()), SLOT(slotDownloadReadyRead()));
    connect(mCurrentReply, SIGNAL(downloadProgress(qint64,qint64)), SLOT(slotDownloadProgress(qint64,qint64)));
    connect(mCurrentReply, SIGNAL(error(QNetworkReply::NetworkError)), SLOT(slotDownloadError(QNetworkReply::NetworkError)));
    connect(mCurrentReply, SIGNAL(finished()), SLOT(slotDownloadFinished()));
    mSaveMode = false;
    mData.clear();
}
void BasePage::downloadData(const QUrl &url, const QDir &savedir)
{
    if(!mAccessManager) {
        qDebug() << "Please call :setupNetworkAccessManager() before you call this function";
        return;
    }
    if(mCurrentReply)
        mCurrentReply->close();
    QFileInfo fi(url.path());
    mTarget.setFileName(savedir.absolutePath()+QDir::separator()+fi.fileName());
    if(!mTarget.open(QIODevice::WriteOnly)) {
        downloadError(QString("Could not save into %1").arg(savedir.absolutePath()));
        return;
    }
    mHash->reset();
    QNetworkRequest request(url);
    mCurrentReply = mAccessManager->get(request);
    connect(mCurrentReply, SIGNAL(readyRead()), SLOT(slotDownloadReadyRead()));
    connect(mCurrentReply, SIGNAL(downloadProgress(qint64,qint64)), SLOT(slotDownloadProgress(qint64,qint64)));
    connect(mCurrentReply, SIGNAL(error(QNetworkReply::NetworkError)), SLOT(slotDownloadError(QNetworkReply::NetworkError)));
    connect(mCurrentReply, SIGNAL(finished()), SLOT(slotDownloadFinished()));
    mSaveMode = true;
}
void BasePage::slotDownloadReadyRead()
{
    if(mSaveMode) {
        QByteArray ba = mCurrentReply->read(mCurrentReply->bytesAvailable());
        mTarget.write(ba);
        if(mHashCheck)
            mHash->addData(ba);
    }
    else
        mData.append(mCurrentReply->read(mCurrentReply->bytesAvailable()));
}
QString BasePage::hashCode()
{
    return mHash->result().toHex();
}
void BasePage::slotDownloadError(QNetworkReply::NetworkError errorCode)
{
    qDebug() << "error" << errorCode << mCurrentReply->errorString();
    downloadError(mCurrentReply->errorString());
}
void BasePage::slotDownloadFinished()
{
    if(mSaveMode) {
        mTarget.close();
        downloadFinished(mCurrentReply);
    } else
        downloadFinished(mCurrentReply, mData);
}
void BasePage::addLog(const QString &str, bool newline, const QString &color)
{
    (void)str;
    (void)newline;
    (void)color;
    qDebug() << str;
}
void BasePage::downloadFinished(const QNetworkReply *reply, const QByteArray &data)
{
    (void)reply;
    (void)data;
}
void BasePage::downloadFinished(const QNetworkReply *reply)
{
    (void)reply;
}
void BasePage::downloadError(const QString &errorStr)
{
    (void)errorStr;
}
void BasePage::downloadProgress(qint64 byteReceieved, qint64 byteTotal)
{
    (void)byteReceieved;
    (void)byteTotal;
}

void BasePage::slotDownloadProgress(qint64 byteReceieved, qint64 byteTotal)
{
    //qDebug() << "downloading: " << byteReceieved << "/" << byteTotal;
    downloadProgress(byteReceieved, byteTotal);
}
bool BasePage::createTempInTargetDir()
{
    QString targetDirPath = field("tizeninstaller.installpath").toString();
    return createDir(targetDirPath+QDir::separator()+"temp");
}
void BasePage::removeTempInTargetDir()
{
    QString targetDirPath = field("tizeninstaller.installpath").toString();
    removeDir(targetDirPath+QDir::separator()+"temp");
}
bool BasePage::createDir(const QString &path)
{
    QDir dir(path);
    if(!dir.exists()) {
        if(QDir::root().mkpath(path))
            addLog(QString("mkdir %1").arg(path));
        else {
            QMessageBox::warning(this, tr("Error"), tr("Could not create the target directory: %1").arg(path));
            return false;
        }
    }
    return true;
}
bool BasePage::removeDir(const QString &dirName)
{
    bool result = true;
    QDir dir(dirName);

    if (dir.exists(dirName)) {
        Q_FOREACH(QFileInfo info, dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden  | QDir::AllDirs | QDir::Files, QDir::DirsFirst)) {
            if (info.isDir()) {
                result = removeDir(info.absoluteFilePath());
            }
            else {
                result = QFile::remove(info.absoluteFilePath());
            }

            if (!result) {
                return result;
            }
        }
        result = dir.rmdir(dirName);
    }

    return result;
}


void BasePage::copyScriptFromResource(const QString &resourcePath, const QString &targetDir)
{
    QFile file(resourcePath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QFileInfo fi(resourcePath);
        QFile script(targetDir+QDir::separator()+fi.fileName());
        script.open(QIODevice::WriteOnly);
        script.setPermissions((QFile::Permission)0x7755);
        script.write(file.readAll());
        script.close();
        file.close();
    }
}
void BasePage::copyScriptFromResource(const QString &resourcePath)
{
    QString targetDirPath = field("tizeninstaller.installpath").toString()+QDir::separator()+"temp";
    copyScriptFromResource(resourcePath, targetDirPath);
}
void BasePage::shell(const QString &cmd)
{
    QString targetDirPath = field("tizeninstaller.installpath").toString()+QDir::separator()+"temp";
    shell(cmd, targetDirPath);
}
void BasePage::shell(const QString &cmd, const QString &currentWorkingDir)
{
    QStringList sysEnv = QProcess::systemEnvironment();
    QString targetDirPath = field("tizeninstaller.installpath").toString();
    sysEnv << QString("INSTALLED_PATH=%1").arg(targetDirPath);
    sysEnv << QString("MAKESHORTCUT_PATH=%1").arg(currentWorkingDir+QDir::separator()+"makeshortcut.sh");
    sysEnv << QString("REMOVESHORTCUT_PATH=%1").arg(currentWorkingDir+QDir::separator()+"removeshortcut.sh");
    if(mProcess) {
        mProcess->close();
        delete mProcess;
    }
    mProcess = new QProcess(this);
    mProcess->setEnvironment(sysEnv);
    mProcess->setWorkingDirectory(currentWorkingDir);
    connect(mProcess, SIGNAL(readyReadStandardError ()), this, SLOT(slotStandardError()));
    connect(mProcess, SIGNAL(readyReadStandardOutput ()), this, SLOT(slotStandardOutput()));
    connect(mProcess, SIGNAL(error(QProcess::ProcessError)), this, SLOT(slotError(QProcess::ProcessError)));
    connect(mProcess, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(slotProcessFinished(int, QProcess::ExitStatus)));
    connect(mProcess, SIGNAL(started()), this, SLOT(slotStarted()));
    connect(mProcess, SIGNAL(stateChanged(QProcess::ProcessState)), this, SLOT(slotStateChanged(QProcess::ProcessState)));
    qDebug() << QString("INSTALLED_PATH=%1").arg(targetDirPath);
    qDebug() << QString("MAKESHORTCUT_PATH=%1").arg(currentWorkingDir+QDir::separator()+"makeshortcut.sh");
    qDebug() << QString("REMOVESHORTCUT_PATH=%1").arg(currentWorkingDir+QDir::separator()+"removeshortcut.sh");
    qDebug() << "cd" << currentWorkingDir;
    qDebug() << "cmd" << cmd;
    mProcess->start(cmd);
}
void BasePage::slotStarted()
{
    qDebug() << "started" << mProcess->pid();
}
void BasePage::slotStateChanged(QProcess::ProcessState state)
{
    qDebug() << "stateChagned" << state;
}
void BasePage::processError(QProcess::ProcessError error)
{
    qDebug() << "error" << error;
}
void BasePage::slotError(QProcess::ProcessError error)
{
    processError(error);
    processFinished(-1, QProcess::CrashExit);
}
void BasePage::slotStandardOutput()
{
    QByteArray ba = mProcess->readAllStandardOutput();
    if(ba.length()>0)
        processStandardOutput(ba);
}
void BasePage::slotStandardError()
{
    QByteArray ba = mProcess->readAllStandardError();
    if(ba.length()>0)
        processStandardError(ba);
}
void BasePage::processStandardOutput(const QByteArray &out)
{
    qDebug() << out;
}
void BasePage::processStandardError(const QByteArray &out)
{
    qDebug() << out;
}
void BasePage::processFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    (void)exitCode;
    (void)exitStatus;
}
void BasePage::slotProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    disconnect(mProcess, SIGNAL(readyReadStandardError ()), this, SLOT(slotStandardError()));
    disconnect(mProcess, SIGNAL(readyReadStandardOutput ()), this, SLOT(slotStandardOutput()));
    disconnect(mProcess, SIGNAL(error(QProcess::ProcessError)), this, SLOT(slotError(QProcess::ProcessError)));
    disconnect(mProcess, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(slotProcessFinished(int, QProcess::ExitStatus)));
    disconnect(mProcess, SIGNAL(started()), this, SLOT(slotStarted()));
    disconnect(mProcess, SIGNAL(stateChanged(QProcess::ProcessState)), this, SLOT(slotStateChanged(QProcess::ProcessState)));
    processFinished(exitCode, exitStatus);
}


IntroPage::IntroPage(QWidget *parent)
    : BasePage(parent)
{
    setTitle(tr("Welcome to Tizen SDK"));
    setBanner();
    QLabel *label = new QLabel(tr("Tizen SDK will be installed on your computer. "
                          "It is recommended that you close all other applications "
                          "before starting installation."));
    label->setWordWrap(true);
    mButtonInstall = new QRadioButton(tr("Install or Update Tizen SDK"));
    mButtonInstall->setChecked(true);
    mButtonUninstall = new QRadioButton(tr("Remove Tizen SDK"));
    registerField("tizeninstaller.mode.install", mButtonInstall);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(label);
    layout->addWidget(mButtonInstall);
    layout->addWidget(mButtonUninstall);
    setLayout(layout);
}
void IntroPage::initializePage()
{
    if(!((InstallWizard*)wizard())->hasInstalledPackageList())
        mButtonUninstall->setEnabled(false);
}

int IntroPage::nextId() const
{
    if(mButtonInstall->isChecked())
        return InstallWizard::InstallLicense;
    else
        return InstallWizard::UninstallConfigure;
}
InstallLicensePage::InstallLicensePage(QWidget *parent)
    : BasePage(parent)
{
    setTitle(tr("License Agreement"));
    setBannerType(InstallerBannerInstall, 1);
    setBanner();
    QLabel *label = new QLabel(tr("To continue you must accept the terms of this agreement."));
    label->setWordWrap(true);
    QTextEdit *textEdit = new QTextEdit();
    textEdit->setReadOnly(true);

    QFile file(":/etc/COPYING");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QTextStream in(&file);
    in.setCodec(gCodec);
    textEdit->setText(in.readAll());

    mAgreement = new QCheckBox(tr("I accept the terms of this agreement."));

    registerField("tizeninstaller.license.agreement*", mAgreement);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(label);
    layout->addWidget(textEdit);
    layout->addWidget(mAgreement);
    setLayout(layout);
}
InstallConfigurePageNetwork::InstallConfigurePageNetwork(QWidget *parent)
    : BasePage(parent)
{
    setTitle(tr("Connection Settings"));
    setBannerType(InstallerBannerInstall, 2);
    setBanner();

    QLabel *label = new QLabel(tr("Configure how SDK installer connects to Internet"));
    label->setWordWrap(true);

    mProxyMode = new QComboBox(this);
    mProxyMode->addItem(tr("No proxy"));
    mProxyMode->addItem(tr("HTTP Proxy"));
    mProxyMode->addItem(tr("Socks5 Proxy"));
    connect(mProxyMode, SIGNAL(activated(int)), SLOT(slotProxyMode(int)));
    registerField("tizeninstaller.network.proxy", mProxyMode);

    mProxyModeLabel = new QLabel(tr("Host:"), this);
    mProxyHost = new QLineEdit(this);
    connect(mProxyHost, SIGNAL(textChanged(QString)), this, SIGNAL(completeChanged()));
    mProxyPort = new QSpinBox(this);
    mProxyPort->setMinimum(22);
    mProxyPort->setMaximum(9999);
    mProxyPort->setValue(8080);

    mProxyAuthLabel = new QLabel(tr("Authentication (Optional)"), this);
    mProxyAuthUserLabel = new QLabel(tr("Username:"), this);
    mProxyAuthUser = new QLineEdit(this);
    mProxyAuthPasswdLabel = new QLabel(tr("Password:"), this);
    mProxyAuthPasswd = new QLineEdit(this);
    mProxyAuthPasswd->setEchoMode(QLineEdit::Password);
    registerField("tizeninstaller.network.proxy-host", mProxyHost);
    registerField("tizeninstaller.network.proxy-port", mProxyPort);
    mProxyModeLabel->hide();
    mProxyHost->hide();
    mProxyPort->hide();
    mProxyAuthLabel->hide();
    mProxyAuthUser->hide();
    mProxyAuthPasswd->hide();
    mProxyAuthUserLabel->hide();
    mProxyAuthPasswdLabel->hide();
    registerField("tizeninstaller.network.proxy-user", mProxyAuthUser);
    registerField("tizeninstaller.network.proxy-passwd", mProxyAuthPasswd);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(label);
    layout->addWidget(mProxyMode);
    QHBoxLayout *hlayout = new QHBoxLayout;
    hlayout->addWidget(mProxyModeLabel);
    hlayout->addWidget(mProxyHost);
    hlayout->addWidget(mProxyPort);
    layout->addLayout(hlayout);
    layout->addWidget(mProxyAuthLabel);

    QGridLayout *grid = new QGridLayout;
    grid->addWidget(mProxyAuthUserLabel, 0, 0);
    grid->addWidget(mProxyAuthUser, 0, 1);
    grid->addWidget(mProxyAuthPasswdLabel, 1, 0);
    grid->addWidget(mProxyAuthPasswd, 1, 1);
    layout->addLayout(grid);
    setLayout(layout);
}
bool InstallConfigurePageNetwork::isComplete() const
{
    if(mProxyMode->currentIndex()>0 && mProxyHost->text().trimmed()=="")
        return false;
    return true;
}
void InstallConfigurePageNetwork::slotProxyMode(int mode)
{
    if(mode==0) {
        mProxyModeLabel->hide();
        mProxyModeLabel->setText("127.0.0.1");
        mProxyHost->hide();
        mProxyPort->hide();
        mProxyAuthLabel->hide();
        mProxyAuthUser->hide();
        mProxyAuthPasswd->hide();
        mProxyAuthUser->setText("");
        mProxyAuthPasswd->setText("");
        mProxyAuthUserLabel->hide();
        mProxyAuthPasswdLabel->hide();
    } else {
        mProxyModeLabel->show();
        mProxyHost->show();
        mProxyPort->show();
        mProxyAuthLabel->show();
        mProxyAuthUser->show();
        mProxyAuthPasswd->show();
        mProxyAuthUserLabel->show();
        mProxyAuthPasswdLabel->show();
        if(mode==1)
            mProxyPort->setValue(8080);
        else
            mProxyPort->setValue(1080);
    }
    emit completeChanged();
}
InstallConfigurePage::InstallConfigurePage(QWidget *parent)
    : BasePage(parent), mPackageListDownloaded(false)
{
    setTitle(tr("Installation Packages"));
    setBannerType(InstallerBannerInstall, 2);
    setBanner();
    QLabel *label = new QLabel(tr("Customize the items that you wish to install."));
    label->setWordWrap(true);

    mTreeView = new QTreeView(this);
    mModel = NULL;

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(label);
    layout->addWidget(mTreeView);
    setLayout(layout);
#ifdef Q_OS_LINUX
    mDistPackagesItem = NULL;
#endif

}
void InstallConfigurePage::initializePage()
{
    if(mModel)
        return;
    setupNetworkAccessManager();
    downloadText(((InstallWizard*)wizard())->packageListURL());
}
bool InstallConfigurePage::isComplete() const
{
    if(!mPackageListDownloaded)
        return false;
    if(mModel->rowCount() > 0) {
        for(int i=0; i<mModel->rowCount(); i++) {
            QStandardItem *item = mModel->item(i);
            TizenPackage *package = ((InstallWizard*)wizard())->packageIndex()->find(item->text());
            if(item->checkState()==Qt::Checked &&
                   (package->status()==TizenPackage::NewPackage
                ||  package->status()==TizenPackage::UpgradePackage)) {
                return true;
            }
            for(int j=0; j<item->rowCount(); j++) {
                QStandardItem *subItem = item->child(j);
                TizenPackage *subPackage = ((InstallWizard*)wizard())->packageIndex()->find(subItem->text());
                if(subItem->checkState()==Qt::Checked &&
                       (subPackage->status()==TizenPackage::NewPackage
                    ||  subPackage->status()==TizenPackage::UpgradePackage)) {
                    return true;
                }
            }
        }
    }
    return false;
}
QStringList InstallConfigurePage::packageNames()
{
    QStringList packageList;
    if(mModel->rowCount() > 0) {
        for(int i=0; i<mModel->rowCount(); i++) {
            QStandardItem *item = mModel->item(i);
            if(item->data().toString()!="tizen")
                continue;
            TizenPackage *package = packageIndex()->find(item->text());
            if(item->checkState()==Qt::Checked) {
                for(int j=0; j<item->rowCount(); j++) {
                    QStandardItem *subItem = item->child(j);
                    TizenPackage *subPackage = packageIndex()->find(subItem->text());
                    if(subItem->checkState()==Qt::Checked &&
                           (subPackage->status()==TizenPackage::NewPackage
                        ||  subPackage->status()==TizenPackage::UpgradePackage)) {
                        packageList << subItem->text();
                    }
                }
                if((package->status()==TizenPackage::NewPackage
                    ||  package->status()==TizenPackage::UpgradePackage))
                    packageList << item->text();
            }
        }
    }
    return packageList;
}
void InstallConfigurePage::downloadError(const QString &errorStr)
{
    QMessageBox::warning(this, tr("Network error"), errorStr);
}
void InstallConfigurePage::downloadFinished(const QNetworkReply *reply, const QByteArray &data)
{
    if(200!=reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()) {
        if(QMessageBox::No == QMessageBox::warning(this, tr("Download failed"),
            tr("Failed to retrieve the package index file from repository server. Retry?"),
            QMessageBox::Yes|QMessageBox::No, QMessageBox::No))
            return;
        downloadText(((InstallWizard*)wizard())->packageListURL());
        return;
    }
    qDebug() << data;

    if(!packageIndex()->parse(data, true)) {
        QMessageBox::warning(this, tr("Warnings"),
            tr("Failed to parse the package index file from repository server."));
            return;
    }

    mModel = createPackageItemModel();
    mTreeView->setModel(mModel);
    mTreeView->header()->resizeSections(QHeaderView::ResizeToContents);
    mPackageListDownloaded = true;
    connect(mModel, SIGNAL(itemChanged(QStandardItem*)), this, SLOT(slotItemChanged(QStandardItem*)));

#ifdef Q_OS_LINUX
    if(((InstallWizard*)wizard())->usePackageKit())
        QTimer::singleShot(0, this, SLOT(slotPKInitTransaction()));
    else
#endif
        emit completeChanged();
}
void InstallConfigurePage::slotPKInitTransaction()
{
#ifdef Q_OS_LINUX
    PKTransaction *pkTransactionProxy = NULL;
    if(!(pkTransactionProxy = ((InstallWizard*)wizard())->newPKTransaction(true))) {
        emit completeChanged();
        return;
    }
    QStringList packages;
    QString distroName = ((InstallWizard*)wizard())->distroName();
    QString distroVersion = ((InstallWizard*)wizard())->distroVersion();
    QString dependenciesInfo = QString(":/dependencies/%1-%2.pkgs")
                                          .arg(distroName).arg(distroVersion);
    if(!QFile::exists(dependenciesInfo)) {
        dependenciesInfo = QString(":/dependencies/%1.pkgs").arg(distroName);
        if(!QFile::exists(dependenciesInfo)) {
            dependenciesInfo = QString(":/dependencies/common.pkgs");
        }
    }
    QFile f(dependenciesInfo);
    f.open(QIODevice::ReadOnly|QIODevice::Text);
    QString data = f.readAll();
    QStringList dependencies = data.split('\n');
    f.close();
    for(int i=0; i<dependencies.count(); i++) {
        QString pkgName = dependencies.at(i).trimmed();
        if(pkgName.length()==0)
            continue;
        qDebug() << "dpends pkg" << pkgName << "for" << distroName;
        packages << pkgName;
    }

    QStandardItem *item = new QStandardItem(tr("%1 package dependencies").arg(distroName.toUpper()));
    item->setCheckable(true);
    item->setCheckState(Qt::Checked);
    item->setColumnCount(3);
    item->setToolTip(tr("The %1 packages for Tizen SDK").arg(distroName));
    item->setData("packagekit");
    QList<QStandardItem*> columns;
    columns << item;
    columns << new QStandardItem(distroVersion);
    columns << new QStandardItem("");
    columns << new QStandardItem("");
    for(int k=0; k<columns.count(); k++)
        columns.at(k)->setEditable(false);
    mModel->appendRow(columns);
    mDistPackagesItem = item;;
    connect(pkTransactionProxy,
            SIGNAL(Package(const QString&, const QString&, const QString&)),
            this,
            SLOT(slotPKSignalPackage(const QString&, const QString&, const QString&)));
    connect(pkTransactionProxy,
            SIGNAL(ErrorCode(const QString&, const QString&)),
            this,
            SLOT(slotPKSignalErrorCode(const QString&, const QString&)));
    connect(pkTransactionProxy,
            SIGNAL(Finished(const QString&,uint)),
            this,
            SLOT(slotPKSignalFinished(const QString,uint)));
    mCurrentPKTransaction = "resolve";
    pkTransactionProxy->Resolve("none", packages);
    qDebug() << "resolve";
#endif
}
#ifdef Q_OS_LINUX
QStringList InstallConfigurePage::packageIDs()
{
    QStringList packageIDs;
    if(mDistPackagesItem) {
        for(int i=0; i<mDistPackagesItem->rowCount(); i++) {
            QStandardItem *item = mDistPackagesItem->child(i, 0);
            if(item->checkState()==Qt::Checked &&
                    item->data(Qt::UserRole+2).toString()=="available") {
                QString packageID = item->data().toString().trimmed();
                packageIDs << packageID;
            }
        }
    }
    return packageIDs;
}
#endif
void InstallConfigurePage::slotPKSignalPackage(const QString &info, const QString &packageId, const QString &summary)
{
#ifdef Q_OS_LINUX
    qDebug() << info << packageId << summary;
    if(mCurrentPKTransaction=="resolve") {
        QStringList packageInfo = packageId.split(";");
        QString name = packageInfo.at(0);
        QString version = packageInfo.at(1);
        QString arch = packageInfo.at(2);
        qDebug() << info << name << version << arch;
        QList<QStandardItem*> founds = mModel->findItems(name);
        if(founds.count() > 0) {
            QStandardItem *item = founds.at(0);
            QStandardItem *versionItem = item->model()->item(item->row(), 1);
            versionItem->setText(version);
            QStandardItem *statusItem = item->model()->item(item->row(), 2);
            if(info=="available") {
                item->setData(packageId);
                item->setData(info, Qt::UserRole+2);
                statusItem->setText(tr("Upgrade"));
            }
            return;
        }
        QStandardItem *item = new QStandardItem(name);
        item->setIcon(QIcon::fromTheme("package-x-generic"));
        item->setCheckable(true);
        item->setCheckState(Qt::Checked);
        item->setColumnCount(3);
        item->setToolTip(summary);
        item->setData(packageId);
        item->setData(info, Qt::UserRole+2);
        mPackageIdList << packageId;

        QList<QStandardItem*> columns;
        columns << item;
        columns << new QStandardItem(version);
        if(info=="installed")
            columns << new QStandardItem(tr("Installed"));
        else
            columns << new QStandardItem(tr("New"));
        columns << new QStandardItem(tr("Unknown"));
        for(int i=0; i<columns.count(); i++)
            columns.at(i)->setEditable(false);
        mDistPackagesItem->appendRow(columns);
        //mTreeView->header()->resizeSections(QHeaderView::ResizeToContents);
    }
#endif
}
void InstallConfigurePage::slotPKSignalDetails(const QString &package_id,
                                                const QString &license,
                                                const QString &group,
                                                const QString &detail,
                                                const QString &url,
                                                qulonglong size)
{
#ifdef Q_OS_LINUX
    (void)license;
    (void)group;
    (void)detail;
    (void)url;
    //qDebug() << "detail" << package_id << license << group << detail << url << size;
    for(int i=0; i<mDistPackagesItem->rowCount(); i++) {
        QStandardItem *item = mDistPackagesItem->child(i, 0);
        if(item->data().toString()==package_id) {
            QStandardItem *sizeItem = mDistPackagesItem->child(i, 3);
            sizeItem->setText(TizenPackageIndex::aboutSize((uint64_t)size));
            break;
        }
    }
#endif
}
void InstallConfigurePage::slotPKSignalFinished(const QString &exit, uint runtime)
{
#ifdef Q_OS_LINUX
    qDebug() << "finished" << mCurrentPKTransaction << exit << runtime;
    if(mCurrentPKTransaction == "resolve") {
        mTreeView->expand(mDistPackagesItem->index());
        PKTransaction *pkTransactionProxy = NULL;
        if(!(pkTransactionProxy = ((InstallWizard*)wizard())->newPKTransaction(true)))
            return;
        connect(pkTransactionProxy,
                SIGNAL(Details(const QString &, const QString &, const QString &, const QString &, const QString &, qulonglong )),
                this,
                SLOT(slotPKSignalDetails(const QString &, const QString &, const QString &, const QString &, const QString &, qulonglong )));
        connect(pkTransactionProxy,
                SIGNAL(Finished(const QString&,uint)),
                this,
                SLOT(slotPKSignalFinished(const QString,uint)));
        connect(pkTransactionProxy,
                SIGNAL(ErrorCode(const QString&, const QString&)),
                this,
                SLOT(slotPKSignalErrorCode(const QString&, const QString&)));
        mCurrentPKTransaction = "details";
        qDebug() << "getDetails";
        pkTransactionProxy->GetDetails(mPackageIdList);
    } else if(mCurrentPKTransaction == "details")
        emit completeChanged();
#endif
}
void InstallConfigurePage::slotPKSignalErrorCode(const QString &code, const QString &details)
{
#ifdef Q_OS_LINUX
    qDebug() << "errorcode" << code << details;
#endif
}
void InstallConfigurePage::slotItemChanged(QStandardItem *item)
{
    if(item->rowCount() > 0) {
        for(int i=0; i<item->rowCount(); i++) {
            item->child(i)->setCheckState(item->checkState());
        }
        emit completeChanged();
    }
}
InstallConfigurePageLocation::InstallConfigurePageLocation(QWidget *parent)
    : BasePage(parent)
{
    setTitle(tr("Location"));
    setBannerType(InstallerBannerInstall, 2);
    setBanner();

    QLabel *label = new QLabel(tr("Choose a location where you want to install."));
    label->setWordWrap(true);
    mPath = new QLineEdit();
    QPushButton *button = new QPushButton("...", this);
    button->setFocusPolicy(Qt::NoFocus);
    connect(button, SIGNAL(clicked()), this, SLOT(slotOpenDir()));
    mPath->setText(QDir::homePath()+QDir::separator()+"tizen_sdk");
    mPath->setReadOnly(true);
    mRequiredSpaceLabel = new QLabel(this);
    mAvailableSpaceLabel = new QLabel(this);
    registerField("tizeninstaller.installpath", mPath);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(label);
    QHBoxLayout *hlayout = new QHBoxLayout;
    hlayout->addWidget(mPath);
    hlayout->addWidget(button);
    layout->addLayout(hlayout);
    layout->addStretch();
    layout->addWidget(mRequiredSpaceLabel);
    layout->addWidget(mAvailableSpaceLabel);
    setLayout(layout);
}
void InstallConfigurePageLocation::initializePage()
{
    long total = 0;
    QStringList packageNames = ((InstallConfigurePage*)wizard()->page(InstallWizard::InstallConfigure))->packageNames();
    qDebug() << "initialized" << packageNames;
    for( int i=0; i<packageIndex()->count(); i++ ) {
        TizenPackage *package = packageIndex()->at(i);
        if(packageNames.contains(package->name()))
            total += package->size();
    }
    mRequiredSpaceLabel->setText(tr("Space required: %1").arg(TizenPackageIndex::aboutSize((uint64_t)total)));
    QDir dir(mPath->text());
    if(!dir.exists())
        dir.cdUp();
    struct statfs buf;
    int r = statfs(dir.absolutePath().toLocal8Bit().constData(), &buf);
    qDebug() << "f_bavail" << buf.f_bavail;
    qDebug() << "f_bsize" << buf.f_bsize;
    qDebug() << "freespace" << (long)buf.f_bavail*(long)buf.f_bsize;
    mSpaceIsNotEnough = true;
    if(r==0) {
        mAvailableSpaceLabel->setText(tr("Space available: %1").arg(TizenPackageIndex::aboutSize((uint64_t)buf.f_bavail * (uint64_t)buf.f_bsize)));
        if(((uint64_t)buf.f_bavail * (uint64_t)buf.f_bsize)>=(uint64_t)total)
            mSpaceIsNotEnough =  false;
    }
    else
        mAvailableSpaceLabel->setText(tr("Space available: %1").arg("Unknown"));
    emit completeChanged();
    setCommitPage(true);
    setButtonText(QWizard::CommitButton, tr("Install SDK"));

    //mAvailableSpaceLabel
}
bool InstallConfigurePageLocation::isComplete() const
{
    return !mSpaceIsNotEnough;
}
void InstallConfigurePageLocation::slotOpenDir()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select a directory"), QDir::homePath());
    if(!dir.isNull())
        mPath->setText(dir);
}
InstallingPage::InstallingPage(QWidget *parent)
    : BasePage(parent)
{
    setTitle(tr("Installation Progress"));
    setBannerType(InstallerBannerInstall, 3);
    setBanner();
    QLabel *label = new QLabel(tr("Please wait while the Tizen SDK installation is being processed"));
    label->setWordWrap(true);

    mProgressBar = new QProgressBar;
    mCurrentStatus = new QLabel(tr("Preparing ..."));
    mCurrentReceived = new QLabel;
    mCurrentRemainTime = new QLabel;
    mCurrentSpeed = new QLabel;
    mCurrentPackage = NULL;
    mInstallLog = new QTextEdit;
    mInstallLog->setReadOnly(true);
    mQuaZip = NULL;
    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(label);
    layout->addWidget(mCurrentStatus);
    QHBoxLayout *hlayout = new QHBoxLayout;
    hlayout->addStretch();
    hlayout->addWidget(mCurrentReceived);
    layout->addLayout(hlayout);
    layout->addWidget(mProgressBar);
    hlayout = new QHBoxLayout;
    hlayout->addWidget(mCurrentRemainTime);
    hlayout->addStretch();
    hlayout->addWidget(mCurrentSpeed);
    layout->addLayout(hlayout);
    layout->addWidget(mInstallLog);
    setLayout(layout);
    mInstalledPackageIndex = NULL;
    connect(&mTimer, SIGNAL(timeout()), this, SLOT(slotDisplayDownloadSpeed()));
}
InstallingPage::~InstallingPage()
{
    if(mLogFile.isOpen())
        mLogFile.close();
}
void InstallingPage::initializePage()
{
    setupNetworkAccessManager();

#ifdef Q_OS_LINUX
    QStringList packageIDs =
        ((InstallConfigurePage*)wizard()->page(InstallWizard::InstallConfigure))
        ->packageIDs();
    if(packageIDs.count()>0)
        QTimer::singleShot(0, this, SLOT(slotStartInstallPKPackages()));
    else
#endif
        QTimer::singleShot(0, this, SLOT(slotStartDownload()));
}
void InstallingPage::slotStartInstallPKPackages()
{
#ifdef Q_OS_LINUX
    PKTransaction *pkTransactionProxy = NULL;
    if(!(pkTransactionProxy = ((InstallWizard*)wizard())->newPKTransaction(true))) {
        QTimer::singleShot(0, this, SLOT(slotStartDownload()));
        return;
    }
    connect(pkTransactionProxy,
            SIGNAL(Changed()),
            this,
            SLOT(slotPKSignalChanged()));
    connect(pkTransactionProxy,
            SIGNAL(ErrorCode(const QString&, const QString&)),
            this,
            SLOT(slotPKSignalErrorCode(const QString&, const QString&)));
    connect(pkTransactionProxy,
            SIGNAL(Finished(const QString&,uint)),
            this,
            SLOT(slotPKSignalFinished(const QString,uint)));
    QStringList packageIDs =
        ((InstallConfigurePage*)wizard()->page(InstallWizard::InstallConfigure))
        ->packageIDs();
    pkTransactionProxy->InstallPackages(true, packageIDs);
#endif
}
void InstallingPage::slotStartDownload()
{
    mPackageNames = ((InstallConfigurePage*)wizard()->page(InstallWizard::InstallConfigure))->packageNames();
    mCurrent = 0;
    mCacheDir = QDesktopServices::storageLocation(QDesktopServices::TempLocation)+QDir::separator()+"tizensdk";
    QDir tmpDir(QDesktopServices::storageLocation(QDesktopServices::TempLocation));
    if(!tmpDir.exists("tizensdk"))
        tmpDir.mkdir("tizensdk");
    if(!BasePage::createDir(QDesktopServices::storageLocation(QDesktopServices::DataLocation)))
        return;
    mLogFile.setFileName(QDesktopServices::storageLocation(QDesktopServices::DataLocation)+QDir::separator()+"install-log.txt");
    mLogFile.open(QIODevice::WriteOnly | QIODevice::Text);

    addLog(tr("Preparing download cache directroy: %1").arg(mCacheDir));
    addLog(tr("Download !"));
    if(mInstalledPackageIndex)
        delete mInstalledPackageIndex;
    mInstalledPackageIndex = new TizenPackageIndex();
    mCompleted = false;
    QTimer::singleShot(0, this, SLOT(slotUpdateDownloadStatus()));
}



void InstallingPage::slotPKSignalChanged()
{
#ifdef Q_OS_LINUX
    PKTransaction *pkTransactionProxy = ((InstallWizard*)wizard())->pkTransactionProxy();
    QString status = pkTransactionProxy->property("Status").toString();
    int remainTime = pkTransactionProxy->property("RemainingTime").toInt();
    int Time = pkTransactionProxy->property("RemainingTime").toInt();
    int percentage = pkTransactionProxy->property("Percentage").toInt();
    int subPercentage = pkTransactionProxy->property("Subpercentage").toInt();
    qint64 speed = pkTransactionProxy->property("Speed").toInt();
    QString packageId = pkTransactionProxy->property("LastPackage").toString();

    qDebug() << "-----------------";
    qDebug() << __func__;
    qDebug() << "Speed" << speed;
    qDebug() << "RemainingTime" << remainTime;
    //qDebug() << "ElapsedTime" << pkTransactionProxy->property("ElapsedTime");
    //qDebug() << "CallerActive" << pkTransactionProxy->property("CallerActive");
    //qDebug() << "AllowCancel" << pkTransactionProxy->property("AllowCancel");
    qDebug() << "Subpercentage" << subPercentage;
    qDebug() << "Percentage" << percentage;
    //qDebug() << "Uid" << pkTransactionProxy->property("Uid");
    qDebug() << "LastPackage" << packageId;
    qDebug() << "Status" << status;
    //qDebug() << "Role" << pkTransactionProxy->property("Role");
    qDebug() << "-----------------";

    QStringList packageInfo = packageId.split(";");
    QString name, version, repo;
    if(packageInfo.length()>1) {
        name = packageInfo.at(0);
        version = packageInfo.at(1);
        //QString arch = packageInfo.at(2);
        repo = tr("repository");
        if(packageInfo.count()>3)
            repo = packageInfo.at(3);
    }

    QString log = tr("PackageKit: %1 %2 %3").arg(status).arg(name).arg(version);
    if(log!=mPKLastStatus) {
        addLog(log);
        mPKLastStatus = log;
    }
    if(status=="download")
        mCurrentStatus->setText(tr("Downloading to %1 %2 from %3").arg(name).arg(version).arg(repo));
    else if(status=="install")
        mCurrentStatus->setText(tr("Installing to %1 %2").arg(name).arg(version));
    else if(name.length()>0)
        mCurrentStatus->setText(tr("Preparing to %1 %2").arg(name).arg(version));
    else
        mCurrentStatus->setText("");
    setDownloadSpeed(speed);
    setRemainTime(remainTime);
    mProgressBar->setMaximum(100);
    mProgressBar->setValue(percentage);
#endif
}
void InstallingPage::slotPKSignalItemProgress(const QString &id, uint percentage)
{
#ifdef Q_OS_LINUX
    qDebug() << "itemProgress" << id << percentage;
#endif
}
void InstallingPage::slotPKSignalMessage(const QString &code, const QString &details)
{
#ifdef Q_OS_LINUX
    qDebug() << "message" << code << details;
#endif
}
void InstallingPage::slotPKSignalErrorCode(const QString &code, const QString &details)
{
#ifdef Q_OS_LINUX
    addLog(tr("PackageKit Error(%1) - %2").arg(code).arg(details));
#endif
}
void InstallingPage::slotPKSignalFinished(const QString &exit, uint runtime)
{
    PKTransaction *pkTransactionProxy = ((InstallWizard*)wizard())->pkTransactionProxy();
    disconnect(pkTransactionProxy, SIGNAL(Finished(const QString&,uint)),
            this, SLOT(slotPKSignalFinished(const QString,uint)));
    QTimer::singleShot(0, this, SLOT(slotStartDownload()));

#ifdef Q_OS_LINUX
    qDebug() << "finished" << exit << runtime;
    addLog(tr("PackageKit finished - %1").arg(exit));
#endif
}




void InstallingPage::slotUpdateDownloadStatus()
{
    mCurrentPackage = packageIndex()->find(mPackageNames.at(mCurrent));
    mCurrentReceived->setText("");
    QFileInfo fi(mCurrentPackage->path());
    mCurrentStatus->setText(tr("[%1/%2] Downloading %3 ...")
                                .arg(mCurrent+1)
                                .arg(mPackageNames.count())
                                .arg(fi.fileName()));
    mCurrentRemainTime->setText("");
    mCurrentSpeed->setText("");
    mProgressBar->setMaximum(100);
    mProgressBar->setValue(0);
    QUrl url =  ((InstallWizard*)wizard())->packageListURL().resolved(QUrl(mCurrentPackage->path()));
    addLog(mCurrentStatus->text(), false);
    //setHashCheck(true); // FIXME Qt4 doesn't support SHA-2
    downloadData(url, mCacheDir);
    addLogIntoFile("Download "+url.toString());
    mDownloadedBytes = 0;
    mDownloadedBytesLast = 0;
    if(!mTimer.isActive())
        mTimer.start(1000);
}
void InstallingPage::downloadFinished(const QNetworkReply *reply)
{
    int httpstatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if(200!=httpstatus) {
        addLogIntoFile(QString("Download Fail: http code(%1)").arg(httpstatus));
        if(QMessageBox::No == QMessageBox::warning(this, tr("Download failed"),
            tr("Failed to retrieve package %1 from repository server. Retry?").arg(mCurrentPackage->name()),
            QMessageBox::Yes|QMessageBox::No, QMessageBox::No))
            return;
        downloadData(reply->url(), mCacheDir);
        addLogIntoFile("Retry download "+reply->url().toString());
        return;
    }

    // FIXME - need to compare downloaded file's hash and package index
    //qDebug() << hash() << mCurrentPackage->sha256();

    addLog(tr("Completed"), true, "green");
    if(mTimer.isActive())
        mTimer.stop();

    // download next package
    mCurrent += 1;
    if(mCurrent < mPackageNames.count())
        QTimer::singleShot(0, this, SLOT(slotUpdateDownloadStatus()));
    else {
        // all packages download finished
        addLog(tr("Extract !"));
        QString targetDirPath = field("tizeninstaller.installpath").toString();
        if(!createDir(targetDirPath))
            return;
        qDebug() << QDesktopServices::storageLocation(QDesktopServices::DataLocation);
#ifdef Q_OS_LINUX
        if(!createDir(QDesktopServices::storageLocation(QDesktopServices::HomeLocation)+QDir::separator()
                +".TizenSDK"+QDir::separator()+"info"+QDir::separator()+"installedlist"))
            return;
        if(!createDir(QDesktopServices::storageLocation(QDesktopServices::HomeLocation)+QDir::separator()
                +".TizenSDK"+QDir::separator()+"info"+QDir::separator()+"removescript"))
            return;
#else
        if(!createDir(QDesktopServices::storageLocation(QDesktopServices::DataLocation)+QDir::separator()+"installed-packages"))
            return;
        if(!createDir(QDesktopServices::storageLocation(QDesktopServices::DataLocation)+QDir::separator()+"remove-scripts"))
            return;
#endif

        if(mTimer.isActive())
            mTimer.stop();
        mCurrent = 0;
        QTimer::singleShot(0, this, SLOT(slotUpdateExtractStatus()));
    }
}
void InstallingPage::downloadError(const QString &errorStr)
{
    addLog(tr("Failure"), true, "red");
    addLog(errorStr);
}
void InstallingPage::downloadProgress(qint64 byteReceieved, qint64 byteTotal)
{
    mProgressBar->setValue((int)(byteReceieved*100/byteTotal));
    QString received = QString("%1 / %2")
                .arg(TizenPackageIndex::aboutSize((uint64_t)byteReceieved))
                .arg(TizenPackageIndex::aboutSize((uint64_t)byteTotal));
    if(received!=mCurrentReceived->text())
        mCurrentReceived->setText(received);
    mDownloadedBytes = byteReceieved;
}
void InstallingPage::slotDisplayDownloadSpeed()
{
    qint64 receivedPerSecond = mDownloadedBytes - mDownloadedBytesLast;

    setDownloadSpeed(receivedPerSecond);

    qint64 remainPackageSize = (qint64)mCurrentPackage->size() - mDownloadedBytes;
    int remainSec = remainPackageSize / receivedPerSecond;
    setRemainTime(remainSec);

    mDownloadedBytesLast = mDownloadedBytes;
}
void InstallingPage::setDownloadSpeed(qint64 receivedPerSecond)
{
    if(receivedPerSecond>(1000*1000*1000))
        mCurrentSpeed->setText(QString("%1.%2GB/s").arg(receivedPerSecond/(1000*1000*1000)).arg((receivedPerSecond%(1000*1000*1000))/((1000*1000*1000)/100)));
    else if(receivedPerSecond>(1000*1000))
        mCurrentSpeed->setText(QString("%1.%2MB/s").arg(receivedPerSecond/(1000*1000)).arg((receivedPerSecond%(1000*1000))/(1000*1000/10)));
    else if(receivedPerSecond>1000)
        mCurrentSpeed->setText(QString("%1.%2KB/s").arg(receivedPerSecond/1000).arg((receivedPerSecond%1000)/(1000/10)));
    else
        mCurrentSpeed->setText(QString("%1B/s").arg(receivedPerSecond));
}
void InstallingPage::setRemainTime(int remainSec)
{
    int days = remainSec / 86400;
    int hours = (remainSec % 86400) / 3600;
    int mins = ((remainSec % 86400) % 3600) / 60;
    int secs = (((remainSec % 86400) % 3600) % 60);
    char time[32] = {0, };
    if(days > 0) {
        sprintf(time, "%02d:%02d:%02d", hours, mins, secs);
        mCurrentRemainTime->setText(tr("Remain time: ")+QString("%1 day(s) %2").arg(time));
    } else if(hours > 0) {
        sprintf(time, "%02d:%02d:%02d", hours, mins, secs);
        mCurrentRemainTime->setText(tr("Remain time: ")+time);
    } else {
        sprintf(time, "%02d:%02d", mins, secs);
        mCurrentRemainTime->setText(tr("Remain time: ")+time);
    }
}
void InstallingPage::slotUpdateExtractStatus()
{
    mCurrentPackage = packageIndex()->find(mPackageNames.at(mCurrent));
    mCurrentReceived->setText("");
    mCurrentStatus->setText(tr("[%1/%2] Extracting %3 ...")
                                .arg(mCurrent+1)
                                .arg(mPackageNames.count())
                                .arg(mCurrentPackage->name()));
    mCurrentRemainTime->setText("");
    mCurrentSpeed->setText("");
    mProgressBar->setMaximum(100);
    mProgressBar->setValue(0);
    addLog(mCurrentStatus->text());

    QString tempPackagePath = QUrl::fromLocalFile(mCacheDir+QDir::separator()+"tizensdk").resolved(QUrl(mCurrentPackage->path())).toLocalFile();
    mQuaZip = new QuaZip(tempPackagePath);
    mQuaZip->open(QuaZip::mdUnzip);
    mCurrentFileCountFromZip = mQuaZip->getFileNameList().count();
    mProgressBar->setMaximum(mCurrentFileCountFromZip);
    mInstallScript = QString::null;

    if(!createTempInTargetDir())
        return;
    copyScriptFromResource(":/etc/makeshortcut.sh");
    copyScriptFromResource(":/etc/removeshortcut.sh");

    if(mQuaZip->goToFirstFile()) {
        QTimer::singleShot(0, this, SLOT(slotExtracting()));
    } else
        QTimer::singleShot(0, this, SLOT(slotExtractFinished()));
}
void InstallingPage::slotExtracting()
{
    QuaZipFile zipfile(mQuaZip);
    QuaZipFileInfo zipinfo;
    zipfile.getFileInfo(&zipinfo);

    QString targetDirPath = field("tizeninstaller.installpath").toString();
    QDir targetDir(targetDirPath);

    mCurrentReceived->setText(QString("%1 / %2").arg(mProgressBar->value()+1).arg(mCurrentFileCountFromZip));
    mProgressBar->setValue(mProgressBar->value()+1);


    QString path;
    QString installedList;
    if(zipfile.getActualFileName().startsWith("data")) {
        path = zipfile.getActualFileName().mid(5); // remove 'data/' from filename;
        if(!path.isEmpty())
            installedList += path+"\n";
        else
            goto extractNext;
        if(S_ISDIR(zipinfo.externalAttr >> 16) && !targetDir.exists(path)) {
            targetDir.mkpath(path);
            goto extractNext;
        }
    }


    {
        zipfile.open(QIODevice::ReadOnly);
        QFileInfo fi(zipfile.getActualFileName());
        if(fi.filePath()=="pkginfo.manifest") {
            mInstalledPackageIndex->parse(zipfile.readAll(), false);
        } else {
            if(zipfile.getActualFileName().startsWith("data")) {
                path = zipfile.getActualFileName().replace(QRegExp("^data"), targetDirPath);
            }
            else {
                path = targetDirPath+QDir::separator()+"temp"+QDir::separator()+fi.filePath();
                if(fi.fileName().toLower()=="install" || fi.fileName().toLower()=="install.sh"
    #ifdef Q_WS_WIN
                        || fi.fileName().toLower()=="install.bat"
    #endif
                        )
                    mInstallScript = fi.filePath();
                if(fi.fileName().toLower()=="remove" || fi.fileName().toLower()=="remove.sh"
    #ifdef Q_WS_WIN
                        || fi.fileName().toLower()=="remove.bat"
    #endif
                        ) {
#ifdef Q_OS_LINUX
                    QString dir = QDesktopServices::storageLocation(QDesktopServices::HomeLocation)+QDir::separator()
                            +".TizenSDK"+QDir::separator()+"info"+QDir::separator()+"removescript"
                            +QDir::separator()+mCurrentPackage->name();
#else
                    QString dir = QDesktopServices::storageLocation(QDesktopServices::DataLocation)
                                +QDir::separator()+"remove-scripts"+QDir::separator()
                                +mCurrentPackage->name();
#endif
                    if(!createDir(dir))
                        return;
                    path = dir+QDir::separator()+fi.filePath();
                }
            }
            mCurrentRemainTime->setText(fi.fileName());
            QFile target(path);
            target.open(QIODevice::WriteOnly);
            fchmod(target.handle(), (zipinfo.externalAttr >> 16));
            target.write(zipfile.readAll());
            target.close();

            if(mLogFile.isOpen()) {
                mLogFile.write(path.toLocal8Bit().constData());
                mLogFile.write("\n");
            }

        }
        zipfile.close();
    }

extractNext:
    if(mQuaZip->goToNextFile()) {
        QTimer::singleShot(0, this, SLOT(slotExtracting()));
    } else {
        mCurrentRemainTime->setText("");
        delete mQuaZip;
#ifdef Q_OS_LINUX
        QString logfile =  QDesktopServices::storageLocation(QDesktopServices::HomeLocation)+QDir::separator()
                +".TizenSDK"+QDir::separator()+"info"+QDir::separator()+"installedlist"
                +QDir::separator()+mCurrentPackage->name()+".list";
#else
        QString logfile = QDesktopServices::storageLocation(QDesktopServices::DataLocation)
                +QDir::separator()+"installed-packages"+QDir::separator()
                +mCurrentPackage->name()+".list";
#endif


        QFile log(logfile);
        log.open(QIODevice::WriteOnly);
        log.write(installedList.toLocal8Bit().constData());
        log.close();
        addLog(tr("Saved installed file list into %1").arg(logfile));
        QTimer::singleShot(0, this, SLOT(slotExtractFinished()));
    }
}
void InstallingPage::slotExtractFinished()
{
    QString targetDirPath = field("tizeninstaller.installpath").toString();
#ifdef Q_OS_LINUX // FIXME install script is only allowed on Linux environment
    if(!mInstallScript.isNull()) {
        addLog(tr("Executing install script : %1 ...").arg(mInstallScript));
        QFile file(targetDirPath+QDir::separator()+"temp"+QDir::separator()+mInstallScript);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            addLogIntoFile("------------------------------------------------------------------------------\n");
            QTextStream in(&file);
            in.setCodec(gCodec);
            addLogIntoFile(in.readAll());
            addLogIntoFile("------------------------------------------------------------------------------\n");
        }
        shell("./"+mInstallScript);
        // This package extracting will be finished at processFinished
    }
    else
#endif
    {
        mCurrentStatus->setText(tr("Completed"));
        mCurrentReceived->setText(tr("Total %1 package(s)").arg(mPackageNames.count()));
        extractFinished(QProcess::NormalExit);
    }
}
void InstallingPage::processStandardOutput(const QByteArray &out)
{
    QString data = gCodec->toUnicode(out);
    addLog(data.replace("\n", "<br>"), false);
}
void InstallingPage::processStandardError(const QByteArray &out)
{
    QString data = gCodec->toUnicode(out);
    addLog(data.replace("\n", "<br>"), false, "red");
}
void InstallingPage::processFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    (void)exitCode;
    extractFinished(exitStatus);
}
void InstallingPage::processError(QProcess::ProcessError error)
{
    addLog(tr("Failed to start install script: %1").arg(error));
}
void InstallingPage::extractFinished(QProcess::ExitStatus exitStatus)
{
    removeTempInTargetDir();
    if(exitStatus==QProcess::NormalExit)
        addLog(tr("Completed"), true, "green");
    else
        addLog(tr("Failed"), true, "red");

    mCurrent += 1;
    if(mCurrent < mPackageNames.count())
        QTimer::singleShot(0, this, SLOT(slotUpdateExtractStatus()));
    else {
        // extracting and installing finished
        QString targetDirPath = field("tizeninstaller.installpath").toString();
#ifdef Q_OS_LINUX
        QString confPath = QDesktopServices::storageLocation(QDesktopServices::HomeLocation)+QDir::separator()+".TizenSDK"+QDir::separator()+"tizensdkpath" ;
        {
            QFile f(confPath);
            f.open(QIODevice::WriteOnly|QIODevice::Text);
            f.write("TIZEN_SDK_INSTALLED_PATH=");
            f.write(targetDirPath.toLocal8Bit().constData());
            f.close();
        }
#else
        QString confPath = QDesktopServices::storageLocation(QDesktopServices::DataLocation)+QDir::separator()+"Tizen.conf";
        QSettings settings(confPath, QSettings::IniFormat);
        settings.setValue("installed-path", targetDirPath);
        settings.sync();
#endif
        addLog(tr("Saved config file: %1").arg(confPath));
        addLog(tr("Completed"), true, "green");

        QString installedPackageIndexPath = ((InstallWizard*)wizard())->installedPackageIndexPath();
        QFile f(installedPackageIndexPath);
        f.open(QIODevice::WriteOnly|QIODevice::Text);

        for(int i=0; i<packageIndex()->count(); i++) {
            TizenPackage *package = packageIndex()->at(i);
            if(package->status()==TizenPackage::Installed ||
                    mPackageNames.contains(package->name())) {
                package->store(&f);
            }
        }
        f.close();
        addLog(tr("Saved installed packages index: %1").arg(installedPackageIndexPath));
        addLog(tr("Completed"), true, "green");

        mCompleted = true;
        emit completeChanged();
    }
}
void InstallingPage::addLogIntoFile(const QString &str, bool newline)
{
    if(mLogFile.isOpen()) {
        mLogFile.write(str.toLocal8Bit().constData());
        if(newline)
            mLogFile.write("\n");
    }
}
void InstallingPage::addLog(const QString &str, bool newLine, const QString &color)
{
    QString colorStr = "black";
    if(!color.isNull())
        colorStr = color;
    mInstallLog->insertHtml(QString("<font color=\"%1\">%2</font>%3").arg(colorStr).arg(str).arg(newLine?"<br>":""));
    QTextCursor c = mInstallLog->textCursor();
    c.movePosition (QTextCursor::End);
    mInstallLog->setTextCursor(c);

    addLogIntoFile(str, newLine);
}
bool InstallingPage::isComplete() const
{
    return mCompleted;
}
void InstallingPage::cleanupPage()
{
    if(((InstallWizard*)wizard())->meego())
        ((InstallWizard*)wizard())->meego()->sayAgain();
}
InstallCompletePage::InstallCompletePage(QWidget *parent)
    : BasePage(parent)
{
    setTitle(tr("Installation Completed"));
    setBannerType(InstallerBannerInstall, 4);
    setBanner();

    //setFinalPage(true);

    QLabel *label = new QLabel(tr("Thank you for installing Tizen SDK")+"\n\n"
                        +tr("To use the SDK program, Open the Application menu, find the Tizen SDK folder, and click the program icon."));
    label->setWordWrap(true);

    mShowChangeLog = new QCheckBox(tr("Show release note."));
    mShowChangeLog->setEnabled(false);
    registerField("tizeninstaller.showreleasenote", mShowChangeLog);

    mShowInstallLog = new QCheckBox(tr("Show install log."));
    registerField("tizeninstaller.showinstalllog", mShowInstallLog);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(label);
    layout->addStretch();
    layout->addWidget(mShowChangeLog);
    layout->addWidget(mShowInstallLog);
    setLayout(layout);
}
int InstallCompletePage::nextId() const
{
    return -1;
}
void InstallCompletePage::initializePage()
{
    setupNetworkAccessManager();
    mReleaseNoteURL = ((InstallWizard*)wizard())->packageListURL().resolved(QUrl("RELEASE_NOTE.txt"));
    downloadText(mReleaseNoteURL);
    qDebug() << mReleaseNoteURL;
    if(((InstallWizard*)wizard())->meego())
        ((InstallWizard*)wizard())->meego()->sayLastMessage();
}
void InstallCompletePage::downloadError(const QString &errorStr)
{
    (void)errorStr;
    mShowChangeLog->setChecked(false);
    mShowChangeLog->setEnabled(false);
}
void InstallCompletePage::downloadFinished(const QNetworkReply *reply, const QByteArray &data)
{
    if(200!=reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()) {
        if(QMessageBox::No == QMessageBox::warning(this, tr("Download failed"),
            tr("Failed to retrieve release note from repository server. Retry?"),
            QMessageBox::Yes|QMessageBox::No, QMessageBox::No))
            return;
        downloadText(mReleaseNoteURL);
        return;
    }
    mShowChangeLog->setChecked(true);
    mShowChangeLog->setEnabled(true);
    QString releaseNotePath = field("tizeninstaller.installpath").toString()+QDir::separator()+"RELEASE_NOTE.txt";
    QFile target(releaseNotePath);
    target.open(QIODevice::WriteOnly);
    target.write(data);
    target.close();
}




















UninstallConfigurePage::UninstallConfigurePage(QWidget *parent)
    : BasePage(parent)
{
    setTitle(tr("Uninstallation Packages"));
    setBannerType(InstallerBannerUninstall, 1);
    setBanner();

    QLabel *label = new QLabel(tr("Select the items that you wish to uninstall."));
    label->setWordWrap(true);

    mTreeView = new QTreeView(this);
    mModel = NULL;
    mRemoveOption = new QCheckBox(tr("Do not remove SDK direcotry if user created files exists."));
    registerField("tizeninstaller.removeoption", mRemoveOption);

    setCommitPage(true);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(label);
    layout->addWidget(mTreeView);
    layout->addWidget(mRemoveOption);
    setLayout(layout);
}
void UninstallConfigurePage::initializePage()
{
    if(mModel)
        return;
    packageIndex()->clear();
    ((InstallWizard*)wizard())->loadInstalledPackageList();

    mModel = createPackageItemModel();
    mTreeView->setModel(mModel);
    mTreeView->header()->resizeSections(QHeaderView::ResizeToContents);
    emit completeChanged();
    connect(mModel, SIGNAL(itemChanged(QStandardItem*)), this, SLOT(slotItemChanged(QStandardItem*)));
}
void UninstallConfigurePage::slotItemChanged(QStandardItem *item)
{
    if(item->rowCount() > 0) {
        for(int i=0; i<item->rowCount(); i++) {
            item->child(i)->setCheckState(item->checkState());
        }
        emit completeChanged();
    }
}
bool UninstallConfigurePage::isComplete() const
{
    if(mModel->rowCount() > 0) {
        for(int i=0; i<mModel->rowCount(); i++) {
            if(mModel->item(i)->checkState()==Qt::Checked)
                return true;
        }
    }
    return false;
}
QStringList UninstallConfigurePage::packageNames()
{
    QStringList packageList;
    if(mModel->rowCount() > 0) {
        for(int i=0; i<mModel->rowCount(); i++) {
            QStandardItem *package = mModel->item(i);
            if(package->checkState()==Qt::Checked) {
                for(int j=0; j<package->rowCount(); j++)
                    packageList << package->child(j)->text();
                packageList << package->text();
            }
        }
    }
    return packageList;
}



UninstallingPage::UninstallingPage(QWidget *parent)
    : BasePage(parent)
{
    setTitle(tr("Uninstalling ..."));
    setBannerType(InstallerBannerUninstall, 2);
    setBanner();

    mProgressBar = new QProgressBar;
    mCurrentStatus = new QLabel(tr("Preparing ..."));
    mCurrentRemoved = new QLabel;
    mCurrentPackage = NULL;
    mUninstallLog = new QTextEdit;
    mUninstallLog->setReadOnly(true);
    mCompleted = false;
    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(mCurrentStatus);
    QHBoxLayout *hlayout = new QHBoxLayout;
    hlayout->addStretch();
    hlayout->addWidget(mCurrentRemoved);
    layout->addLayout(hlayout);
    layout->addWidget(mProgressBar);
    layout->addWidget(mUninstallLog);
    setLayout(layout);
}
UninstallingPage::~UninstallingPage()
{
    if(mLogFile.isOpen())
        mLogFile.close();
}
void UninstallingPage::initializePage()
{
    mPackageNames = ((UninstallConfigurePage*)wizard()->page(InstallWizard::UninstallConfigure))->packageNames();
    mCurrent = 0;
    mLogFile.setFileName(QDesktopServices::storageLocation(QDesktopServices::DataLocation)+QDir::separator()+"uninstall-log.txt");
    mLogFile.open(QIODevice::WriteOnly | QIODevice::Text);

    addLog(tr("Preparing to uninstall ..."));

#ifdef Q_OS_LINUX
    QString confPath = QDesktopServices::storageLocation(QDesktopServices::HomeLocation)+QDir::separator()+".TizenSDK"+QDir::separator()+"tizensdkpath" ;
    QFile f(confPath);
    f.open(QIODevice::ReadOnly|QIODevice::Text);
    mInstalledPath = f.readAll().split('=').at(1);
    f.close();
#else
    QString confPath = QDesktopServices::storageLocation(QDesktopServices::DataLocation)+QDir::separator()+"Tizen.conf";
    QSettings settings(confPath, QSettings::IniFormat);
    mInstalledPath =  settings.value("installed-path", "unknown").toString();
#endif
    if(mInstalledPath=="unknown") {
        addLog(tr("Could not load tizen sdk install path on your system"));
        return;
    }
    addLog(tr("Installed Tizen SDK path on your system: %1").arg(mInstalledPath));
    mCompleted = false;
    QTimer::singleShot(0, this, SLOT(slotUpdateRemoveStatus()));
}
void UninstallingPage::slotUpdateRemoveStatus()
{
    mCurrentPackage = packageIndex()->find(mPackageNames.at(mCurrent));
    mCurrentRemoved->setText("");
    mCurrentStatus->setText(tr("[%1/%2] Removing %3 ...")
                                .arg(mCurrent+1)
                                .arg(mPackageNames.count())
                                .arg(mCurrentPackage->name()));
    mProgressBar->setMaximum(200); // remove script (100) + remove installed files(100)
    mProgressBar->setValue(0);
    addLog(mCurrentStatus->text());
#ifdef Q_OS_LINUX
    QString removeScriptDirPath =
            QDesktopServices::storageLocation(QDesktopServices::HomeLocation)+QDir::separator()
            +".TizenSDK"+QDir::separator()+"info"+QDir::separator()+"removescript"
            +QDir::separator()+mCurrentPackage->name();
#else
    QString removeScriptDirPath =
            QDesktopServices::storageLocation(QDesktopServices::DataLocation)+QDir::separator()
            +"remove-scripts"+QDir::separator()+mCurrentPackage->name();
#endif
    QDir removeScriptDir(removeScriptDirPath);
    QString removeScriptName = QString::null;
    if(removeScriptDir.exists()) {
        if(removeScriptDir.exists("remove"))
            removeScriptName = "remove";
        else if(removeScriptDir.exists("remove.sh"))
            removeScriptName = "remove.sh";
#ifdef Q_WS_WIN
        else if(removeScriptDir.exists("remove.bat"))
            removeScriptName = "remove.bat";
#endif
    }
#ifdef Q_OS_LINUX // FIXME remove script is only allowed on Linux environment
    if(!removeScriptName.isNull()) {
        addLog(tr("Executing remove script : %1 ...").arg(removeScriptName));
        QFile file(removeScriptDirPath+QDir::separator()+removeScriptName);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            file.setPermissions((QFile::Permission)0x7755);
            addLogIntoFile("------------------------------------------------------------------------------\n");
            QTextStream in(&file);
            in.setCodec(gCodec);
            addLogIntoFile(in.readAll());
            addLogIntoFile("------------------------------------------------------------------------------\n");
            copyScriptFromResource(":/etc/makeshortcut.sh", removeScriptDirPath);
            copyScriptFromResource(":/etc/removeshortcut.sh", removeScriptDirPath);
            shell("./"+removeScriptName, removeScriptDirPath);
        }
    } else
#endif
    {
        // no remove script executing step
        mProgressBar->setValue(100);
        QTimer::singleShot(0, this, SLOT(slotCleanupInstalledFiles()));
    }
}
void UninstallingPage::slotCleanupInstalledFiles()
{
#ifdef Q_OS_LINUX
    QString installedFileLog = QDesktopServices::storageLocation(QDesktopServices::HomeLocation)+QDir::separator()
                +".TizenSDK"+QDir::separator()+"info"+QDir::separator()+"installedlist"
                +QDir::separator()+mCurrentPackage->name()+".list";
#else
    QString installedFileLog = QDesktopServices::storageLocation(QDesktopServices::DataLocation)
            +QDir::separator()+"installed-packages"+QDir::separator()
            +mCurrentPackage->name()+".list";
#endif
    QFile f(installedFileLog);
    if(f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString data = f.readAll();
        QStringList list = data.split("\n");
        f.close();

        QDir installedDir(mInstalledPath);
        for(int i=list.count()-1; i>=0; i--) {
            QString name = list.at(i);
            if(name.trimmed()=="")
                continue;
            if(installedDir.exists(name)) {
                QFileInfo fi(mInstalledPath+QDir::separator()+name);
                if(fi.isDir()) {
                    addLogIntoFile(tr("Remove %1 ... ").arg(name), false);
                    if(installedDir.rmdir(name))
                        addLogIntoFile(tr("Completed"), true);
                    else
                        addLogIntoFile(tr("Failed (Not empty)"), true);
                } else {
                    addLogIntoFile(tr("Remove %1 ... ").arg(name), false);
                    if(installedDir.remove(name))
                        addLogIntoFile(tr("Completed"), true);
                    else
                        addLogIntoFile(tr("Failed"), true);
                }

            }
            mProgressBar->setValue(100+((list.count()-i)*100 / list.count()));
        }
    }
    addLog(tr("Remove installed file list for %1: %2 ... ").arg(mCurrentPackage->name()).arg(installedFileLog), false);
    if(QFile::remove(installedFileLog))
        addLog(tr("Completed"), true, "green");
    else
        addLog(tr("Failed"), true, "red");

    mProgressBar->setValue(200);

    mCurrent += 1;
    if(mCurrent < mPackageNames.count()) {
        QTimer::singleShot(0, this, SLOT(slotUpdateRemoveStatus()));
    }
    else {
        QTimer::singleShot(0, this, SLOT(slotCleanupFinish()));
    }
}
void UninstallingPage::slotCleanupFinish()
{
    qDebug() << "current " + mCurrent;
    // finished
    QString installedPackageIndexPath = ((InstallWizard*)wizard())->installedPackageIndexPath();
    QFile f(installedPackageIndexPath);
    f.open(QIODevice::WriteOnly|QIODevice::Text);
    for(int i=0; i<packageIndex()->count(); i++) {
        TizenPackage *package = packageIndex()->at(i);
        if(package->status()==TizenPackage::Installed &&
                !mPackageNames.contains(package->name())) {
            package->store(&f);
        }
    }
    f.close();

    if(mPackageNames.count()==packageIndex()->count()) {
        // removed all
        addLog(tr("Remove installed package index %1 ... ").arg(installedPackageIndexPath), false);
        if(QFile::remove(installedPackageIndexPath))
            addLog(tr("Completed"), true, "green");
        else
            addLog(tr("Failed"), true, "red");

        QString confPath = QDesktopServices::storageLocation(QDesktopServices::DataLocation)+QDir::separator()+"Tizen.conf";
        addLog(tr("Remove Tizen SDK configurations: %1 ... ").arg(confPath), false);
        if(QFile::remove(confPath))
            addLog(tr("Completed"), true, "green");
        else
            addLog(tr("Failed"), true, "red");

        bool ret = false;
        addLog(tr("Remove Tizen SDK directory %1 ...").arg(mInstalledPath));
        if(!field("tizeninstaller.removeoption").toBool())
            ret = removeDir(mInstalledPath);
        else {
            QDir dir(mInstalledPath);
            ret = dir.rmdir(mInstalledPath);
        }
        if(ret)
            addLog(tr("Completed"), true, "green");
        else
            addLog(tr("Failed (Not empty)"), true, "red");

    }

    mCompleted = true;
    emit completeChanged();
}
void UninstallingPage::processStandardOutput(const QByteArray &out)
{
    QString data = gCodec->toUnicode(out);
    addLog(data.replace("\n", "<br>"), false);
}
void UninstallingPage::processStandardError(const QByteArray &out)
{
    QString data = gCodec->toUnicode(out);
    addLog(data.replace("\n", "<br>"), false, "red");
}
void UninstallingPage::processError(QProcess::ProcessError error)
{
    addLog(tr("Failed to start remove script: %1").arg(error));
}
void UninstallingPage::processFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    (void)exitCode;
    if(exitStatus==QProcess::NormalExit)
        addLog(tr("Completed"), true, "green");
    else
        addLog(tr("Failed"), true, "red");
    mProgressBar->setValue(100);
#ifdef Q_OS_LINUX
    QString removeScriptDirPath =
            QDesktopServices::storageLocation(QDesktopServices::HomeLocation)+QDir::separator()
            +".TizenSDK"+QDir::separator()+"info"+QDir::separator()+"removescript"
            +QDir::separator()+mCurrentPackage->name();
#else
    QString removeScriptDirPath =
            QDesktopServices::storageLocation(QDesktopServices::DataLocation)+QDir::separator()+"remove-scripts"+QDir::separator()+mCurrentPackage->name();
#endif
    addLog(tr("Remove the cleanup script for %1: %2 ... ").arg(mCurrentPackage->name()).arg(removeScriptDirPath), false);
    if(removeDir(removeScriptDirPath))
        addLog(tr("Completed"), true, "green");
    else
        addLog(tr("Failed"), true, "red");
    if(mCurrent < mPackageNames.count())
        QTimer::singleShot(0, this, SLOT(slotCleanupInstalledFiles()));
}
void UninstallingPage::addLogIntoFile(const QString &str, bool newline)
{
    if(mLogFile.isOpen()) {
        mLogFile.write(str.toLocal8Bit().constData());
        if(newline)
            mLogFile.write("\n");
    }
}
void UninstallingPage::addLog(const QString &str, bool newLine, const QString &color)
{
    addLogIntoFile(str, newLine);

    QString colorStr = "black";
    if(!color.isNull())
        colorStr = color;
    mUninstallLog->insertHtml(QString("<font color=\"%1\">%2</font>%3").arg(colorStr).arg(str).arg(newLine?"<br>":""));
    QTextCursor c = mUninstallLog->textCursor();
    c.movePosition (QTextCursor::End);
    mUninstallLog->setTextCursor(c);
}
bool UninstallingPage::isComplete() const
{
    return mCompleted;
}



UninstallCompletePage::UninstallCompletePage(QWidget *parent)
    : BasePage(parent)
{
    setTitle(tr("Uninstallation completed"));
    setBannerType(InstallerBannerUninstall, 3);
    setBanner();

    QLabel *label = new QLabel(tr("Thank you for using Tizen SDK"));
    label->setWordWrap(true);

    mShowUninstallLog = new QCheckBox(tr("Show uninstall log."));
    registerField("tizeninstaller.showuninstalllog", mShowUninstallLog);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(label);
    layout->addStretch();
    layout->addWidget(mShowUninstallLog);
    setLayout(layout);
}
