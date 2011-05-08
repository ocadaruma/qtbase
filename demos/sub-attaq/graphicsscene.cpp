/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

//Own
#include "graphicsscene.h"
#include "states.h"
#include "boat.h"
#include "submarine.h"
#include "torpedo.h"
#include "bomb.h"
#include "pixmapitem.h"
#include "animationmanager.h"
#include "qanimationstate.h"
#include "progressitem.h"
#include "textinformationitem.h"

//Qt
#include <QtCore/QPropertyAnimation>
#include <QtCore/QSequentialAnimationGroup>
#include <QtCore/QParallelAnimationGroup>
#include <QtCore/QStateMachine>
#include <QtCore/QFinalState>
#include <QtCore/QPauseAnimation>
#include <QtWidgets/QAction>
#include <QtCore/QDir>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QGraphicsView>
#include <QtWidgets/QGraphicsSceneMouseEvent>
#include <QtCore/QXmlStreamReader>

GraphicsScene::GraphicsScene(int x, int y, int width, int height, Mode mode)
    : QGraphicsScene(x , y, width, height), mode(mode), boat(new Boat)
{
    PixmapItem *backgroundItem = new PixmapItem(QString("background"),mode);
    backgroundItem->setZValue(1);
    backgroundItem->setPos(0,0);
    addItem(backgroundItem);

    PixmapItem *surfaceItem = new PixmapItem(QString("surface"),mode);
    surfaceItem->setZValue(3);
    surfaceItem->setPos(0,sealLevel() - surfaceItem->boundingRect().height()/2);
    addItem(surfaceItem);

    //The item that display score and level
    progressItem = new ProgressItem(backgroundItem);

    textInformationItem = new TextInformationItem(backgroundItem);
    textInformationItem->hide();
    //We create the boat
    addItem(boat);
    boat->setPos(this->width()/2, sealLevel() - boat->size().height());
    boat->hide();

    //parse the xml that contain all data of the game
    QXmlStreamReader reader;
    QFile file(":data.xml");
    file.open(QIODevice::ReadOnly);
    reader.setDevice(&file);
    LevelDescription currentLevel;
    while (!reader.atEnd()) {
         reader.readNext();
         if (reader.tokenType() == QXmlStreamReader::StartElement) {
             if (reader.name() == "submarine") {
                 SubmarineDescription desc;
                 desc.name = reader.attributes().value("name").toString();
                 desc.points = reader.attributes().value("points").toString().toInt();
                 desc.type = reader.attributes().value("type").toString().toInt();
                 submarinesData.append(desc);
             } else if (reader.name() == "level") {
                 currentLevel.id = reader.attributes().value("id").toString().toInt();
                 currentLevel.name = reader.attributes().value("name").toString();
             } else if (reader.name() == "subinstance") {
                 currentLevel.submarines.append(qMakePair(reader.attributes().value("type").toString().toInt(), reader.attributes().value("nb").toString().toInt()));
             }
         } else if (reader.tokenType() == QXmlStreamReader::EndElement) {
            if (reader.name() == "level") {
                levelsData.insert(currentLevel.id, currentLevel);
                currentLevel.submarines.clear();
            }
         }
   }
}

qreal GraphicsScene::sealLevel() const
{
    return (mode == Big) ? 220 : 160;
}

