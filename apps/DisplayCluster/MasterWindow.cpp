/*********************************************************************/
/* Copyright (c) 2014, EPFL/Blue Brain Project                       */
/*                     Raphael Dumusc <raphael.dumusc@epfl.ch>       */
/* All rights reserved.                                              */
/*                                                                   */
/* Redistribution and use in source and binary forms, with or        */
/* without modification, are permitted provided that the following   */
/* conditions are met:                                               */
/*                                                                   */
/*   1. Redistributions of source code must retain the above         */
/*      copyright notice, this list of conditions and the following  */
/*      disclaimer.                                                  */
/*                                                                   */
/*   2. Redistributions in binary form must reproduce the above      */
/*      copyright notice, this list of conditions and the following  */
/*      disclaimer in the documentation and/or other materials       */
/*      provided with the distribution.                              */
/*                                                                   */
/*    THIS  SOFTWARE IS PROVIDED  BY THE  UNIVERSITY OF  TEXAS AT    */
/*    AUSTIN  ``AS IS''  AND ANY  EXPRESS OR  IMPLIED WARRANTIES,    */
/*    INCLUDING, BUT  NOT LIMITED  TO, THE IMPLIED  WARRANTIES OF    */
/*    MERCHANTABILITY  AND FITNESS FOR  A PARTICULAR  PURPOSE ARE    */
/*    DISCLAIMED.  IN  NO EVENT SHALL THE UNIVERSITY  OF TEXAS AT    */
/*    AUSTIN OR CONTRIBUTORS BE  LIABLE FOR ANY DIRECT, INDIRECT,    */
/*    INCIDENTAL,  SPECIAL, EXEMPLARY,  OR  CONSEQUENTIAL DAMAGES    */
/*    (INCLUDING, BUT  NOT LIMITED TO,  PROCUREMENT OF SUBSTITUTE    */
/*    GOODS  OR  SERVICES; LOSS  OF  USE,  DATA,  OR PROFITS;  OR    */
/*    BUSINESS INTERRUPTION) HOWEVER CAUSED  AND ON ANY THEORY OF    */
/*    LIABILITY, WHETHER  IN CONTRACT, STRICT  LIABILITY, OR TORT    */
/*    (INCLUDING NEGLIGENCE OR OTHERWISE)  ARISING IN ANY WAY OUT    */
/*    OF  THE  USE OF  THIS  SOFTWARE,  EVEN  IF ADVISED  OF  THE    */
/*    POSSIBILITY OF SUCH DAMAGE.                                    */
/*                                                                   */
/* The views and conclusions contained in the software and           */
/* documentation are those of the authors and should not be          */
/* interpreted as representing official policies, either expressed   */
/* or implied, of The University of Texas at Austin.                 */
/*********************************************************************/

#include "MasterWindow.h"

#include "log.h"
#include "Options.h"
#include "configuration/MasterConfiguration.h"

#include "ContentLoader.h"
#include "ContentFactory.h"
#include "StateSerializationHelper.h"

#include "DynamicTexture.h"

#include "DisplayGroup.h"
#include "ContentWindow.h"

#include "DisplayGroupGraphicsView.h"
#include "DisplayGroupListWidget.h"
#include "BackgroundWidget.h"
#include "WebbrowserWidget.h"

#include <dc/core/version.h>

#include <QtWidgets>

namespace
{
const QString STATE_FILES_FILTER( "State files (*.dcx)" );
const QSize DEFAULT_WINDOW_SIZE( 800, 600 );
}

MasterWindow::MasterWindow( DisplayGroupPtr displayGroup,
                            MasterConfiguration& config )
    : QMainWindow()
    , displayGroup_( displayGroup )
    , options_( new Options )
    , backgroundWidget_( new BackgroundWidget( config, this ))
    , webbrowserWidget_( new WebbrowserWidget( config, this ))
    , dggv_( new DisplayGroupGraphicsView( config, options_, this ))
    , contentFolder_( config.getDockStartDir( ))
    , sessionFolder_( config.getDockStartDir( ))
{
    backgroundWidget_->setModal( true );

    connect( backgroundWidget_, SIGNAL( backgroundColorChanged( QColor )),
             options_.get(), SLOT( setBackgroundColor( QColor )));
    connect( backgroundWidget_, SIGNAL( backgroundContentChanged( ContentPtr )),
             options_.get(), SLOT( setBackgroundContent( ContentPtr )));

    connect( webbrowserWidget_,
             SIGNAL( openWebBrowser( QPointF, QSize, QString )),
             this, SIGNAL( openWebBrowser( QPointF, QSize, QString )));

    resize( DEFAULT_WINDOW_SIZE );
    setAcceptDrops( true );

    setupMasterWindowUI();

    show();
}

