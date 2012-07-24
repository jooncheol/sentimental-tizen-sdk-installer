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

#ifndef DIALOG_H
#define DIALOG_H

#include <QWizard>
#include <QPixmap>
#include <QRadioButton>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QtNetwork/QNetworkAccessManager>
#include <QNetworkReply>
#include <QTreeView>
#include <QTextEdit>
#include <QDir>
#include <tizenpackageindex.h>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QProgressBar>
#include <QProcess>
#include <QTimer>
#include <QCryptographicHash>
#include "quazip.h"
#include "meego.h"
#ifdef Q_OS_UNIX
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusInterface>
#include "pktransaction.h"
#endif

class InstallWizard : public QWizard
{
    Q_OBJECT

public:
    enum {
        Intro,
        InstallLicense,
        InstallConfigureNetwork,
        InstallConfigure,
        InstallConfigureLocation,
        Installing,
        InstallComplete,
        UninstallConfigure,
        Uninstalling,
        UninstallComplete
    };
    explicit InstallWizard(const QString &, bool withMeeGo, QWidget *parent = 0);
    ~InstallWizard();
    void accept();
    void setPackageListURL(const QUrl &url) { mPackageListUrl = url; };
    void loadInstalledPackageList();
    bool hasInstalledPackageList() { return mHasInstalledPackages; };
    QUrl packageListURL() { return mPackageListUrl; };
    void setPackageIndex(TizenPackageIndex *tpi) { mPackageIndex = tpi; }
    TizenPackageIndex* packageIndex() { return mPackageIndex; }
    MeeGo* meego() { return mMeeGo; }
    QString installedPackageIndexPath();
#ifdef Q_OS_LINUX
    void setUsePackageKit(bool flag) { mUsePackageKit = flag; };
    bool usePackageKit() { return mUsePackageKit; };

    QString distroName() { return mPKDistroName; };
    QString distroVersion() { return mPKDistroVersion; };
    PKTransaction* newPKTransaction(bool);
    QDBusInterface* pkProxy();
    PKTransaction*  pkTransactionProxy();
private:
    QString         mPKDistroName;
    QString         mPKDistroVersion;
    QDBusInterface  *mPKProxy;
    PKTransaction   *mPKTransactionProxy;
private:
    bool mUsePackageKit;
#endif
private slots:
    void slotPKDestroyed();
protected:
    virtual void resizeEvent(QResizeEvent *event);
private:
    void openEditor(const QString &path);
private:
    TizenPackageIndex *mPackageIndex;
    QUrl               mPackageListUrl;
    MeeGo             *mMeeGo;
    bool               mHasInstalledPackages;
};

typedef enum _InstallerBannerType {
    InstallerBannerIntro,
    InstallerBannerInstall,
    InstallerBannerUninstall
} InstallerBannerType;

class BasePage : public QWizardPage
{
    Q_OBJECT
public:
    BasePage(QWidget *parent = 0);
    void setBannerType(InstallerBannerType, int);
    void setBanner();
    void setupNetworkAccessManager();
    void downloadText(const QUrl &);
    void downloadData(const QUrl &, const QDir &);
    void cancelDownload();
    void setHashCheck(bool flag) { mHashCheck = flag; }
    QString hashCode();
    static bool removeDir(const QString &);
    bool createTempInTargetDir();
    void removeTempInTargetDir();
    void copyScriptFromResource(const QString &resourcePath);
    void copyScriptFromResource(const QString &resourcePath, const QString &target);
    TizenPackageIndex* packageIndex();
    void shell(const QString &);
    void shell(const QString &, const QString &cwd);
    QStandardItemModel* createPackageItemModel();
    bool createDir(const QString &);
    QString installedPath() { return mInstalledPath; }
protected:
    virtual void addLog(const QString &str, bool newline=true, const QString &color=QString::null);
    virtual void downloadFinished(const QNetworkReply *, const QByteArray &);
    virtual void downloadFinished(const QNetworkReply *);
    virtual void downloadError(const QString &errorStr);
    virtual void downloadProgress(qint64 byteReceieved, qint64 byteTotal);
    virtual void processError(QProcess::ProcessError);
    virtual void processFinished(int, QProcess::ExitStatus);
    virtual void processStandardOutput(const QByteArray &);
    virtual void processStandardError(const QByteArray &);

private slots:
    void slotDownloadReadyRead();
    void slotDownloadProgress(qint64, qint64);
    void slotDownloadFinished();
    void slotDownloadError(QNetworkReply::NetworkError);
#ifndef QT_NO_OPENSSL
    void slotDownloadSSLError(QNetworkReply *, const QList<QSslError>&);
#endif
    void slotError(QProcess::ProcessError);
    void slotStandardOutput();
    void slotStandardError();
    void slotProcessFinished(int, QProcess::ExitStatus);
    void slotStarted();
    void slotStateChanged(QProcess::ProcessState);

private:
    QPixmap banner();
    InstallerBannerType mBannerType;
    int mStep;

