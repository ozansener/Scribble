/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtGui>

#include "mainwindow.h"
#include "scribblearea.h"

//! [0]
MainWindow::MainWindow()
{
    scribbleArea = new ScribbleArea;
    setCentralWidget(scribbleArea);

    createActions();
    createMenus();
    createToolbar();
    setWindowTitle(tr("Error-tolerant Interactive Image Segmentation by Using Dynamic and Iterated Graph-Cut"));
    resize(NORMALWIDTH, NORMALHEIGHT+60);
    scribbleArea->setSegmentationMode(0);
}
//! [0]

//! [1]
void MainWindow::closeEvent(QCloseEvent *event)
//! [1] //! [2]
{
    if (maybeSave()) {
        event->accept();
    } else {
        event->ignore();
    }
}
//! [2]

//! [3]
void MainWindow::open()
//! [3] //! [4]
{

    if (maybeSave()) {
        QString fileName = QFileDialog::getOpenFileName(this,
                                   tr("Open File"), QDir::currentPath());
        if (!fileName.isEmpty())
            scribbleArea->openImage(fileName);
    }

}
//! [4]

//! [5]
void MainWindow::save()
//! [5] //! [6]
{
    QAction *action = qobject_cast<QAction *>(sender());
    QByteArray fileFormat = action->data().toByteArray();
    saveFile(fileFormat);
}


void MainWindow::penWidth()
//! [9] //! [10]
{
    bool ok;
    int newWidth = QInputDialog::getInteger(this, tr("Scribble"),
                                            tr("Select pen width:"),
                                            scribbleArea->penWidth(),
                                            1, 50, 1, &ok);
    if (ok)
        scribbleArea->setPenWidth(newWidth);
}
//! [10]

//! [11]
void MainWindow::about()
//! [11] //! [12]
{
    QMessageBox::about(this, tr("Nokia 2D/3D Conversion"),
            tr("<p>Nokia 2D/3D Conversion</p>"));
}
//! [12]



//! [13]
void MainWindow::createActions()
//! [13] //! [14]
{
    openAct = new QAction(tr("&Open..."), this);
    openAct->setShortcuts(QKeySequence::Open);
    connect(openAct, SIGNAL(triggered()), this, SLOT(open()));

    foreach (QByteArray format, QImageWriter::supportedImageFormats()) {
        QString text = tr("%1...").arg(QString(format).toUpper());

        QAction *action = new QAction(text, this);
        action->setData(format);
        connect(action, SIGNAL(triggered()), this, SLOT(save()));
        saveAsActs.append(action);
    }

    printAct = new QAction(tr("&Print..."), this);
    connect(printAct, SIGNAL(triggered()), scribbleArea, SLOT(print()));

    exitAct = new QAction(tr("E&xit"), this);
    exitAct->setShortcuts(QKeySequence::Quit);
    connect(exitAct, SIGNAL(triggered()), this, SLOT(close()));

    penWidthAct = new QAction(tr("Pen &Width..."), this);
    connect(penWidthAct, SIGNAL(triggered()), this, SLOT(penWidth()));

    clearScreenAct = new QAction(tr("&Clear Screen"), this);
    clearScreenAct->setShortcut(tr("Ctrl+L"));
    connect(clearScreenAct, SIGNAL(triggered()),
            scribbleArea, SLOT(clearImage()));

    aboutAct = new QAction(tr("&About"), this);
    connect(aboutAct, SIGNAL(triggered()), this, SLOT(about()));

    aboutQtAct = new QAction(tr("About &Qt"), this);
    connect(aboutQtAct, SIGNAL(triggered()), qApp, SLOT(aboutQt()));

//    exitFullScrAct= new QAction(tr("&Normal Screen"), this);
//    connect(exitFullScrAct, SIGNAL(triggered()), this, SLOT(exitFullScreen()));

    resetAct = new QAction(tr("Reset"), this);
    connect(resetAct, SIGNAL(triggered()), this, SLOT(reset()));

    segmentAct = new QAction(tr("Segment via GraphCut"),this);
    connect(segmentAct,SIGNAL(triggered()),scribbleArea,SLOT(segmentViaCut()));

    depthAct = new QAction(tr("Get Result"), this);
    connect(depthAct, SIGNAL(triggered()), scribbleArea, SLOT(computeDepth()));

}
void MainWindow::createMenus()
{
    saveAsMenu = new QMenu(tr("&Save As"), this);
    foreach (QAction *action, saveAsActs)
        saveAsMenu->addAction(action);

    fileMenu = new QMenu(tr("&File"), this);
    fileMenu->addAction(openAct);
    fileMenu->addMenu(saveAsMenu);
    fileMenu->addAction(printAct);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAct);

    optionMenu = new QMenu(tr("&Options"), this);
    //optionMenu->addAction(penColorAct);
    optionMenu->addAction(penWidthAct);
    optionMenu->addSeparator();
    optionMenu->addAction(clearScreenAct);

    helpMenu = new QMenu(tr("&Help"), this);
    helpMenu->addAction(aboutAct);
    helpMenu->addAction(aboutQtAct);

    menuBar()->addMenu(fileMenu);
    menuBar()->addMenu(optionMenu);
    menuBar()->addMenu(helpMenu);
}

