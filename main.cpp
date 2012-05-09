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

#include <QtGui/QApplication>
#include <QTranslator>
#include <QLocale>
#include <QDebug>
#include "installwizard.h"
#include <stdio.h>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QCoreApplication::setOrganizationName("Tizen Toys");
    // XXX This domain is only for Mac settings.
    QCoreApplication::setOrganizationDomain("tizen-toys.gitorious.org");
    QCoreApplication::setApplicationName("Sentimental Tizen SDK Installer");

    QTranslator translator;
    translator.load(QString(":/translations/i18n_%1.qm").arg(QLocale::system().name()));
    a.installTranslator(&translator);

    // beta url
    // const char *defaultURL = "http://source.tizen.org/tizensdkpackages/release/pkg_list_linux";

    // 1.0 url
    const char *defaultURL = "http://download.tizen.org/sdk/current/pkg_list_linux";
    QString packageIndexURL(defaultURL);
    bool withMeeGo = false;
#ifdef Q_OS_LINUX
    bool withPackageKit = true;
#endif
    QStringList args = a.arguments();
    args.takeFirst();
    while(!args.isEmpty() && args.at(0).startsWith(QLatin1String("-"))) {
        QString arg = args.takeFirst();
        if(arg==QLatin1String("-h") || arg==QLatin1String("--help")) {
            printf("Usage\n");
            printf("  %s (options) (PackageIndexURL)\n\n", argv[0]);
            printf("default tizen package index url is:\n");
            printf("  %s\n", defaultURL);
            printf("Options:\n");
            printf("  -h, --help             : This help message\n");
            printf("  --with-meego           : With MeeGo character\n");
#ifdef Q_OS_LINUX
            printf("  --without-packagekit   : Install depends system packages using packagekit\n");
#endif
            return 0;
        } else if(arg==QLatin1String("--with-meego")) {
            withMeeGo = true;
        }
#ifdef Q_OS_LINUX
        else if(arg==QLatin1String("--without-packagekit")) {
            withPackageKit = false;
        }
#endif
    }
    if(!args.isEmpty())
        packageIndexURL = args.takeFirst();

    qsrand(QTime(0,0,0).secsTo(QTime::currentTime()));

    InstallWizard wizard(packageIndexURL, withMeeGo);
#ifdef Q_OS_LINUX
    wizard.setUsePackageKit(withPackageKit);
#endif
    wizard.show();

    return a.exec();
}
