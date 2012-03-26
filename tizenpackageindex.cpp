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

#include "tizenpackageindex.h"
#include <QStringList>
#include <QDebug>
#include <QVariant>

TizenPackage::TizenPackage(TizenPackageIndex *tpi, QObject *parent) :
    QObject(parent), mTizenPackageIndex(tpi)
{
    mStatus = Unknown;
    mInstalled = false;
}
void TizenPackage::setName(QString data)
{
    mName = data; emit informationChanged();
}
void TizenPackage::setVersion(QString data)
{
    mVersion = data; emit informationChanged();
}
void TizenPackage::setOS(QString data)
{
    mOS = data; emit informationChanged();
}
void TizenPackage::setBuildHostOS(QString data)
{
    mBuildHostOS = data; emit informationChanged();
}
void TizenPackage::setMaintainer(QString data)
{
    mMaintainer = data; emit informationChanged();
}
void TizenPackage::setDescription(QString data)
{
    mDescription = data; emit informationChanged();
}
void TizenPackage::setAttribute(QString data)
{
    mAttribute = data; emit informationChanged();
}
void TizenPackage::setPath(QString data)
{
    mPath = data; emit informationChanged();
}
void TizenPackage::setSrcPath(QString data)
{
    mSrcPath = data; emit informationChanged();
}
void TizenPackage::setOrigin(QString data)
{
    mOrigin = data; emit informationChanged();
}
void TizenPackage::setSize(uint32_t data)
{
    mSize = data; emit informationChanged();
}
void TizenPackage::setSha256(QString data)
{
    mSha256 = data; emit informationChanged();
}
void TizenPackage::setStatus(PackageStatus status)
{
    mStatus = status; emit informationChanged();
};
QString TizenPackage::statusString()
{
    if(mStatus==Installed)
        return tr("Installed");
    else if(mStatus==NewPackage)
        return tr("New");
    else if(mStatus==UpgradePackage)
        return tr("Upgrade");
    return tr("Unknown");
}
void TizenPackage::setSourceDependency(QStringList data)
{
    mSourceDependency = data; emit informationChanged();
}
void TizenPackage::setBuildDependency(QStringList data)
{
    mBuildDependency = data; emit informationChanged();
}
void TizenPackage::setInstallDependency(QStringList data)
{
    mInstallDependency = data; emit informationChanged();
}
bool TizenPackage::checkUpgrade(TizenPackage *package)
{
    if(version()<package->version()) {
        return true;
    }
    return false;
}
void TizenPackage::setInstalled(bool flag)
{
    if(flag)
        mStatus = Installed;
    mInstalled = flag; emit informationChanged();
}
void TizenPackage::store(QIODevice *io)
{
    io->write(QString("Package: %1\n").arg(name()).toLocal8Bit().constData());
    io->write(QString("Version: %1\n").arg(version()).toLocal8Bit().constData());
    io->write(QString("OS: %1\n").arg(os()).toLocal8Bit().constData());
    io->write(QString("Build-host-os: %1\n").arg(buildHostOS()).toLocal8Bit().constData());
    io->write(QString("Maintainer: %1\n").arg(maintainer()).toLocal8Bit().constData());
    QString deps;
    QStringList dependencies = buildDependency();
    for(int i=0; i<dependencies.count(); i++) {
        deps += dependencies.at(i);
        if((i+1)!=dependencies.count())
            deps += ", ";
    }
    io->write(QString("Build-dependency: %1\n").arg(deps).toLocal8Bit().constData());
    deps = "";
    dependencies = sourceDependency();
    for(int i=0; i<dependencies.count(); i++) {
        deps += dependencies.at(i);
        if((i+1)!=dependencies.count())
            deps += ", ";
    }
    io->write(QString("Source-dependency: %1\n").arg(deps).toLocal8Bit().constData());
    deps = "";
    dependencies = installDependency();
    for(int i=0; i<dependencies.count(); i++) {
        deps += dependencies.at(i);
        if((i+1)!=dependencies.count())
            deps += ", ";
    }
    io->write(QString("Install-dependency: %1\n").arg(deps).toLocal8Bit().constData());
    io->write(QString("Attribute: %1\n").arg(attribute()).toLocal8Bit().constData());
    io->write(QString("Path: %1\n").arg(path()).toLocal8Bit().constData());
    io->write(QString("Src-path: %1\n").arg(srcPath()).toLocal8Bit().constData());
    io->write(QString("Origin: %1\n").arg(origin()).toLocal8Bit().constData());
    io->write(QString("SHA256: %1\n").arg(sha256()).toLocal8Bit().constData());
    io->write(QString("Size: %1\n").arg(size()).toLocal8Bit().constData());
    io->write(QString("Description: %1\n").arg(description()).toLocal8Bit().constData());
    io->write("\n\n");
}
// create one depth dependencies
void TizenPackage::makeDependencies()
{
    for(int i=0; i<mInstallDependency.count(); i++) {
        QString dep = mInstallDependency.at(i).split(" ").at(0).trimmed();
        for(int j=0; j<mTizenPackageIndex->count(); j++) {
            TizenPackage *p = mTizenPackageIndex->at(j);
            if(dep == p->name()) {
                qDebug() << name() << "has" << p->name();
                mDependList.append(p);
                break;
            }
        }
    }
}
int TizenPackage::count()
{
    return mDependList.count();
}
TizenPackage * TizenPackage::at(int index)
{
    return mDependList.at(index);
}
static uint32_t calcSubPackageSize(QStringList &calcedList, TizenPackage *subPackage)
{
    uint32_t total = 0;
    for(int i=0; i<subPackage->count(); i++)
        total += calcSubPackageSize(calcedList, subPackage->at(i));
    if(!calcedList.contains(subPackage->name())) {
        total += subPackage->size();
        calcedList += subPackage->name();
    }
    return total;
}
uint32_t TizenPackage::totalSize()
{
    uint32_t total = 0;
    QStringList calcedList;
    for(int i=0; i<mDependList.count(); i++) {
        total += calcSubPackageSize(calcedList, mDependList.at(i));
    }
    qDebug() << name() << calcedList << total << size();
    if(!calcedList.contains(name()))
        total += size();
    return total;
}

