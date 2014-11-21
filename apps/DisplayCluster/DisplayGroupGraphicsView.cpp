/*********************************************************************/
/* Copyright (c) 2011 - 2012, The University of Texas at Austin.     */
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

#include "DisplayGroupGraphicsView.h"

#include "DisplayGroup.h"
#include "DisplayGroupGraphicsScene.h"
#include "ContentWindowGraphicsItem.h"
#include "ContentWindow.h"

#include "gestures/PanGesture.h"
#include "gestures/PanGestureRecognizer.h"
#include "gestures/PinchGesture.h"
#include "gestures/PinchGestureRecognizer.h"

#include <boost/foreach.hpp>

#define VIEW_MARGIN 0.05f

DisplayGroupGraphicsView::DisplayGroupGraphicsView( const Configuration& config,
                                                    QWidget* parent_ )
    : QGraphicsView( parent_ )
{
    setScene( new DisplayGroupGraphicsScene( config, this ));
    setAlignment( Qt::AlignLeft | Qt::AlignTop );

    setInteractive( true );
    setDragMode( QGraphicsView::RubberBandDrag );
    setAcceptDrops( true );

    grabGestures();
}

DisplayGroupGraphicsView::~DisplayGroupGraphicsView()
{
}

void DisplayGroupGraphicsView::setModel( DisplayGroupPtr displayGroup )
{
    if( displayGroup_ )
    {
        displayGroup_->disconnect( this );
        static_cast< DisplayGroupGraphicsScene* >( scene( ))->clearAndRestoreBackground();
        grabGestures();
    }

    displayGroup_ = displayGroup;
    if( !displayGroup_ )
        return;

    ContentWindowPtrs contentWindows = displayGroup_->getContentWindows();
    BOOST_FOREACH( ContentWindowPtr contentWindow, contentWindows )
    {
        addContentWindow( contentWindow );
    }

    connect( displayGroup_.get(),
             SIGNAL( contentWindowAdded( ContentWindowPtr )),
             this, SLOT( addContentWindow( ContentWindowPtr )));
    connect( displayGroup_.get(),
             SIGNAL( contentWindowRemoved( ContentWindowPtr )),
            this, SLOT(removeContentWindow(ContentWindowPtr)));
    connect( displayGroup_.get(),
             SIGNAL( contentWindowMovedToFront( ContentWindowPtr )),
             this, SLOT( moveContentWindowToFront( ContentWindowPtr )));
}

void DisplayGroupGraphicsView::grabGestures()
{
    viewport()->grabGesture( Qt::TapGesture );
    viewport()->grabGesture( Qt::TapAndHoldGesture );
    viewport()->grabGesture( Qt::SwipeGesture );
}

bool DisplayGroupGraphicsView::viewportEvent( QEvent* evt )
{
    if( evt->type() == QEvent::Gesture )
    {
        QGestureEvent* gesture = static_cast< QGestureEvent* >( evt );
        gestureEvent( gesture );
        return QGraphicsView::viewportEvent( gesture );
    }
    return QGraphicsView::viewportEvent( evt );
}

void DisplayGroupGraphicsView::gestureEvent( QGestureEvent* evt )
{
    QGesture* gesture = 0;

    if( ( gesture = evt->gesture( Qt::SwipeGesture )))
    {
        evt->accept( Qt::SwipeGesture );
        swipe( static_cast< QSwipeGesture* >( gesture ));
    }
    else if( ( gesture = evt->gesture( PanGestureRecognizer::type( ))))
    {
        evt->accept( PanGestureRecognizer::type( ));
        pan( static_cast< PanGesture* >( gesture ));
    }
    else if( ( gesture = evt->gesture( PinchGestureRecognizer::type( ))))
    {
        evt->accept( PinchGestureRecognizer::type( ));
        pinch( static_cast< PinchGesture* >( gesture ));
    }
    else if( ( gesture = evt->gesture( Qt::TapGesture )))
    {
        evt->accept( Qt::TapGesture );
        tap( static_cast< QTapGesture* >( gesture ));
    }
    else if( ( gesture = evt->gesture( Qt::TapAndHoldGesture )))
    {
        evt->accept( Qt::TapAndHoldGesture );
        tapAndHold( static_cast< QTapAndHoldGesture* >( gesture ));
    }
}

void DisplayGroupGraphicsView::swipe( QSwipeGesture* )
{
    std::cout << "SWIPE VIEW" << std::endl;
}

void DisplayGroupGraphicsView::pan( PanGesture* )
{
}

