/*********************************************************************/
/* Copyright (c) 2011 - 2012, The University of Texas at Austin.     */
/* Copyright (c) 2013-2015, EPFL/Blue Brain Project                  */
/*                     Raphael.Dumusc@epfl.ch                        */
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

#include "Content.h"

IMPLEMENT_SERIALIZE_FOR_XML( Content )

qreal Content::maxScale_ = 3.0;

Content::Content( const QString& uri )
    : _uri( uri )
{
}

const QString& Content::getURI() const
{
    return _uri;
}

QSize Content::getDimensions() const
{
    return _size;
}

QSize Content::getMinDimensions() const
{
    if( _sizeHints.minWidth == deflect::SizeHints::UNSPECIFIED_SIZE ||
        _sizeHints.minHeight == deflect::SizeHints::UNSPECIFIED_SIZE )
    {
        return UNDEFINED_SIZE;
    }
    return QSize( _sizeHints.minWidth, _sizeHints.minHeight );
}

QSize Content::getPreferredDimensions() const
{
    if( _sizeHints.preferredWidth == deflect::SizeHints::UNSPECIFIED_SIZE ||
        _sizeHints.preferredHeight == deflect::SizeHints::UNSPECIFIED_SIZE )
    {
        return getDimensions();
    }
    return QSize( _sizeHints.preferredWidth, _sizeHints.preferredHeight );
}

QSize Content::getMaxDimensions() const
{
    if( _sizeHints.maxWidth == deflect::SizeHints::UNSPECIFIED_SIZE ||
        _sizeHints.maxHeight == deflect::SizeHints::UNSPECIFIED_SIZE )
    {
        return getDimensions().isValid() ? getDimensions() * getMaxScale()
                                         : UNDEFINED_SIZE;
    }
    return QSize( _sizeHints.maxWidth, _sizeHints.maxHeight );
}

void Content::setSizeHints( const deflect::SizeHints& sizeHints )
{
    if( _sizeHints == sizeHints )
        return;
    _sizeHints = sizeHints;
    emit modified();
}

void Content::setMaxScale( const qreal value )
{
    if( value > 0 )
        maxScale_ = value;
}

qreal Content::getMaxScale()
{
    return maxScale_;
}

void Content::setDimensions( const QSize& dimensions )
{
    if( _size == dimensions )
        return;

    _size = dimensions;

    emit modified();
}

qreal Content::getAspectRatio() const
{
    if( _size.height() == 0 )
        return 0.0;
    return (qreal)_size.width() / (qreal)_size.height();
}

bool Content::hasFixedAspectRatio() const
{
    return true;
}

ContentActionsModel* Content::getActions()
{
    return &_actions;
}