MasterWindow::~MasterWindow()
{
}

DisplayGroupGraphicsView* MasterWindow::getGraphicsView()
{
    return dggv_;
}

OptionsPtr MasterWindow::getOptions() const
{
    return options_;
}

void MasterWindow::setupMasterWindowUI()
{
    // create menus in menu bar
    QMenu * fileMenu = menuBar()->addMenu("&File");
    QMenu * editMenu = menuBar()->addMenu("&Edit");
    QMenu * viewMenu = menuBar()->addMenu("&View");
    QMenu * toolsMenu = menuBar()->addMenu("&Tools");
    QMenu * helpMenu = menuBar()->addMenu("&Help");

    // create tool bar
    QToolBar * toolbar = addToolBar("toolbar");

    // open content action
    QAction * openContentAction = new QAction("Open Content", this);
    openContentAction->setStatusTip("Open content");
    connect(openContentAction, SIGNAL(triggered()), this, SLOT(openContent()));

    // open contents directory action
    QAction * openContentsDirectoryAction = new QAction("Open Contents Directory", this);
    openContentsDirectoryAction->setStatusTip("Open contents directory");
    connect(openContentsDirectoryAction, SIGNAL(triggered()), this, SLOT(openContentsDirectory()));

    // clear contents action
    QAction * clearContentsAction = new QAction("Clear", this);
    clearContentsAction->setStatusTip("Clear");
    connect(clearContentsAction, SIGNAL(triggered()), displayGroup_.get(), SLOT(clear()));

    // save state action
    QAction * saveStateAction = new QAction("Save State", this);
    saveStateAction->setStatusTip("Save state");
    connect(saveStateAction, SIGNAL(triggered()), this, SLOT(saveState()));

    // load state action
    QAction * loadStateAction = new QAction("Load State", this);
    loadStateAction->setStatusTip("Load state");
    connect(loadStateAction, SIGNAL(triggered()), this, SLOT(loadState()));

    // compute image pyramid action
    QAction * computeImagePyramidAction = new QAction("Compute Image Pyramid", this);
    computeImagePyramidAction->setStatusTip("Compute image pyramid");
    connect(computeImagePyramidAction, SIGNAL(triggered()), this, SLOT(computeImagePyramid()));

    // load background content action
    QAction * backgroundAction = new QAction("Background", this);
    backgroundAction->setStatusTip("Select the background color and content");
    connect(backgroundAction, SIGNAL(triggered()), backgroundWidget_, SLOT(show()));

    // Open webbrowser action
    QAction * webbrowserAction = new QAction("Web Browser", this);
    webbrowserAction->setStatusTip("Open a web browser");
    connect(webbrowserAction, SIGNAL(triggered()), webbrowserWidget_, SLOT(show()));

    // quit action
    QAction * quitAction = new QAction("Quit", this);
    quitAction->setStatusTip("Quit application");
    connect(quitAction, SIGNAL(triggered()), this, SLOT(close()));

    // show window borders action
    QAction * showWindowBordersAction = new QAction("Show Window Borders", this);
    showWindowBordersAction->setStatusTip("Show window borders");
    showWindowBordersAction->setCheckable(true);
    showWindowBordersAction->setChecked(options_->getShowWindowBorders());
    connect(showWindowBordersAction, SIGNAL(toggled(bool)), options_.get(), SLOT(setShowWindowBorders(bool)));

    // show touch points action
    QAction * showTouchPoints = new QAction("Show Touch Points", this);
    showTouchPoints->setStatusTip("Show touch points");
    showTouchPoints->setCheckable(true);
    showTouchPoints->setChecked(options_->getShowTouchPoints());
    connect(showTouchPoints, SIGNAL(toggled(bool)), options_.get(), SLOT(setShowTouchPoints(bool)));

    // show test pattern action
    QAction * showTestPatternAction = new QAction("Show Test Pattern", this);
    showTestPatternAction->setStatusTip("Show test pattern");
    showTestPatternAction->setCheckable(true);
    showTestPatternAction->setChecked(options_->getShowTestPattern());
    connect(showTestPatternAction, SIGNAL(toggled(bool)), options_.get(), SLOT(setShowTestPattern(bool)));

    // show zoom context action
    QAction * showZoomContextAction = new QAction("Show Zoom Context", this);
    showZoomContextAction->setStatusTip("Show zoom context");
    showZoomContextAction->setCheckable(true);
    showZoomContextAction->setChecked(options_->getShowZoomContext());
    connect(showZoomContextAction, SIGNAL(toggled(bool)), options_.get(), SLOT(setShowZoomContext(bool)));

    // show streaming segments action
    QAction * showStreamingSegmentsAction = new QAction("Show Streaming Segments", this);
    showStreamingSegmentsAction->setStatusTip("Show Streaming Segments");
    showStreamingSegmentsAction->setCheckable(true);
    showStreamingSegmentsAction->setChecked(options_->getShowStreamingSegments());
    connect(showStreamingSegmentsAction, SIGNAL(toggled(bool)), options_.get(), SLOT(setShowStreamingSegments(bool)));

    // show streaming statistics action
    QAction * showStatisticsAction = new QAction("Show Statistics", this);
    showStatisticsAction->setStatusTip("Show statistics");
    showStatisticsAction->setCheckable(true);
    showStatisticsAction->setChecked(options_->getShowStatistics());
    connect(showStatisticsAction, SIGNAL(toggled(bool)), options_.get(), SLOT(setShowStatistics(bool)));

    // show window title action
    QAction* showWindowTitlesAction = new QAction( "Show Window Titles", this );
    showWindowTitlesAction->setStatusTip( "Show window titles" );
    showWindowTitlesAction->setCheckable( true );
    showWindowTitlesAction->setChecked( displayGroup_->getShowWindowTitles( ));
    connect( showWindowTitlesAction, SIGNAL( toggled( bool )),
             displayGroup_.get(), SLOT( setShowWindowTitles( bool )));
    connect( displayGroup_.get(), SIGNAL( showWindowTitlesChanged( bool )),
             showWindowTitlesAction, SLOT( setChecked( bool )));

    // enable alpha blending
    QAction* enableAlphaBlendingAction = new QAction( "Alpha Blending", this );
    enableAlphaBlendingAction->setStatusTip(
           "Enable alpha blending for transparent contents (png, svg, etc..)" );
    enableAlphaBlendingAction->setCheckable( true );
    enableAlphaBlendingAction->setChecked( options_->isAlphaBlendingEnabled( ));
    connect( enableAlphaBlendingAction, SIGNAL( toggled( bool )),
             options_.get(), SLOT( enableAlphaBlending( bool )));

    QAction * showAboutDialog = new QAction("About", this);
    showAboutDialog->setStatusTip("About DisplayCluster");
    connect(showAboutDialog, SIGNAL(triggered()), this, SLOT(openAboutWidget()));

    // add actions to menus
    fileMenu->addAction( openContentAction );
    fileMenu->addAction( openContentsDirectoryAction );
    fileMenu->addAction( loadStateAction );
    fileMenu->addAction( saveStateAction );
    fileMenu->addAction( webbrowserAction );
    fileMenu->addAction( clearContentsAction );
    fileMenu->addAction( quitAction );
    editMenu->addAction( backgroundAction );
    viewMenu->addAction( showStatisticsAction );
    viewMenu->addAction( showWindowTitlesAction );
    viewMenu->addAction( showStreamingSegmentsAction );
    viewMenu->addAction( showWindowBordersAction );
    viewMenu->addAction( showTouchPoints );
    viewMenu->addAction( showTestPatternAction );
    viewMenu->addAction( showZoomContextAction );
    viewMenu->addAction( enableAlphaBlendingAction );
    toolsMenu->addAction( computeImagePyramidAction );

    helpMenu->addAction( showAboutDialog );

    // add actions to toolbar
    toolbar->addAction(openContentAction);
    toolbar->addAction(openContentsDirectoryAction);
    toolbar->addAction(loadStateAction);
    toolbar->addAction(saveStateAction);
    toolbar->addAction(webbrowserAction);
    toolbar->addAction(clearContentsAction);
    toolbar->addAction(backgroundAction);

    // main widget / layout area
    QTabWidget * mainWidget = new QTabWidget();
    setCentralWidget(mainWidget);

    // add the local renderer group
    dggv_->setDataModel(displayGroup_);
    mainWidget->addTab((QWidget *)dggv_, "Display group 0");
    // Forward background touch events
    connect(dggv_, SIGNAL(backgroundTap(QPointF)), this, SIGNAL(hideDock()));
    connect(dggv_, SIGNAL(backgroundTapAndHold(QPointF)),
            this, SIGNAL(openDock(QPointF)));

    // create contents dock widget
    QDockWidget * contentsDockWidget = new QDockWidget("Contents", this);
    QWidget * contentsWidget = new QWidget();
    QVBoxLayout * contentsLayout = new QVBoxLayout();
    contentsWidget->setLayout(contentsLayout);
    contentsDockWidget->setWidget(contentsWidget);
    addDockWidget(Qt::LeftDockWidgetArea, contentsDockWidget);

    // add the list widget
    DisplayGroupListWidget* dglwp = new DisplayGroupListWidget(this);
    dglwp->setDataModel(displayGroup_);
    contentsLayout->addWidget(dglwp);
}

