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

#include "ContentWindowGraphicsItem.h"

#include "ContentWindow.h"
#include "Content.h"
#include "ContentInteractionDelegate.h"

#include "gestures/DoubleTapGestureRecognizer.h"
#include "gestures/PanGestureRecognizer.h"
#include "gestures/PinchGestureRecognizer.h"

#include <QtGui/QPainter>
#include <QtGui/QGraphicsScene>
#include <QtGui/QGraphicsView>

#include <QEvent>
#include <QGestureEvent>

#define STD_WHEEL_DELTA 120 // Common value for the delta of mouse wheel events

qreal ContentWindowGraphicsItem::zCounter_ = 0;

ContentWindowGraphicsItem::ContentWindowGraphicsItem( ContentWindowPtr contentWindow )
    : contentWindow_( contentWindow )
    , resizing_( false )
    , moving_( false )
{
    assert( contentWindow_ );

    connect( contentWindow_.get(), SIGNAL( coordinatesAboutToChange( )),
             this, SLOT( prepareToChangeGeometry( )));

    setFlag( QGraphicsItem::ItemIsMovable, true );
    setFlag( QGraphicsItem::ItemIsFocusable, true );

    // new items at the front
    // we assume that interface items will be constructed in depth order so
    // this produces the correct result...
    setZToFront();

    grabGesture( DoubleTapGestureRecognizer::type( ));
    grabGesture( PanGestureRecognizer::type( ));
    grabGesture( PinchGestureRecognizer::type( ));
    grabGesture( Qt::SwipeGesture );
    grabGesture( Qt::TapAndHoldGesture );
    grabGesture( Qt::TapGesture );
}

ContentWindowGraphicsItem::~ContentWindowGraphicsItem()
{
}

ContentWindowPtr ContentWindowGraphicsItem::getContentWindow() const
{
    return contentWindow_;
}

void ContentWindowGraphicsItem::paint( QPainter* painter,
                                       const QStyleOptionGraphicsItem*,
                                       QWidget* )
{
    drawFrame_( painter );
    drawCloseButton_( painter );
    drawResizeIndicator_( painter );
    drawFullscreenButton_( painter );
    drawMovieControls_( painter );
    drawTextLabel_( painter );
    drawWindowInfo_( painter );
}

void ContentWindowGraphicsItem::setZToFront()
{
    setZValue( ++zCounter_ );
}

void ContentWindowGraphicsItem::prepareToChangeGeometry()
{
    prepareGeometryChange();
}

QRectF ContentWindowGraphicsItem::boundingRect() const
{
    return contentWindow_->getAbsCoordinates();
}

bool ContentWindowGraphicsItem::sceneEvent( QEvent* event_ )
{
    switch( event_->type( ))
    {
    case QEvent::Gesture:
        emit moveToFront( contentWindow_ );
        contentWindow_->getInteractionDelegate().gestureEvent( static_cast< QGestureEvent* >( event_ ));
        return true;
    case QEvent::KeyPress:
        // Override default behaviour to process TAB key events
        keyPressEvent( static_cast< QKeyEvent* >( event_ ));
        return true;
    default:
        return QGraphicsObject::sceneEvent( event_ );
    }
}

void ContentWindowGraphicsItem::mouseMoveEvent( QGraphicsSceneMouseEvent* event_ )
{
    if( contentWindow_->selected( ))
    {
        contentWindow_->getInteractionDelegate().mouseMoveEvent( event_ );
        return;
    }

    if( event_->buttons().testFlag( Qt::LeftButton ))
    {
        if( resizing_ )
        {
            QRectF coordinates = boundingRect();
            coordinates.setBottomRight( event_->scenePos( ));

            float targetAR = contentWindow_->getContent()->getAspectRatio();

            const float eventCoordAR = coordinates.width() / coordinates.height();
            if( eventCoordAR < targetAR )
                contentWindow_->setSize( coordinates.width(),
                                         coordinates.width() / targetAR );
            else
                contentWindow_->setSize( coordinates.height() * targetAR,
                                         coordinates.height( ));
        }
        else
        {
            const QPointF delta = event_->scenePos() - event_->lastScenePos();
            const QPointF newPos = boundingRect().topLeft() + delta;

            contentWindow_->setPosition( newPos.x(), newPos.y( ));
        }
    }
}

