/*********************************************************************/
/* Copyright (c) 2013-2015, EPFL/Blue Brain Project                  */
/*                     Raphael Dumusc <raphael.dumusc@epfl.ch>       */
/*                     Daniel.Nachbaur@epfl.ch                       */
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

#include "ZoomInteractionDelegate.h"
#include "ContentWindow.h"

#include <QTransform>

#define MIN_ZOOM 1.0

ZoomInteractionDelegate::ZoomInteractionDelegate( ContentWindow& contentWindow )
    : ContentInteractionDelegate( contentWindow )
{
}

ZoomInteractionDelegate::~ZoomInteractionDelegate()
{
}

void ZoomInteractionDelegate::pan( const QPointF position, const QPointF delta )
{
    Q_UNUSED( position );
    _moveZoomRect( delta );
}

void ZoomInteractionDelegate::pinch( QPointF position,
                                     const qreal pixelDelta )
{
    QRectF contentRect = _toContentRect( _contentWindow.getZoomRect( ));

    position -= getWindowCoord().topLeft();

    QSizeF newSize = contentRect.size();
    newSize.scale( newSize.width() + pixelDelta,
                   newSize.height() + pixelDelta,
                   pixelDelta < 0 ? Qt::KeepAspectRatio
                                  : Qt::KeepAspectRatioByExpanding );

    contentRect = ContentWindowController::scaleRectAroundPosition( contentRect,
                                                                    position,
                                                                    newSize );
    _checkAndApply( _toZoomRect( contentRect ));
}

void ZoomInteractionDelegate::adjustZoomToContentAspectRatio()
{
    QRectF contentRect = _toContentRect( _contentWindow.getZoomRect( ));
    QSizeF contentSize = _contentWindow.getContent()->getDimensions();
    contentSize = contentSize / contentSize.width();
    contentSize.scale( contentRect.size(), Qt::KeepAspectRatio );
    contentRect.setSize( contentSize );

    _checkAndApply( _toZoomRect( contentRect ));
}

void ZoomInteractionDelegate::_checkAndApply( QRectF zoomRect )
{
    _constrainZoomLevel( zoomRect );
    _constraintPosition( zoomRect );
    _contentWindow.setZoomRect( zoomRect );
}

void ZoomInteractionDelegate::_moveZoomRect( const QPointF& sceneDelta ) const
{
    QRectF zoomRect = _contentWindow.getZoomRect();
    const qreal zoom = zoomRect.width();
    const QPointF normalizedDelta = getNormalizedPoint( sceneDelta ) * zoom;
    zoomRect.translate( -normalizedDelta );

    _constraintPosition( zoomRect );
    _contentWindow.setZoomRect( zoomRect );
}

void ZoomInteractionDelegate::_constrainZoomLevel( QRectF& zoomRect ) const
{
    const QSizeF maxZoom = _getMaxZoom();

    // constrain max zoom
    if( zoomRect.width() < maxZoom.width() || zoomRect.height() < maxZoom.height( ))
        zoomRect = _contentWindow.getZoomRect();

    const QSizeF minZoom = _getMinZoom();

    // constrain min zoom
    if( zoomRect.width() > minZoom.width( ))
        zoomRect.setWidth( minZoom.width( ));
    if( zoomRect.height() > minZoom.height( ))
        zoomRect.setHeight( minZoom.height( ));
}

void ZoomInteractionDelegate::_constraintPosition( QRectF& zoomRect ) const
{
    if( zoomRect.left() < 0.0 )
        zoomRect.moveLeft( 0.0 );
    if( zoomRect.right() > 1.0 )
        zoomRect.moveRight( 1.0 );
    if( zoomRect.top() < 0.0 )
        zoomRect.moveTop( 0.0 );
    if( zoomRect.bottom() > 1.0 )
        zoomRect.moveBottom( 1.0 );
}

QSizeF getMaxContentSize( ContentWindow& window )
{
    if( window.getController( ))
        return window.getController()->getMaxContentSize();
    return window.getContent()->getMaxDimensions();
}

QSizeF ZoomInteractionDelegate::_getMaxZoom() const
{
    const QSizeF content( getMaxContentSize( _contentWindow ));
    const QSizeF window( getWindowCoord().size( ));
    return QSizeF( window.width() / content.width(),
                   window.height() / content.height( ));
}

QSizeF ZoomInteractionDelegate::_getMinZoom() const
{
    const QSizeF window( getWindowCoord().size( ));
    QSizeF content( _contentWindow.getContent()->getDimensions( ));
    content.scale( window, Qt::KeepAspectRatioByExpanding );
    const qreal windowAR = window.width() / window.height();
    const qreal contentAR = content.width() / content.height();

    if( contentAR > windowAR )
        return QSizeF( MIN_ZOOM * window.width() / content.width(), MIN_ZOOM );
    return QSizeF( MIN_ZOOM, MIN_ZOOM * window.height() / content.height( ));
}

QRectF ZoomInteractionDelegate::_toContentRect( const QRectF& zoomRect ) const
{
    const QRectF& window = getWindowCoord();

    const qreal w = window.width() / zoomRect.width();
    const qreal h = window.height() / zoomRect.height();

    const qreal posX = -zoomRect.x() * w;
    const qreal posY = -zoomRect.y() * h;

    return QRectF( posX, posY, w, h );
}

QRectF ZoomInteractionDelegate::_toZoomRect( const QRectF& contentRect ) const
{
    const QRectF& window = getWindowCoord();
    const qreal w = window.width() / contentRect.width();
    const qreal h = window.height() / contentRect.height();

    const qreal posX = -contentRect.x() / contentRect.width();
    const qreal posY = -contentRect.y() / contentRect.height();

    return QRectF( posX, posY, w, h );
}