void GraphicsScene::setupScene(QAction *newAction, QAction *quitAction)
{
    static const int nLetters = 10;
    static struct {
        char const *pix;
        qreal initX, initY;
        qreal destX, destY;
    } logoData[nLetters] = {
        {"s",   -1000, -1000, 300, 150 },
        {"u",    -800, -1000, 350, 150 },
        {"b",    -600, -1000, 400, 120 },
        {"dash", -400, -1000, 460, 150 },
        {"a",    1000,  2000, 350, 250 },
        {"t",     800,  2000, 400, 250 },
        {"t2",    600,  2000, 430, 250 },
        {"a2",    400,  2000, 465, 250 },
        {"q",     200,  2000, 510, 250 },
        {"excl",    0,  2000, 570, 220 } };

    QSequentialAnimationGroup * lettersGroupMoving = new QSequentialAnimationGroup(this);
    QParallelAnimationGroup * lettersGroupFading = new QParallelAnimationGroup(this);

    for (int i = 0; i < nLetters; ++i) {
        PixmapItem *logo = new PixmapItem(QLatin1String(":/logo-") + logoData[i].pix, this);
        logo->setPos(logoData[i].initX, logoData[i].initY);
        logo->setZValue(i + 3);
        //creation of the animations for moving letters
        QPropertyAnimation *moveAnim = new QPropertyAnimation(logo, "pos", lettersGroupMoving);
        moveAnim->setEndValue(QPointF(logoData[i].destX, logoData[i].destY));
        moveAnim->setDuration(200);
        moveAnim->setEasingCurve(QEasingCurve::OutElastic);
        lettersGroupMoving->addPause(50);
        //creation of the animations for fading out the letters
        QPropertyAnimation *fadeAnim = new QPropertyAnimation(logo, "opacity", lettersGroupFading);
        fadeAnim->setDuration(800);
        fadeAnim->setEndValue(0);
        fadeAnim->setEasingCurve(QEasingCurve::OutQuad);
    }

    QStateMachine *machine = new QStateMachine(this);

    //This state is when the player is playing
    PlayState *gameState = new PlayState(this, machine);

    //Final state
    QFinalState *final = new QFinalState(machine);

    //Animation when the player enter in the game
    QAnimationState *lettersMovingState = new QAnimationState(machine);
    lettersMovingState->setAnimation(lettersGroupMoving);

    //Animation when the welcome screen disappear
    QAnimationState *lettersFadingState = new QAnimationState(machine);
    lettersFadingState->setAnimation(lettersGroupFading);

    //if new game then we fade out the welcome screen and start playing
    lettersMovingState->addTransition(newAction, SIGNAL(triggered()), lettersFadingState);
    lettersFadingState->addTransition(lettersFadingState, SIGNAL(animationFinished()), gameState);

    //New Game is triggered then player start playing
    gameState->addTransition(newAction, SIGNAL(triggered()), gameState);

    //Wanna quit, then connect to CTRL+Q
    gameState->addTransition(quitAction, SIGNAL(triggered()), final);
    lettersMovingState->addTransition(quitAction, SIGNAL(triggered()), final);

    //Welcome screen is the initial state
    machine->setInitialState(lettersMovingState);

    machine->start();

    //We reach the final state, then we quit
    connect(machine, SIGNAL(finished()), qApp, SLOT(quit()));
}

void GraphicsScene::addItem(Bomb *bomb)
{
    bombs.insert(bomb);
    connect(bomb,SIGNAL(bombExecutionFinished()),this, SLOT(onBombExecutionFinished()));
    QGraphicsScene::addItem(bomb);
}

void GraphicsScene::addItem(Torpedo *torpedo)
{
    torpedos.insert(torpedo);
    connect(torpedo,SIGNAL(torpedoExecutionFinished()),this, SLOT(onTorpedoExecutionFinished()));
    QGraphicsScene::addItem(torpedo);
}

void GraphicsScene::addItem(SubMarine *submarine)
{
    submarines.insert(submarine);
    connect(submarine,SIGNAL(subMarineExecutionFinished()),this, SLOT(onSubMarineExecutionFinished()));
    QGraphicsScene::addItem(submarine);
}

void GraphicsScene::addItem(QGraphicsItem *item)
{
    QGraphicsScene::addItem(item);
}

void GraphicsScene::onBombExecutionFinished()
{
    Bomb *bomb = qobject_cast<Bomb *>(sender());
    bombs.remove(bomb);
    bomb->deleteLater();
    if (boat)
        boat->setBombsLaunched(boat->bombsLaunched() - 1);
}

void GraphicsScene::onTorpedoExecutionFinished()
{
    Torpedo *torpedo = qobject_cast<Torpedo *>(sender());
    torpedos.remove(torpedo);
    torpedo->deleteLater();
}

void GraphicsScene::onSubMarineExecutionFinished()
{
    SubMarine *submarine = qobject_cast<SubMarine *>(sender());
    submarines.remove(submarine);
    if (submarines.count() == 0)
        emit allSubMarineDestroyed(submarine->points());
    else
        emit subMarineDestroyed(submarine->points());
    submarine->deleteLater();
}

void GraphicsScene::clearScene()
{
    foreach (SubMarine *sub, submarines) {
        sub->destroy();
        sub->deleteLater();
    }

    foreach (Torpedo *torpedo, torpedos) {
        torpedo->destroy();
        torpedo->deleteLater();
    }

    foreach (Bomb *bomb, bombs) {
        bomb->destroy();
        bomb->deleteLater();
    }

    submarines.clear();
    bombs.clear();
    torpedos.clear();

    AnimationManager::self()->unregisterAllAnimations();

    boat->stop();
    boat->hide();
    boat->setEnabled(true);
}