void MasterWindow::openContent()
{
    const QString filter = ContentFactory::getSupportedFilesFilterAsString();

    const QString filename = QFileDialog::getOpenFileName( this,
                                                           tr("Choose content"),
                                                           contentFolder_,
                                                           filter );
    if( filename.isEmpty( ))
        return;

    contentFolder_ = QFileInfo( filename ).absoluteDir().path();

    if( !ContentLoader( displayGroup_ ).load( filename ))
    {
        QMessageBox messageBox;
        messageBox.setText( "Unsupported file." );
        messageBox.exec();
    }
}

void MasterWindow::addContentDirectory( const QString& directoryName,
                                        unsigned int gridX,
                                        unsigned int gridY )
{
    QDir directory( directoryName );
    directory.setFilter( QDir::Files );
    directory.setNameFilters( ContentFactory::getSupportedFilesFilter( ));

    QFileInfoList list = directory.entryInfoList();

    // Prevent opening of folders with an excessively large number of items
    if( list.size() > 16 )
    {
        QString msg = "Opening this folder will create " +
                      QString::number( list.size( )) +
                      " content elements. Are you sure you want to continue?";

        typedef QMessageBox::StandardButton button;
        const button reply = QMessageBox::question( this, "Warning", msg,
                                                    QMessageBox::Yes |
                                                    QMessageBox::No );
        if ( reply != QMessageBox::Yes )
            return;
    }

    // If the grid size is unspecified, compute one large enough to hold all the elements
    if ( gridX == 0 || gridY == 0 )
        estimateGridSize( list.size(), gridX, gridY );

    unsigned int contentIndex = 0;

    const QSizeF win( displayGroup_->getCoordinates().width() / (qreal)gridX,
                      displayGroup_->getCoordinates().height() / (qreal)gridY );

    ContentLoader contentLoader( displayGroup_ );

    for( int i = 0; i < list.size() && contentIndex < gridX * gridY; ++i )
    {
        const QFileInfo& fileInfo = list.at(i);
        const QString& filename = fileInfo.absoluteFilePath();

        const unsigned int x_coord = contentIndex % gridX;
        const unsigned int y_coord = contentIndex / gridX;
        const QPointF position( x_coord * win.width() + 0.5 * win.width(),
                                y_coord * win.height() + 0.5 * win.height( ));

        if( contentLoader.load( filename, position, win ))
            ++contentIndex;
    }
}