TizenPackageIndex::TizenPackageIndex(QObject *parent) :
    QObject(parent)
{
}
TizenPackageIndex::~TizenPackageIndex()
{
    clear();
}
void TizenPackageIndex::clear()
{
    while(!mList.isEmpty())
        delete mList.takeFirst();
}

bool TizenPackageIndex::parse(const QString &packageIndexData, bool fromRepositoryServer)
{
    QStringList lines = packageIndexData.split("\n");
    TizenPackage *package = NULL;
    for(int i=0; i<lines.count(); i++) {
        QString line = lines.at(i).trimmed();
        int colon = line.indexOf(":");
        if(colon < 0)
            continue;
        QString field = line.mid(0, colon).trimmed();
        QString value = line.mid(colon+1).trimmed();
        if(!package && field == QLatin1String("Package")) {
            package = new TizenPackage(this);
            package->setProperty("name", value);
        } else if (package && field == QLatin1String("Version")) {
            package->setProperty("version", value);
        } else if (package && field == QLatin1String("OS")) {
            package->setProperty("os", value);
        } else if (package && field == QLatin1String("Build-host-os")) {
            package->setProperty("buildHostOS", value);
        } else if (package && field == QLatin1String("Maintainer")) {
            package->setProperty("maintainer", value);
        } else if (package && field == QLatin1String("Source-dependency")) {
            QStringList l = value.split(",");
            QStringList dependencies;
            for(int j=0; j<l.count(); j++)
                dependencies.append(l.at(j).trimmed());
            package->setProperty("sourceDependency", dependencies);
            qDebug() << "sourceDependency" << dependencies;
        } else if (package && field == QLatin1String("Build-dependency")) {
            QStringList l = value.split(",");
            QStringList dependencies;
            for(int j=0; j<l.count(); j++)
                dependencies.append(l.at(j).trimmed());
            package->setProperty("buildDependency", dependencies);
            qDebug() << "buildDependency" << dependencies;
        } else if (package && field == QLatin1String("Install-dependency")) {
            QStringList l = value.split(",");
            QStringList dependencies;
            for(int j=0; j<l.count(); j++)
                dependencies.append(l.at(j).trimmed());
            package->setProperty("installDependency", dependencies);
            qDebug() << "installDependency" << dependencies;
        } else if (package && field == QLatin1String("Attribute")) {
            package->setProperty("attribute", value);
        } else if (package && field == QLatin1String("Size")) {
            package->setSize((uint32_t)value.toInt());
            qDebug() << package->name() << "size" << package->size() << value << value.toInt() << (uint32_t)value.toInt();
        } else if (package && field == QLatin1String("Path")) {
            package->setProperty("path", value);
        } else if (package && field == QLatin1String("Src-path")) {
            package->setProperty("srcPath", value);
        } else if (package && field == QLatin1String("Origin")) {
            package->setProperty("origin", value);
        } else if (package && field == QLatin1String("SHA256")) {
            package->setProperty("sha256", value);
        } else if (package && field == QLatin1String("Description")) {
            package->setProperty("description", value);
            if(!fromRepositoryServer) {
                package->setProperty("installed", true);
            } else {
                // check whether already installed or not
                TizenPackage *installedPackage = NULL;
                for(int j=0; j<mList.count(); j++) {
                    TizenPackage *p = mList.at(j);
                    if(p->name()==package->name()) {
                        installedPackage = p;
                        if(p->checkUpgrade(package)) {
                            package->setStatus(TizenPackage::UpgradePackage);
                        } else
                            package->setStatus(TizenPackage::Installed);
                        break;
                    }
                }
                if(installedPackage) {
                    mList.removeOne(installedPackage);
                    delete installedPackage;
                } else
                    package->setStatus(TizenPackage::NewPackage);
            }
            mList.append(package);
            qDebug() << "Added package: " << package->property("name");
            package = NULL;
        } else {
            qDebug() << "XXX" << field << value;
        }
    }

    for(int i=0; i<mList.count(); i++) {
        mList.at(i)->makeDependencies();
    }
    return true;
}

uint32_t TizenPackageIndex::totalSize()
{
    uint32_t size = 0;
    for(int i=0; i < mList.count(); i++) {
        TizenPackage *p = mList.at(i);
        size += p->size();
    }
    return size;
}
QString TizenPackageIndex::aboutSize(uint64_t size)
{
    if(size>(1024*1024*1024))
        return QString("%1.%2GiB").arg(size/(1024*1024*1024)).arg((size%(1024*1024*1024))/((1024*1024*1024)/100));
    if(size>(1024*1024))
        return QString("%1.%2MiB").arg(size/(1024*1024)).arg((size%(1024*1024))/(1024*1024/10));
    if(size>1024)
        return QString("%1.%2KiB").arg(size/1024).arg((size%1024)/(1024/10));
    return QString("%1Byte").arg(size);
}
TizenPackage* TizenPackageIndex::find(const QString &name)
{
    for(int i=0; i < mList.count(); i++) {
        TizenPackage *p = mList.at(i);
        if(p->name()==name)
            return p;
        }
    return NULL;
}
void TizenPackageIndex::store(QIODevice *io)
{
    for(int i=0; i < mList.count(); i++) {
        TizenPackage *p = mList.at(i);
        p->store(io);
    }
}
