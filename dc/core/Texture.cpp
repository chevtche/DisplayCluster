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

#include "Texture.h"

#include "log.h"
#include "ContentWindow.h"

#include <QtGui/QImageReader>

Texture::Texture( const QString& uri )
    : uri_( uri )
{
    const QImageReader imageReader( uri_ );
    if( !imageReader.canRead( ))
    {
        put_flog( LOG_ERROR, "error loading: '%s'",
                  uri_.toLocal8Bit().constData( ));
        return;
    }
    imageSize_ = imageReader.size();
}

bool Texture::generateTexture()
{
    const QImage image( uri_ );
    if( image.isNull( ))
    {
        put_flog( LOG_ERROR, "error loading: '%s'",
                  uri_.toLocal8Bit().constData( ));
        return false;
    }
    quad_.enableAlphaBlending( image.hasAlphaChannel( ));

    return texture_.init( image, GL_BGRA, true );
}

void Texture::render()
{
    if( !texture_.isValid( ))
        return;

    quad_.render();
}

void Texture::renderPreview()
{
    if( !texture_.isValid( ))
        return;

    previewQuad_.render();
}

void Texture::preRenderUpdate( ContentWindowPtr window, const QRect& )
{
    if( !texture_.isValid( ))
        generateTexture();

    quad_.setTexCoords( window->getZoomRect( ));
    quad_.setTexture( texture_.getTextureId( ));

    previewQuad_.setTexture( texture_.getTextureId( ));
}
