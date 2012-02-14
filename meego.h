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

#ifndef MEEGO_H
#define MEEGO_H

#include <QWidget>
#include <QLabel>
#include <QImage>
#include <QPixmap>
#include <QStringList>
#include <QTimer>

class Balloon : public QWidget
{
    Q_OBJECT
public:
    explicit Balloon(int, int, QWidget *parent = 0);
    void setMonolog(const QString &msg);

signals:

public slots:

protected:
    virtual void paintEvent(QPaintEvent *);
    virtual void mousePressEvent(QMouseEvent *);
private:
    QImage mBalloon;
    QPixmap mScaledBalloon;
    QLabel *mLabel;

};
class MeeGo : public QWidget
{
    Q_OBJECT
public:
    explicit MeeGo(int, int, QWidget *parent = 0);
    void sayLastMessage();
    void sayAgain();

signals:

private slots:
    void slotTimeout();

protected:
    virtual void paintEvent(QPaintEvent *);
    virtual void moveEvent(QMoveEvent *);
    virtual void mousePressEvent(QMouseEvent *);
private:
    QImage mMeeGo;
    QPixmap mScaledMeeGo;
    Balloon *mBalloon;
    QStringList mMonologs;
    QTimer  *mTimer;
    bool    mFirstMonolog;
    int     mLastMonologIndex;
    bool    mLastMessage;
};

#endif // MEEGO_H