    QNetworkAccessManager   *mAccessManager;
    QNetworkReply           *mCurrentReply;
    QByteArray              mData;
    QFile                   mTarget;
    bool                    mSaveMode;
    QProcess                *mProcess;
    bool                    mHashCheck;
    QString                 mHexHashData;
    QCryptographicHash      *mHash;
    QString                 mInstalledPath;
};

class IntroPage : public BasePage
{
    Q_OBJECT
public:
    IntroPage(QWidget *parent = 0);
    virtual void initializePage();
    virtual int nextId() const;
private:
    QRadioButton *mButtonInstall;
    QRadioButton *mButtonUninstall;
};

class InstallLicensePage : public BasePage
{
    Q_OBJECT
public:
    InstallLicensePage(QWidget *parent = 0);
private:
    QCheckBox *mAgreement;
};

class InstallConfigurePageNetwork : public BasePage
{
    Q_OBJECT
public:
    InstallConfigurePageNetwork(QWidget *parent = 0);
    virtual bool isComplete() const;
private slots:
    void slotProxyMode(int);
private:
    QComboBox   *mProxyMode;
    QLabel      *mProxyModeLabel;
    QLineEdit   *mProxyHost;
    QSpinBox    *mProxyPort;
    QLabel      *mProxyAuthLabel;
    QLabel      *mProxyAuthUserLabel;
    QLineEdit   *mProxyAuthUser;
    QLabel      *mProxyAuthPasswdLabel;
    QLineEdit   *mProxyAuthPasswd;
};

class InstallConfigurePage : public BasePage
{
    Q_OBJECT
public:
    InstallConfigurePage(QWidget *parent = 0);
    virtual void initializePage();
    virtual bool isComplete() const;
    QStringList packageNames();
#ifdef Q_OS_LINUX
    QStringList packageIDs();
#endif
private:
    QTreeView   *mTreeView;
    QStandardItemModel *mModel;
    bool        mPackageListDownloaded;
#ifdef Q_OS_LINUX
    QStandardItem   *mDistPackagesItem;
    QString         mPKDistroId;
    QString         mCurrentPKTransaction;
    QDBusInterface  *mPKProxy;
    PKTransaction   *mPKTransactionProxy;
    QStringList     mPackageIdList;
#endif
private slots:
    void slotItemChanged(QStandardItem*);

    // for packagekit
    void slotPKInitTransaction();
    void slotPKSignalPackage(const QString&, const QString&, const QString&);
    void slotPKSignalDetails(const QString &, const QString &, const QString &, const QString &, const QString &, qulonglong );
    void slotPKSignalFinished(const QString&,uint);
    void slotPKSignalErrorCode(const QString&, const QString&);
protected:
    virtual void downloadFinished(const QNetworkReply *, const QByteArray &);
    virtual void downloadError(const QString &errorStr);
};

class InstallConfigurePageLocation : public BasePage
{
    Q_OBJECT
public:
    InstallConfigurePageLocation(QWidget *parent = 0);
    virtual void initializePage();
    virtual bool isComplete() const;

private slots:
    void slotOpenDir();
    void slotInstallPathEditingFinished();
    void slotInstallPathChanged();

private:
    void savePath();

    QLineEdit *mPath;
    QLabel    *mRequiredSpaceLabel;
    QLabel    *mAvailableSpaceLabel;
    bool       mSpaceIsNotEnough;
};