void DisplayGroupGraphicsView::pinch( PinchGesture* )
{
}

void DisplayGroupGraphicsView::tap( QTapGesture* gesture )
{
    if( gesture->state() != Qt::GestureFinished )
        return;

    const QPointF position = getNormalizedPosition( gesture );

    if ( isOnBackground( position ))
        emit backgroundTap( position );
}

void DisplayGroupGraphicsView::tapAndHold( QTapAndHoldGesture* gesture )
{
    if( gesture->state() != Qt::GestureFinished )
        return;

    const QPointF position = getNormalizedPosition( gesture );

    if ( isOnBackground( position ))
        emit backgroundTapAndHold( position );
}

void DisplayGroupGraphicsView::resizeEvent( QResizeEvent* resizeEvt )
{
    const QSizeF& sceneSize = scene()->sceneRect().size();

    QSizeF windowSize( width(), height( ));
    windowSize.scale( sceneSize, Qt::KeepAspectRatioByExpanding );
    windowSize = windowSize * ( 1.f + VIEW_MARGIN );

    // Center the scene in the view
    setSceneRect( -0.5f * (windowSize.width() - sceneSize.width()),
                  -0.5f * (windowSize.height() - sceneSize.height()),
                  windowSize.width(), windowSize.height( ));
    fitInView( sceneRect( ));

    QGraphicsView::resizeEvent( resizeEvt );
}

QPointF DisplayGroupGraphicsView::getNormalizedPosition( const QGesture* gesture ) const
{
    // Gesture::hotSpot() is the position (in pixels) in global SCREEN coordinates.
    // SCREEN is the Display where the Rank0 Qt Window lives.

    // Some gestures also have a position attribute which is inconsistent between
    // event types. For almost all gestures it is equal to the hotSpot attribute.
    // For the special case of the QTapGesture, it is actually the position in pixels
    // in DisplayWall coordinates...

    // The widgetPos is the position (in pixels) in the QGraphicsView.
    const QPoint widgetPos = mapFromGlobal( QPoint( gesture->hotSpot().x(),
                                                    gesture->hotSpot().y( )));
    // The returned value is the normalized position in the QGraphicsView.
    return mapToScene( widgetPos );
}

bool DisplayGroupGraphicsView::isOnBackground( const QPointF& position ) const
{
    const QGraphicsItem* item = scene()->itemAt( position );
    return dynamic_cast< const ContentWindowGraphicsItem* >( item ) == 0;
}

void DisplayGroupGraphicsView::addContentWindow( ContentWindowPtr contentWindow )
{
    assert( displayGroup_ );

    ContentWindowGraphicsItem* cwgi = new ContentWindowGraphicsItem( contentWindow );
    scene()->addItem( static_cast< QGraphicsItem* >( cwgi ) );

    connect( cwgi, SIGNAL( moveToFront( ContentWindowPtr )),
             displayGroup_.get(),
             SLOT( moveContentWindowToFront( ContentWindowPtr )));

    connect( cwgi, SIGNAL( close( ContentWindowPtr )),
             displayGroup_.get(),
             SLOT( removeContentWindow( ContentWindowPtr )));
}

void DisplayGroupGraphicsView::removeContentWindow( ContentWindowPtr contentWindow )
{
    assert( displayGroup_ );

    QList< QGraphicsItem* > itemsList = scene()->items();

    foreach( QGraphicsItem* item, itemsList )
    {
        ContentWindowGraphicsItem* cwgi = dynamic_cast< ContentWindowGraphicsItem* >( item );
        if( cwgi && cwgi->getContentWindow() == contentWindow )
            scene()->removeItem( item );
    }

    // Qt WAR: when all items with grabbed gestures are removed, the viewport
    // also looses any registered gestures, which harms our dock to open...
    // <qt-source>/qgraphicsscene.cpp::ungrabGesture called in removeItemHelper()
    // Always call grabGestures() to prevent this situation from occuring.
    grabGestures();
}

void DisplayGroupGraphicsView::moveContentWindowToFront( ContentWindowPtr contentWindow )
{
    assert( displayGroup_ );

    QList< QGraphicsItem* > itemsList = scene()->items();

    foreach( QGraphicsItem* item, itemsList )
    {
        ContentWindowGraphicsItem* cwgi = dynamic_cast< ContentWindowGraphicsItem* >( item );
        if( cwgi && cwgi->getContentWindow() == contentWindow )
            cwgi->setZToFront();
    }
}