void ContentWindowGraphicsItem::mousePressEvent( QGraphicsSceneMouseEvent* event_ )
{
    // on Mac we've seen that mouse events can go to the wrong graphics item
    // this is due to the bug: https://bugreports.qt.nokia.com/browse/QTBUG-20493
    // here we ignore the event if it shouldn't have been sent to us, which ensures
    // it will go to the correct item...
    if( !boundingRect().contains( event_->pos( )))
    {
        event_->ignore();
        return;
    }

    if( hitCloseButton( event_->pos( )))
    {
        emit close( contentWindow_ );
        return;
    }

    emit moveToFront( contentWindow_ );

    if ( contentWindow_->selected( ))
    {
        contentWindow_->getInteractionDelegate().mousePressEvent( event_ );
        return;
    }

    const ControlState controlState = contentWindow_->getControlState();
    contentWindow_->getContent()->blockAdvance( true );

    if( hitResizeButton( event_->pos( )))
        resizing_ = true;

    else if( hitFullscreenButton( event_->pos( )))
        contentWindow_->toggleFullscreen();

    else if( hitPauseButton( event_->pos( )))
        contentWindow_->setControlState( ControlState( controlState ^ STATE_PAUSED ));

    else if( hitLoopButton( event_->pos( )))
        contentWindow_->setControlState( ControlState( controlState ^ STATE_LOOP ));

    else
        moving_ = true;

    QGraphicsItem::mousePressEvent( event_ );
}

void ContentWindowGraphicsItem::mouseDoubleClickEvent( QGraphicsSceneMouseEvent* event_ )
{
    // on Mac we've seen that mouse events can go to the wrong graphics item
    // this is due to the bug: https://bugreports.qt.nokia.com/browse/QTBUG-20493
    // here we ignore the event if it shouldn't have been sent to us, which ensures
    // it will go to the correct item...
    if( !boundingRect().contains( event_->pos( )))
    {
        event_->ignore();
        return;
    }

    contentWindow_->toggleWindowState();

    QGraphicsItem::mouseDoubleClickEvent( event_ );
}

void ContentWindowGraphicsItem::mouseReleaseEvent( QGraphicsSceneMouseEvent* event_ )
{
    resizing_ = false;
    moving_ = false;

    contentWindow_->getContent()->blockAdvance( false );

    if( contentWindow_->selected( ))
        contentWindow_->getInteractionDelegate().mouseReleaseEvent( event_ );

    QGraphicsItem::mouseReleaseEvent( event_ );
}

void ContentWindowGraphicsItem::wheelEvent( QGraphicsSceneWheelEvent* event_ )
{
    // on Mac we've seen that mouse events can go to the wrong graphics item
    // this is due to the bug: https://bugreports.qt.nokia.com/browse/QTBUG-20493
    // here we ignore the event if it shouldn't have been sent to us, which ensures
    // it will go to the correct item...
    if( !boundingRect().contains( event_->pos( )))
    {
        event_->ignore();
        return;
    }

    if ( contentWindow_->selected( ))
        contentWindow_->getInteractionDelegate().wheelEvent( event_ );
    else
        contentWindow_->scaleSize( 1. + (double)event_->delta() / ( 10. * STD_WHEEL_DELTA ));
}

void ContentWindowGraphicsItem::keyPressEvent( QKeyEvent* event_ )
{
    if( contentWindow_->selected( ))
        contentWindow_->getInteractionDelegate().keyPressEvent( event_ );
}

void ContentWindowGraphicsItem::keyReleaseEvent( QKeyEvent* event_ )
{
    if( contentWindow_->selected( ))
        contentWindow_->getInteractionDelegate().keyReleaseEvent( event_ );
}

void ContentWindowGraphicsItem::getButtonDimensions( float& width, float& height ) const
{
    width = 0.125f * scene()->width();
    width = std::min( width, std::min( 0.45f * (float)boundingRect().height(),
                                       0.45f * (float)boundingRect().width( )));
    height = width;
}

bool ContentWindowGraphicsItem::hitCloseButton( const QPointF& hitPos ) const
{
    float buttonWidth, buttonHeight;
    getButtonDimensions( buttonWidth, buttonHeight );
    const QRectF r = boundingRect();

    return ( fabs( ( r.x() + r.width( )) - hitPos.x( )) <= buttonWidth &&
             fabs( r.y() - hitPos.y( )) <= buttonHeight );
}