void MasterWindow::openContentsDirectory()
{
    const QString dirName = QFileDialog::getExistingDirectory( this, QString(),
                                                               contentFolder_ );
    if( dirName.isEmpty( ))
        return;

    contentFolder_ = dirName;

    const int gridX = QInputDialog::getInt( this, "Grid X dimension",
                                            "Grid X dimension", 0, 0 );
    const int gridY = QInputDialog::getInt( this, "Grid Y dimension",
                                            "Grid Y dimension", 0, 0 );
    assert( gridX >= 0 && gridY >= 0 );

    addContentDirectory( dirName, gridX, gridY );
}

void MasterWindow::openAboutWidget()
{
    const int revision = dc::Version::getRevision();

    std::ostringstream aboutMsg;
    aboutMsg << "Current version: " << dc::Version::getString();
    aboutMsg << std::endl;
    aboutMsg << "SCM revision: " << std::hex << revision << std::dec;

    QMessageBox::about( this, "About Displaycluster", aboutMsg.str().c_str( ));
}

void MasterWindow::saveState()
{
    QString filename = QFileDialog::getSaveFileName( this, "Save State",
                                                     sessionFolder_,
                                                     STATE_FILES_FILTER );
    if( filename.isEmpty( ))
        return;

    sessionFolder_ = QFileInfo( filename ).absoluteDir().path();

    // make sure filename has .dcx extension
    if( !filename.endsWith( ".dcx" ))
    {
        put_flog( LOG_VERBOSE, "appended .dcx filename extension" );
        filename.append( ".dcx" );
    }

    if( !StateSerializationHelper( displayGroup_ ).save( filename ))
    {
        QMessageBox::warning( this, "Error", "Could not save state file.",
                              QMessageBox::Ok, QMessageBox::Ok );
    }
}

