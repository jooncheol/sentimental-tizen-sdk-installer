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

#include "meego.h"
#include <QPainter>
#include <QPaintEvent>
#include <QTimer>
#include <QDesktopServices>
#include <QDir>
#include <QLocale>

Balloon::Balloon(int w, int h, QWidget *parent) :
    QWidget(parent)
{
    mBalloon = QImage(":images/wordballoon.png");
    setToolTip(tr("Click to hide"));
    resize(w, h);
    mLabel = new QLabel(this);
    mLabel->resize(w*8/10, h*6/7);
    mLabel->move((width()-mLabel->width())/2, 0);
    mLabel->setAlignment(Qt::AlignCenter);
    mLabel->setWordWrap(true);
}
void Balloon::setMonolog(const QString &msg)
{
    mLabel->setText(msg);
}
void Balloon::paintEvent(QPaintEvent *e)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);
    p.setClipRegion(e->region());
    if(size()!=mScaledBalloon.size())
        mScaledBalloon = QPixmap::fromImage(mBalloon.scaled(size()));
    p.drawPixmap(0, 0, mScaledBalloon);
}
void Balloon::mousePressEvent(QMouseEvent *e)
{
    QWidget::mousePressEvent(e);
    if(!isHidden())
        hide();
}

MeeGo::MeeGo(int w, int h, QWidget *parent) :
    QWidget(parent)
{
    setToolTip(tr("Click me!"));
    mMeeGo = QImage(":images/meego.png");
    resize(w, h);
    mBalloon = new Balloon(w*5/3, h, parent);
    mBalloon->hide();
    mFirstMonolog = true;
    mLastMessage = false;
    mMonologs << tr("I'm MeeGo...\nPlease don't foget me ..."); // XXX FIRST MONOLOG
    mMonologs << tr("You better do not use this home brew Tizen SDK Installer ... This is not an official and still under brewing.");
    mMonologs << tr("Do you know Mer project?\nVisit to http://merproject.org/");
    mMonologs << tr("Do you know?\nThis is Qt powered program.\nQt is very good cross platform application/UI framework.\nVisit to http://qt.nokia.com/");
    mMonologs << tr("Why the official Tizen SDK installer is implemented by using Java ?");
    mMonologs << tr("You can find the trace of installed Tizen SDK files under %1\nBut Do not edit any files in there!")
#ifdef Q_OS_LINUX
                 .arg(QDesktopServices::storageLocation(QDesktopServices::HomeLocation)+QDir::separator()+".TizenSDK");
#else
                 .arg(QDesktopServices::storageLocation(QDesktopServices::DataLocation)+QDir::separator());
#endif
    mMonologs << tr("Do you have any questions about this Tizen SDK Installer? See the source code: http://gitorious.org/tizen-toys/");
    mMonologs << tr("I miss Moblin, Maemo, Qt, Gtk, Clutter, Maliit ... and many other Open source projects what I used ...");
    mMonologs << tr("I cannot understand why Tizen doesn't use the oFono ... Hmm ...");
    mMonologs << tr("May the D-Bus be with you...\nUse the D-Bus, Luke!");
    mMonologs << tr("Do you want make more friends? Launch the X-Chat2, connect to FreeNode and join the #tizen channel.");
    mMonologs << tr("Tizen SDK's package index file looks very weird. It is similar to the Debian package index format. But it's little bit different.\n~(~.~)~");
    mMonologs << tr("I think that Samsung friends probably will change their weird first Tizen SDK installer. Unfortunately, U may can use this sentimental installer until that time.");
    mMonologs << tr("GObject Introspection and Gtk+ 3 Broadway backend looks very awesome!");
    mMonologs << tr("Do you want make your own MeeGo avatar? Visit to http://appupavatar.com/ .");
    mMonologs << tr("Did you find a problem of this Sentimental Tizen SDK Installer? Do not ask me anything... Fix it by yourself and Send me your patch. :-)");
    mMonologs << tr("Which editor do you prefer... Vim? Emacs?");
    mMonologs << tr("If you are digging on Ubuntu. You should install qemu-kvm binutils-multiarch debhelper fakeroot realpath procps libsdl-gfx1.2-4 gettext liblua5.1-0 libdbus-1-3 libcurl3.");
    mMonologs << tr("Do you want to see the Tizen's emulator code? Read carefully the license agreement...");
    if(QLocale::system().name()=="ko_KR")
        mMonologs << tr("SNSD, Tiara, Kara ... and IU. Yeah !!!");
    mMonologs << tr("I'll always be with you ...\nGood bye TT"); // XXX LAST MONOLOG
    mTimer = new QTimer(this);
    connect(mTimer, SIGNAL(timeout()), this, SLOT(slotTimeout()));
    mTimer->start(3000);
}
void MeeGo::slotTimeout()
{
    mTimer->stop();
    int timeout = 5;
    if(mBalloon->isHidden()) {
        timeout = 15;
        if(mFirstMonolog) {
            mLastMonologIndex = 0;
            mBalloon->setMonolog(mMonologs.first());
            mFirstMonolog = false;
        } else if(mLastMessage) {
            mBalloon->setMonolog(mMonologs.last());
        } else {
            int index;
            // pick random msg except last one
            while((index=qrand()%(mMonologs.count()-1))==mLastMonologIndex) ;
            mBalloon->setMonolog(mMonologs.at(index));
            mLastMonologIndex = index;
        }
        mBalloon->show();
    } else
        mBalloon->hide();
    mTimer->start(timeout*1000);
}

void MeeGo::sayLastMessage()
{
    mLastMessage = true;
    mTimer->stop();
    mBalloon->setMonolog(mMonologs.last());
    mBalloon->show();
}
void MeeGo::sayAgain()
{
    mLastMessage = false;
    mTimer->start(3000);
}

void MeeGo::paintEvent(QPaintEvent *e)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);
    p.setClipRegion(e->region());
    if(size()!=mScaledMeeGo.size())
        mScaledMeeGo = QPixmap::fromImage(mMeeGo.scaled(size()));
    p.drawPixmap(0, 0, mScaledMeeGo);
    p.setBrush(QBrush(QColor(0xff, 0xff, 0xff, 80)));
    p.setPen(Qt::black);
}
void MeeGo::moveEvent(QMoveEvent *e)
{
    QWidget::moveEvent(e);
    mBalloon->move(x()+width()/2, y() - height()*4/7);
}
void MeeGo::mousePressEvent(QMouseEvent *e)
{
    QWidget::mousePressEvent(e);
    slotTimeout();
}