class InstallingPage : public BasePage
{
    Q_OBJECT
public:
    InstallingPage(QWidget *parent = 0);
    virtual ~InstallingPage();
    virtual void initializePage();
    virtual void cleanupPage();
    virtual bool isComplete() const;
private:
    void extractFinished(QProcess::ExitStatus);
    void addLogIntoFile(const QString &str, bool newline=true);
    void setRemainTime(int sec);
    void setDownloadSpeed(qint64 receivedPerSecond);
protected:
    virtual void addLog(const QString &str, bool newline=true, const QString &color=QString::null);
    virtual void downloadFinished(const QNetworkReply *);
    virtual void downloadError(const QString &errorStr);
    virtual void downloadProgress(qint64 byteReceieved, qint64 byteTotal);
    virtual void processError(QProcess::ProcessError);
    virtual void processFinished(int, QProcess::ExitStatus);
    virtual void processStandardOutput(const QByteArray &);
    virtual void processStandardError(const QByteArray &);
private slots:
    void slotStartInstallPKPackages();
    void slotStartDownload();
    void slotDisplayDownloadSpeed();
    void slotUpdateDownloadStatus();
    void slotUpdateExtractStatus();
    void slotExtracting();
    void slotExtractFinished();

    // for packagekit
    void slotPKSignalFinished(const QString&,uint);
    void slotPKSignalErrorCode(const QString&, const QString&);
    void slotPKSignalMessage(const QString&, const QString&);
    void slotPKSignalItemProgress(const QString&, uint);
    void slotPKSignalChanged();
#ifdef Q_OS_LINUX
private:
    QString mPKLastStatus;
#endif
private:
    QFile           mLogFile;
    QuaZip          *mQuaZip;
    QProgressBar    *mProgressBar;
    QLabel          *mCurrentStatus;
    QLabel          *mCurrentReceived;
    QLabel          *mCurrentRemainTime;
    QLabel          *mCurrentSpeed;
    QString         mCacheDir;
    QStringList     mPackageNames;
    QTextEdit       *mInstallLog;
    int             mCurrent;
    TizenPackage    *mCurrentPackage;
    TizenPackageIndex *mInstalledPackageIndex;
    QString         mInstalledList;
    QString         mInstallScript;
    int             mCurrentFileCountFromZip;
    bool            mCompleted;
    QTimer          mTimer;
    qint64          mDownloadedBytes;
    qint64          mDownloadedBytesLast;
};

class InstallCompletePage : public BasePage
{
    Q_OBJECT
public:
    InstallCompletePage(QWidget *parent = 0);
    virtual void initializePage();
    virtual int nextId() const;
protected:
private:
    QCheckBox *mShowChangeLog;
    QCheckBox *mShowInstallLog;
    QUrl       mReleaseNoteURL;
};









class UninstallConfigurePage : public BasePage
{
    Q_OBJECT
public:
    UninstallConfigurePage(QWidget *parent = 0);
    virtual void initializePage();
    virtual bool isComplete() const;
    QStringList packageNames();
private slots:
    void slotItemChanged(QStandardItem*);
private:
    QTreeView   *mTreeView;
    QCheckBox   *mRemoveOption;
    QStandardItemModel *mModel;
private:
};

class UninstallingPage : public BasePage
{
    Q_OBJECT
public:
    UninstallingPage(QWidget *parent = 0);
    virtual ~UninstallingPage();
private:
    virtual void initializePage();
    virtual bool isComplete() const;
    void addLogIntoFile(const QString &str, bool newline=true);
private slots:
    void slotUpdateRemoveStatus();
    void slotCleanupInstalledFiles();
    void slotCleanupFinish();
protected:
    virtual void addLog(const QString &str, bool newline=true, const QString &color=QString::null);
    virtual void processError(QProcess::ProcessError);
    virtual void processFinished(int, QProcess::ExitStatus);
    virtual void processStandardOutput(const QByteArray &);
    virtual void processStandardError(const QByteArray &);
private:
    QFile           mLogFile;
    QProgressBar    *mProgressBar;
    QLabel          *mCurrentStatus;
    QLabel          *mCurrentRemoved;
    QStringList     mPackageNames;
    QTextEdit       *mUninstallLog;
    int             mCurrent;
    TizenPackage    *mCurrentPackage;
    bool            mCompleted;
    QTimer          mTimer;
};

class UninstallCompletePage : public BasePage
{
    Q_OBJECT
public:
    UninstallCompletePage(QWidget *parent = 0);
private:
    QCheckBox *mShowUninstallLog;
};

#endif // DIALOG_H