bool ContentWindowGraphicsItem::hitResizeButton(const QPointF& hitPos ) const
{
    float buttonWidth, buttonHeight;
    getButtonDimensions( buttonWidth, buttonHeight );
    const QRectF r = boundingRect();

    return ( fabs( ( r.x() + r.width( )) - hitPos.x( )) <= buttonWidth &&
             fabs( ( r.y() + r.height( )) - hitPos.y( )) <= buttonHeight );
}

bool ContentWindowGraphicsItem::hitFullscreenButton(const QPointF& hitPos) const
{
    float buttonWidth, buttonHeight;
    getButtonDimensions( buttonWidth, buttonHeight );
    const QRectF r = boundingRect();

    return ( fabs( r.x() - hitPos.x( )) <= buttonWidth &&
             fabs( ( r.y() + r.height( )) - hitPos.y( )) <= buttonHeight );
}

bool ContentWindowGraphicsItem::hitPauseButton(const QPointF& hitPos) const
{
    float buttonWidth, buttonHeight;
    getButtonDimensions( buttonWidth, buttonHeight );
    const QRectF r = boundingRect();

    return ( fabs( ( ( r.x() + r.width( )) / 2) - hitPos.x() - buttonWidth ) <= buttonWidth &&
             fabs( ( r.y() + r.height( )) - hitPos.y( )) <= buttonHeight );
}

bool ContentWindowGraphicsItem::hitLoopButton(const QPointF& hitPos) const
{
    float buttonWidth, buttonHeight;
    getButtonDimensions( buttonWidth, buttonHeight );
    const QRectF r = boundingRect();

    return ( fabs( ( ( r.x() + r.width( )) / 2) - hitPos.x( )) <= buttonWidth &&
             fabs( ( r.y() + r.height( )) - hitPos.y( )) <= buttonHeight );
}

void ContentWindowGraphicsItem::drawCloseButton_( QPainter* painter )
{
    float buttonWidth, buttonHeight;
    getButtonDimensions( buttonWidth, buttonHeight );
    const QRectF coordinates = boundingRect();

    QRectF closeRect(coordinates.x() + coordinates.width() - buttonWidth,
                     coordinates.y(), buttonWidth, buttonHeight);
    QPen pen;
    pen.setColor(QColor(255,0,0));
    painter->setPen(pen);
    painter->drawRect(closeRect);
    painter->drawLine(QPointF(coordinates.x() + coordinates.width() - buttonWidth, coordinates.y()),
                      QPointF(coordinates.x() + coordinates.width(), coordinates.y() + buttonHeight));
    painter->drawLine(QPointF(coordinates.x() + coordinates.width(), coordinates.y()),
                      QPointF(coordinates.x() + coordinates.width() - buttonWidth, coordinates.y() + buttonHeight));
}

void ContentWindowGraphicsItem::drawResizeIndicator_( QPainter* painter )
{
    float buttonWidth, buttonHeight;
    getButtonDimensions( buttonWidth, buttonHeight );
    const QRectF coordinates = boundingRect();

    QRectF resizeRect(coordinates.x() + coordinates.width() - buttonWidth,
                      coordinates.y() + coordinates.height() - buttonHeight,
                      buttonWidth, buttonHeight);
    QPen pen;
    pen.setColor(QColor(128,128,128));
    painter->setPen(pen);
    painter->drawRect(resizeRect);
    painter->drawLine(QPointF(coordinates.x() + coordinates.width(),
                              coordinates.y() + coordinates.height() - buttonHeight),
                      QPointF(coordinates.x() + coordinates.width() - buttonWidth,
                              coordinates.y() + coordinates.height()));
}

void ContentWindowGraphicsItem::drawFullscreenButton_( QPainter* painter )
{
    float buttonWidth, buttonHeight;
    getButtonDimensions( buttonWidth, buttonHeight );
    const QRectF coordinates = boundingRect();

    QRectF fullscreenRect(coordinates.x(),
                          coordinates.y() + coordinates.height() - buttonHeight,
                          buttonWidth, buttonHeight);
    QPen pen;
    pen.setColor(QColor(128,128,128));
    painter->setPen(pen);
    painter->drawRect(fullscreenRect);
}