void MasterWindow::loadState()
{
    const QString filename = QFileDialog::getOpenFileName( this, "Load State",
                                                           sessionFolder_,
                                                           STATE_FILES_FILTER );
    if( filename.isEmpty( ))
        return;

    sessionFolder_ = QFileInfo( filename ).absoluteDir().path();

    loadState( filename );
}

void MasterWindow::loadState( const QString& filename )
{
    if( !StateSerializationHelper( displayGroup_ ).load( filename ))
    {
        QMessageBox::warning( this, "Error", "Could not load state file.",
                              QMessageBox::Ok, QMessageBox::Ok );
    }
}

void MasterWindow::computeImagePyramid()
{
    const QString filename = QFileDialog::getOpenFileName( this, "Select image",
                                                           contentFolder_ );
    if( filename.isEmpty( ))
        return;

    contentFolder_ = QFileInfo( filename ).absoluteDir().path();

    put_flog( LOG_DEBUG, "source image filename: %s",
              filename.toLocal8Bit().constData( ));

    put_flog( LOG_DEBUG, "target location for image pyramid folder: %s",
              contentFolder_.toLocal8Bit().constData( ));

    DynamicTexturePtr dynamicTexture( new DynamicTexture( filename ));
    if( !dynamicTexture->generateImagePyramid( contentFolder_ ))
    {
        QMessageBox::warning( this, "Error", "Image pyramid creation failed.",
                              QMessageBox::Ok, QMessageBox::Ok );
    }

    put_flog( LOG_DEBUG, "done generating pyramid" );
}

void MasterWindow::estimateGridSize( unsigned int numElem, unsigned int &gridX,
                                     unsigned int &gridY )
{
    assert( numElem > 0 );
    gridX = (unsigned int)( ceil( sqrt( numElem )));
    assert( gridX > 0 );
    gridY = ( gridX * ( gridX-1 ) >= numElem ) ? gridX-1 : gridX;
}

QStringList MasterWindow::extractValidContentUrls(const QMimeData* mimeData)
{
    QStringList pathList;

    if (mimeData->hasUrls())
    {
        QList<QUrl> urlList = mimeData->urls();

        foreach (QUrl url, urlList)
        {
            QString extension = QFileInfo(url.toLocalFile().toLower()).suffix();
            if (ContentFactory::getSupportedExtensions().contains(extension))
                pathList.append(url.toLocalFile());
        }
    }

    return pathList;
}

QStringList MasterWindow::extractFolderUrls(const QMimeData* mimeData)
{
    QStringList pathList;

    if (mimeData->hasUrls())
    {
        QList<QUrl> urlList = mimeData->urls();

        foreach (QUrl url, urlList)
        {
            if (QDir(url.toLocalFile()).exists())
                pathList.append(url.toLocalFile());
        }
    }

    return pathList;
}

QString MasterWindow::extractStateFile(const QMimeData* mimeData)
{
    QList<QUrl> urlList = mimeData->urls();
    if (urlList.size() == 1)
    {
        QUrl url = urlList[0];
        QString extension = QFileInfo(url.toLocalFile().toLower()).suffix();
        if (extension == "dcx")
            return url.toLocalFile();
    }
    return QString();
}

void MasterWindow::dragEnterEvent(QDragEnterEvent* dragEvent)
{
    const QStringList& pathList = extractValidContentUrls(dragEvent->mimeData());
    const QStringList& dirList = extractFolderUrls(dragEvent->mimeData());
    const QString& stateFile = extractStateFile(dragEvent->mimeData());

    if (!pathList.empty() || !dirList.empty() || !stateFile.isNull())
    {
        dragEvent->acceptProposedAction();
    }
}

void MasterWindow::dropEvent( QDropEvent* dropEvt )
{
    const QStringList& urls = extractValidContentUrls( dropEvt->mimeData( ));
    ContentLoader loader( displayGroup_ );
    foreach( QString url, urls )
        loader.load( url );

    const QStringList& folders = extractFolderUrls( dropEvt->mimeData( ));
    if( !folders.isEmpty( ))
        addContentDirectory( folders[0] ); // Only one directory at a time

    const QString& stateFile = extractStateFile( dropEvt->mimeData( ));
    if( !stateFile.isNull( ))
        loadState( stateFile );

    dropEvt->acceptProposedAction();
}