bool MainWindow::maybeSave()
//! [17] //! [18]
{
    if (scribbleArea->isModified()) {
       QMessageBox::StandardButton ret;
       ret = QMessageBox::warning(this, tr("Scribble"),
                          tr("The image has been modified.\n"
                             "Do you want to save your changes?"),
                          QMessageBox::Save | QMessageBox::Discard
			  | QMessageBox::Cancel);
        if (ret == QMessageBox::Save) {
            return saveFile("png");
        } else if (ret == QMessageBox::Cancel) {
            return false;
        }
    }
    return true;
}
//! [18]

//! [19]
bool MainWindow::saveFile(const QByteArray &fileFormat)
//! [19] //! [20]
{
    QString initialPath = QDir::currentPath() + "/untitled." + fileFormat;

    QString fileName = QFileDialog::getSaveFileName(this, tr("Save As"),
                               initialPath,
                               tr("%1 Files (*.%2);;All Files (*)")
                               .arg(QString(fileFormat.toUpper()))
                               .arg(QString(fileFormat)));
    if (fileName.isEmpty()) {
        return false;
    } else {
        return scribbleArea->saveImage(fileName, fileFormat);
    }
}
//! [20]


void MainWindow::createToolbar()
{
    fileToolBar = addToolBar(tr("File"));
    fileToolBar->addAction(openAct);
    fileToolBar->addAction(resetAct);
    fileToolBar->addAction(segmentAct);
    fileToolBar->addAction(depthAct);

    combo1 = new QComboBox(this);
    combo1->addItem("Batch Mode");
    combo1->addItem("Coloring Mode");
    combo1->addItem("No GCut Mode");
    connect(combo1, SIGNAL(currentIndexChanged(int)),scribbleArea, SLOT(setSegmentationMode(int)));

    fileToolBar->addWidget(combo1);

    viewCombo = new QComboBox(this);
    viewCombo->addItem("Input");
    viewCombo->addItem("Depth");
    viewCombo->addItem("Anaglyph");
    viewCombo->addItem("Left");
    viewCombo->addItem("Right");
    viewCombo->addItem("Only FG");
    viewCombo->addItem("Enlarged FG");
    viewCombo->addItem("Filled BG");
    viewCombo->addItem("Alpha Blend");
    viewCombo->addItem("SegmentationResult");
    connect(viewCombo,SIGNAL(currentIndexChanged(int)),scribbleArea,SLOT(changeView(int)));
    fileToolBar->addWidget(viewCombo);

    depthModeCombo = new QComboBox(this);
    depthModeCombo->addItem("0.0");depthModeCombo->addItem("0.1");depthModeCombo->addItem("0.2");
    depthModeCombo->addItem("0.3");depthModeCombo->addItem("0.4");depthModeCombo->addItem("0.5");
    depthModeCombo->addItem("0.6");depthModeCombo->addItem("0.7");depthModeCombo->addItem("0.8");
    depthModeCombo->addItem("0.9");depthModeCombo->addItem("1.0");
    connect(depthModeCombo, SIGNAL(currentIndexChanged(int)), scribbleArea, SLOT(changeDepthMode(int)));
    fileToolBar->addWidget(depthModeCombo);

    alphaMatteCombo = new QComboBox(this);
    alphaMatteCombo->addItem("5");
    alphaMatteCombo->addItem("7");
    alphaMatteCombo->addItem("9");
    connect(alphaMatteCombo, SIGNAL(currentIndexChanged(int)), scribbleArea, SLOT(changeAlphaMatte(int)));
    fileToolBar->addWidget(alphaMatteCombo);

    SPMerge = new QCheckBox("SP Merge");
    fileToolBar->addWidget(SPMerge);
    connect(SPMerge,SIGNAL(stateChanged(int)),scribbleArea,SLOT(enableSPMerge(int)));

    enlargeForeground = new QCheckBox("Enlarge FG");
    fileToolBar->addWidget(enlargeForeground);
    connect(enlargeForeground, SIGNAL(stateChanged(int)), scribbleArea, SLOT(changeFGEnlargement(int)));

    blur = new QCheckBox("Blur BG");
    fileToolBar->addWidget(blur);
    connect(blur, SIGNAL(stateChanged(int)), scribbleArea, SLOT(changeBlur(int)));


    /*
    checkFullsScreen = new QCheckBox("FS");
    fileToolBar->addWidget(checkFullsScreen);
    connect(checkFullsScreen, SIGNAL(stateChanged(int)), this,SLOT(fullscreenCheck(int)));
    */
}

void MainWindow::reset(){
    scribbleArea->reset();
}

void MainWindow::fullscreenCheck(int check)
{
    if(check==Qt::Checked)
    {
        fileToolBar->hide();
    }
    else
    {
        fileToolBar->show();
    }
    update();
}
