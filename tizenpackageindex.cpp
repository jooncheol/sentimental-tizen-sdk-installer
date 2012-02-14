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
void TizenPackage::setCategory(QString data)
{
    mCategory = data; emit informationChanged();
}
void TizenPackage::setRemoveScript(QString data)
{
    mRemoveScript = data; emit informationChanged();
}
void TizenPackage::setPath(QString data)
{
    mPath = data; emit informationChanged();
}
void TizenPackage::setSize(long data)
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
void TizenPackage::setDepends(QStringList data)
{
    mDepends = data; emit informationChanged();
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
    io->write(QString("Maintainer: %1\n").arg(maintainer()).toLocal8Bit().constData());
    QString deps;
    QStringList dependencies = depends();
    for(int i=0; i<dependencies.count(); i++) {
        deps += dependencies.at(i);
        if((i+1)!=dependencies.count())
            deps += ", ";
    }
    io->write(QString("Depends: %1\n").arg(deps).toLocal8Bit().constData());
    io->write(QString("Description: %1\n").arg(description()).toLocal8Bit().constData());
    io->write(QString("Attribute: %1\n").arg(attribute()).toLocal8Bit().constData());
    io->write(QString("Remove-script: %1\n").arg(removeScript()).toLocal8Bit().constData());
    io->write(QString("Category: %1\n").arg(category()).toLocal8Bit().constData());
    io->write(QString("Size: %1\n").arg(size()).toLocal8Bit().constData());
    io->write(QString("Path: %1\n").arg(path()).toLocal8Bit().constData());
    io->write(QString("SHA256: %1\n").arg(sha256()).toLocal8Bit().constData());
    io->write("\n\n");
}
// create one depth dependencies
void TizenPackage::makeDependencies()
{
    for(int i=0; i<mDepends.count(); i++) {
        QString dep = depends().at(i);
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
long TizenPackage::totalSize()
{
    long total = size();
    for(int i=0; i<mDependList.count(); i++) {
        total += mDependList.at(i)->size();
    }
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
        } else if (package && field == QLatin1String("Maintainer")) {
            package->setProperty("maintainer", value);
        } else if (package && field == QLatin1String("Depends")) {
            QStringList l = value.split(",");
            QStringList dependencies;
            for(int j=0; j<l.count(); j++)
                dependencies.append(l.at(j).trimmed());
            package->setProperty("depends", dependencies);
            qDebug() << "depends" << dependencies;
        } else if (package && field == QLatin1String("Description")) {
            package->setProperty("description", value);
        } else if (package && field == QLatin1String("Attribute")) {
            package->setProperty("attribute", value);
        } else if (package && field == QLatin1String("Remove-script")) {
            package->setProperty("removescript", value);
        } else if (package && field == QLatin1String("Category")) {
            package->setProperty("category", value);
        } else if (package && field == QLatin1String("Size")) {
            package->setProperty("size", value.toInt());
        } else if (package && field == QLatin1String("Path")) {
            package->setProperty("path", value);
        } else if (package && field == QLatin1String("SHA256")) {
            package->setProperty("sha256", value);
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
