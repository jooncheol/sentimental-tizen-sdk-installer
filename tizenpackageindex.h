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

#ifndef TIZENPACKAGELIST_H
#define TIZENPACKAGELIST_H

#include <QObject>
#include <QList>
#include <QStringList>
#include <stdint.h>
#include <QIODevice>

class TizenPackageIndex;
class TizenPackage : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY informationChanged);
    Q_PROPERTY(QString version READ version WRITE setVersion NOTIFY informationChanged);
    Q_PROPERTY(QString maintainer READ maintainer WRITE setMaintainer NOTIFY informationChanged);
    Q_PROPERTY(QString description READ description WRITE setDescription NOTIFY informationChanged);
    Q_PROPERTY(QString attribute READ attribute WRITE setAttribute NOTIFY informationChanged);
    Q_PROPERTY(QStringList depends READ depends WRITE setDepends NOTIFY informationChanged);
    Q_PROPERTY(QString category READ category WRITE setCategory NOTIFY informationChanged);
    Q_PROPERTY(QString removescript READ removeScript WRITE setRemoveScript NOTIFY informationChanged);
    Q_PROPERTY(QString path READ path WRITE setPath NOTIFY informationChanged);
    Q_PROPERTY(long size READ size WRITE setSize NOTIFY informationChanged);
    Q_PROPERTY(QString sha256 READ sha256 WRITE setSha256 NOTIFY informationChanged);
    Q_PROPERTY(PackageStatus status READ status WRITE setStatus NOTIFY informationChanged);
    Q_PROPERTY(bool installed READ installed WRITE setInstalled NOTIFY informationChanged);
public:
    enum PackageStatus {
        Unknown,
        Installed,
        NewPackage,
        UpgradePackage
    };
    explicit TizenPackage(TizenPackageIndex *tpi, QObject *parent = 0);
    QString name() { return mName; };
    void setName(QString data);
    QString version() { return mVersion; };
    void setVersion(QString data);
    QString maintainer() { return mMaintainer; };
    void setMaintainer(QString data);
    QString description() { return mDescription; };
    void setDescription(QString data);
    QString attribute() { return mAttribute; };
    void setAttribute(QString data);
    QString category() { return mCategory; };
    void setCategory(QString data);
    QString removeScript() { return mRemoveScript; };
    void setRemoveScript(QString data);
    QString path() { return mPath; };
    void setPath(QString data);
    QString sha256() { return mSha256; };
    void setSha256(QString data);
    PackageStatus status() { return mStatus; };
    QString statusString();
    void setStatus(PackageStatus status);
    bool installed() { return mInstalled; };
    void setInstalled(bool);
    bool checkUpgrade(TizenPackage *package);
    QStringList depends() { return mDepends; };
    void setDepends(QStringList data);
    long size() { return mSize; };
    long totalSize();
    void setSize(long data);
    void makeDependencies();
    int count(); // count of dependent packages
    TizenPackage *at(int); // return dependent packages
    void store(QIODevice *);

signals:
    void informationChanged();

public slots:
private:
    TizenPackageIndex *mTizenPackageIndex;
    QString mName;
    QString mVersion;
    QString mMaintainer;
    QString mDescription;
    QString mAttribute;
    QString mCategory;
    QString mRemoveScript;
    QString mPath;
    QString mSha256;
    PackageStatus mStatus;
    bool mInstalled;
    uint32_t     mSize;
    QStringList mDepends;
    QList<TizenPackage*> mDependList;
};



class TizenPackageIndex : public QObject
{
    Q_OBJECT
public:
    explicit TizenPackageIndex(QObject *parent = 0);
    virtual ~TizenPackageIndex();
    void clear();
    bool parse(const QString &, bool fromRepositoryServer);
    uint32_t totalSize();
    int count() { return mList.count(); }
    TizenPackage *at(int i) { return mList.at(i); }
    TizenPackage *find(const QString &name);
    void store(QIODevice *);

    static QString aboutSize(uint64_t size);

signals:

public slots:
private:
    QList<TizenPackage*> mList;

};

#endif // TIZENPACKAGELIST_H