void ContentWindowGraphicsItem::drawMovieControls_( QPainter* painter )
{
    if( contentWindow_->getContent()->getType() != CONTENT_TYPE_MOVIE )
        return;

    float buttonWidth, buttonHeight;
    getButtonDimensions( buttonWidth, buttonHeight );
    const QRectF coordinates = boundingRect();
    const ControlState controlState = contentWindow_->getControlState();

    // play/pause
    QRectF playPauseRect( coordinates.x() + coordinates.width()/2 - buttonWidth,
                          coordinates.y() + coordinates.height() - buttonHeight,
                          buttonWidth, buttonHeight );
    QPen pen;
    pen.setColor( QColor( controlState & STATE_PAUSED ? 128 : 200, 0, 0 ));
    painter->setPen(pen);
    painter->fillRect(playPauseRect, pen.color());

    // loop
    QRectF loopRect( coordinates.x() + coordinates.width()/2,
                     coordinates.y() + coordinates.height() - buttonHeight,
                     buttonWidth, buttonHeight );
    pen.setColor( QColor( 0, controlState & STATE_LOOP ? 200 : 128, 0 ));
    painter->setPen( pen );
    painter->fillRect( loopRect, pen.color( ));
}

void ContentWindowGraphicsItem::drawTextLabel_( QPainter* painter )
{
    float buttonWidth, buttonHeight;
    getButtonDimensions( buttonWidth, buttonHeight );

    const QString label( contentWindow_->getContent()->getURI( ));
    const QString labelSection = label.section( "/", -1, -1 ).prepend( " " );

    QFont font;
    font.setPixelSize( 0.25f * buttonHeight );
    painter->setFont( font );

    QPen pen;
    pen.setColor( QColor( Qt::black ));
    painter->setPen( pen );

    QRectF textBoundingRect = boundingRect();
    textBoundingRect.setWidth( textBoundingRect.width() - buttonWidth );

    painter->drawText( textBoundingRect, Qt::AlignLeft | Qt::AlignTop,
                       labelSection );
}

void ContentWindowGraphicsItem::drawWindowInfo_( QPainter* painter )
{
    const QRectF coordinates = boundingRect();

    const QString coordinatesLabel = QString(" (") +
                                     QString::number( coordinates.x(), 'f', 2 ) +
                                     QString(" ,") +
                                     QString::number( coordinates.y(), 'f', 2 ) +
                                     QString(", ") +
                                     QString::number( coordinates.width(), 'f', 2 ) +
                                     QString(", ") +
                                     QString::number( coordinates.height(), 'f', 2 ) +
                                     QString(")\n");

    double centerX, centerY;
    contentWindow_->getCenter( centerX, centerY );
    const double zoom = contentWindow_->getZoom();
    const QString zoomCenterLabel = QString(" zoom = ") +
                                    QString::number( zoom, 'f', 2 ) +
                                    QString(" @ (") +
                                    QString::number( centerX, 'f', 2 ) +
                                    QString(", ") +
                                    QString::number( centerY, 'f', 2 ) +
                                    QString(")");

    const QString windowInfoLabel = coordinatesLabel + zoomCenterLabel;

    float buttonWidth, buttonHeight;
    getButtonDimensions( buttonWidth, buttonHeight );

    QRectF textBoundingRect = QRectF( coordinates.x() + buttonWidth,
                                      coordinates.y(),
                                      coordinates.width() - 2.f * buttonWidth,
                                      coordinates.height( ));
    QPen pen;
    pen.setColor( QColor( Qt::black ));
    painter->setPen( pen );

    QFont font;
    font.setPixelSize( 0.15f * buttonHeight );
    painter->setFont( font );
    painter->drawText( textBoundingRect, Qt::AlignLeft | Qt::AlignBottom,
                       windowInfoLabel );
}

void ContentWindowGraphicsItem::drawFrame_( QPainter* painter )
{
    QPen pen;
    if( contentWindow_->selected( ))
        pen.setColor( QColor( Qt::red ));
    else
        pen.setColor( QColor( Qt::black ));

    painter->setPen( pen );
    painter->setBrush( QBrush( QColor( 0, 0, 0, 128 )));
    painter->drawRect( boundingRect( ));
}
