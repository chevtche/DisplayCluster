/*********************************************************************/
/* Copyright (c) 2015, EPFL/Blue Brain Project                       */
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

#include "WallWindow.h"

#include "TestPattern.h"
#include "WallScene.h"

WallWindow::WallWindow( QQmlEngine* engine_ )
    : QQuickView( engine_, nullptr )
    , _glContext( new QOpenGLContext )
    , _blockUpdates( false )
    , _isExposed( false )
{
    setSurfaceType( QWindow::OpenGLSurface );
    setResizeMode( SizeRootObjectToView );
    setFlags( Qt::FramelessWindowHint );
}

QOpenGLContext& WallWindow::getGLContext()
{
    return *_glContext;
}

WallScene& WallWindow::getScene()
{
    return *_scene;
}

void WallWindow::createScene( const QPoint& pos )
{
    _scene = make_unique<WallScene>( *this, pos );
}

void WallWindow::setTestPattern( TestPatternPtr testPattern )
{
    _testPattern = testPattern;
}

TestPatternPtr WallWindow::getTestPattern()
{
    return _testPattern;
}

void WallWindow::setBlockDrawCalls( const bool enable )
{
    _blockUpdates = enable;
}

//bool WallWindow::isExposed() const
//{
//    return isExposed_;
//}

//void WallWindow::drawForeground( QPainter* painter, const QRectF& rect_ )
//{
//    if( testPattern_ && testPattern_->isVisible( ))
//    {
//        testPattern_->draw( painter, rect_ );
//        return;
//    }

//    QGraphicsView::drawForeground( painter, rect_ );
//}

//void WallWindow::paintEvent( QPaintEvent* event_ )
//{
//    if( blockUpdates_ )
//        return;

//    isExposed_ = true;

//    QGraphicsView::paintEvent( event_ );
//}
