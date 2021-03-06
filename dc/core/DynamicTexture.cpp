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

#include "DynamicTexture.h"

#include "log.h"
#include "ContentWindow.h"

#include <fstream>
#include <boost/tokenizer.hpp>

#include <QDir>
#include <QImageReader>
#include <QtConcurrentRun>

#ifdef __APPLE__
    #include <OpenGL/glu.h>
    // glu functions deprecated in 10.9
#   pragma clang diagnostic ignored "-Wdeprecated-declarations"
#   pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#else
    #include <GL/glu.h>
#endif

#define TEXTURE_SIZE 512

#undef DYNAMIC_TEXTURE_SHOW_BORDER // define this to show borders around image tiles

const QString DynamicTexture::pyramidFileExtension = QString( "pyr" );
const QString DynamicTexture::pyramidFolderSuffix = QString( ".pyramid/" );

namespace
{
const QString PYRAMID_METADATA_FILE_NAME( "pyramid.pyr" );
}

DynamicTexture::DynamicTexture(const QString& uri, DynamicTexturePtr parent,
                               const QRectF& parentCoordinates, const int childIndex)
    : uri_(uri)
    , useImagePyramid_(false)
    , threadCount_(0)
    , parent_(parent)
    , imageCoordsInParentImage_(parentCoordinates)
    , depth_(0)
    , loadImageThreadStarted_(false)
    , renderedChildren_(false)
{
    // if we're a child...
    if(parent)
    {
        depth_ = parent->depth_ + 1;

        // append childIndex to parent's path to form this object's path
        treePath_ = parent->treePath_;
        treePath_.push_back(childIndex);

        imageExtension_ = parent->imageExtension_;
    }

    // if we're the top-level object
    if(isRoot())
    {
        // this is the top-level object, so its path is 0
        treePath_.push_back(0);

        const QString extension = QString(".").append(pyramidFileExtension);
        if(uri_.endsWith(extension))
            readPyramidMetadataFromFile(uri_);
        else
            readFullImageMetadata(uri_);
    }
}

DynamicTexture::~DynamicTexture()
{
}

bool DynamicTexture::isRoot() const
{
    return depth_ == 0;
}

bool DynamicTexture::readFullImageMetadata( const QString& uri )
{
    const QImageReader imageReader(uri);
    if( !imageReader.canRead( ))
        return false;

    imageExtension_ = QString( imageReader.format( ));
    imageSize_ = imageReader.size();
    return true;
}

bool DynamicTexture::readPyramidMetadataFromFile( const QString& uri )
{
    std::ifstream ifs(uri.toLatin1());

    // read the whole line
    std::string lineString;
    getline(ifs, lineString);

    // parse the arguments, allowing escaped characters, quotes, etc., and assign them to a vector
    std::string separator1("\\"); // allow escaped characters
    std::string separator2(" "); // split on spaces
    std::string separator3("\"\'"); // allow quoted arguments

    boost::escaped_list_separator<char> els(separator1, separator2, separator3);
    boost::tokenizer<boost::escaped_list_separator<char> > tokenizer(lineString, els);

    std::vector<std::string> tokens;
    tokens.assign(tokenizer.begin(), tokenizer.end());

    if( tokens.size() != 3 )
    {
        put_flog( LOG_ERROR, "requires 3 arguments, got %i", tokens.size( ));
        return false;
    }

    imagePyramidPath_ = QString(tokens[0].c_str());
    if( !determineImageExtension( imagePyramidPath_ ))
        return false;

    imageSize_.setWidth(atoi(tokens[1].c_str()));
    imageSize_.setHeight(atoi(tokens[2].c_str()));

    useImagePyramid_ = true;

    put_flog( LOG_VERBOSE, "read pyramid file: '%s'', width: %i, height: %i",
              imagePyramidPath_.toLocal8Bit().constData(), imageSize_.width(),
              imageSize_.height( ));

    return true;
}

bool DynamicTexture::determineImageExtension( const QString& imagePyramidPath )
{
    const QFileInfoList pyramidRootFiles =
            QDir( imagePyramidPath ).entryInfoList( QStringList( "0.*" ));
    if( pyramidRootFiles.empty( ))
        return false;

    const QString extension = pyramidRootFiles.first().suffix();
    if( !QImageReader().supportedImageFormats().contains( extension.toLatin1( )))
        return false;

    imageExtension_ = extension;
    return true;
}

bool DynamicTexture::writeMetadataFile( const QString& pyramidFolder,
                                        const QString& filename ) const
{
    std::ofstream ofs( filename.toStdString().c_str( ));
    if( !ofs.good( ))
    {
        put_flog( LOG_ERROR, "can't write second metadata file: '%s'",
                  filename.toStdString().c_str( ));
        return false;
    }

    ofs << "\"" << pyramidFolder.toStdString() << "\" " << imageSize_.width()
        << " " << imageSize_.height();
    return true;
}

bool DynamicTexture::writePyramidMetadataFiles(const QString& pyramidFolder) const
{
    // First metadata file inside the pyramid folder
    const QString metadataFilename = pyramidFolder + PYRAMID_METADATA_FILE_NAME;

    // Second (more conveniently named) metadata file outside the pyramid folder
    QString secondMetadataFilename = pyramidFolder;
    const int lastIndex = secondMetadataFilename.lastIndexOf(pyramidFolderSuffix);
    secondMetadataFilename.truncate(lastIndex);
    secondMetadataFilename.append(".").append(pyramidFileExtension);

    return writeMetadataFile(pyramidFolder, metadataFilename) &&
           writeMetadataFile(pyramidFolder, secondMetadataFilename);
}

QString DynamicTexture::getPyramidImageFilename() const
{
    QString filename;

    for( unsigned int i=0; i<treePath_.size(); ++i )
    {
        filename.append( QString::number( treePath_[i] ));

        if( i != treePath_.size() - 1 )
            filename.append( "-" );
    }

    filename.append( "." ).append( imageExtension_ );

    return filename;
}

bool DynamicTexture::writePyramidImagesRecursive( const QString& pyramidFolder )
{
    loadImage(); // load this object's scaledImage_

    const QString filename = pyramidFolder + getPyramidImageFilename();
    put_flog( LOG_DEBUG, "saving: '%s'", filename.toLocal8Bit().constData( ));

    if( !scaledImage_.save( filename ))
        return false;
    scaledImage_ = QImage(); // no longer need scaled image

    // recursively generate and save children images
    if( canHaveChildren( ))
    {
        QRectF imageBounds[4];
        imageBounds[0] = QRectF( 0.0, 0.0, 0.5, 0.5 );
        imageBounds[1] = QRectF( 0.5, 0.0, 0.5, 0.5 );
        imageBounds[2] = QRectF( 0.5, 0.5, 0.5, 0.5 );
        imageBounds[3] = QRectF( 0.0, 0.5, 0.5, 0.5 );

#pragma omp parallel for
        for( unsigned int i=0; i<4; ++i )
        {
            DynamicTexturePtr child( new DynamicTexture( "", shared_from_this(),
                                                         imageBounds[i], i ));
            child->writePyramidImagesRecursive( pyramidFolder );
        }
    }
    return true;
}

void loadImageInThread( DynamicTexturePtr dynamicTexture )
{
    try
    {
        dynamicTexture->loadImage();
        dynamicTexture->decrementGlobalThreadCount();
    }
    catch( const boost::bad_weak_ptr& )
    {
        put_flog( LOG_DEBUG, "Parent image deleted during image loading" );
    }
}

void DynamicTexture::loadImageAsync()
{
    // only start the thread if this DynamicTexture tree has one available
    // each DynamicTexture tree is limited to (maxThreads - 2) threads, where
    // the max is determined by the global QThreadPool instance
    // we increase responsiveness / interactivity by not queuing up image loading
    // const int maxThreads = std::max(QThreadPool::globalInstance()->maxThreadCount() - 2, 1);
    // todo: this doesn't perform well with too many threads; restricting to 1 thread for now
    const int maxThreads = 1;

    if(getGlobalThreadCount() < maxThreads)
    {
        loadImageThreadStarted_ = true;
        incrementGlobalThreadCount();
        loadImageThread_ = QtConcurrent::run(loadImageInThread, shared_from_this());
    }
}

bool DynamicTexture::loadFullResImage()
{
    if( !fullscaleImage_.load( uri_ ))
    {
        put_flog( LOG_ERROR, "error loading: '%s'",
                  uri_.toLocal8Bit().constData( ));
        return false;
    }
    imageSize_ = fullscaleImage_.size();
    return true;
}

void DynamicTexture::loadImage()
{
    if(isRoot())
    {
        if(useImagePyramid_)
        {
            scaledImage_.load(imagePyramidPath_+'/'+getPyramidImageFilename());
        }
        else
        {
            if (!fullscaleImage_.isNull() || loadFullResImage())
                scaledImage_ = fullscaleImage_.scaled(TEXTURE_SIZE, TEXTURE_SIZE, Qt::KeepAspectRatio);
        }
    }
    else
    {
        DynamicTexturePtr root = getRoot();

        if(root->useImagePyramid_)
        {
            scaledImage_.load(root->imagePyramidPath_+'/'+getPyramidImageFilename());
        }
        else
        {
            DynamicTexturePtr parent(parent_);
            const QImage image = parent->getImageFromParent(imageCoordsInParentImage_, this);

            if(!image.isNull())
            {
                imageSize_= image.size();
                scaledImage_ = image.scaled(TEXTURE_SIZE, TEXTURE_SIZE, Qt::KeepAspectRatio);
            }
        }
    }

    if( scaledImage_.isNull( ))
    {
        put_flog( LOG_ERROR, "loading failed in DynamicTexture: '%s'",
                  uri_.toLocal8Bit().constData( ));
    }
}

const QSize& DynamicTexture::getSize() const
{
    if( imageSize_.isEmpty() && loadImageThreadStarted_ )
        loadImageThread_.waitForFinished();

    return imageSize_;
}

void DynamicTexture::render()
{
    assert( isRoot( ));

    render( zoomRect_ );
}

void DynamicTexture::renderPreview()
{
    render( UNIT_RECTF );
}

void DynamicTexture::render( const QRectF& texCoords )
{
    if( !isVisibleInCurrentGLView( ))
        return;

    if( canHaveChildren() && !isResolutionSufficientForCurrentGLView( ))
    {
        renderChildren( texCoords );
        renderedChildren_ = true;
        return;
    }

    // Normal rendering: load the texture if not already available
    if( !loadImageThreadStarted_ )
        loadImageAsync();

    drawTexture( texCoords );
}

void DynamicTexture::preRenderUpdate( ContentWindowPtr window,
                                      const QRect& wallArea )
{
    assert( isRoot( ));

    if( !QRectF( wallArea ).intersects( _qmlItem->getSceneRect( )))
        return;

    // Root needs to always have a texture for renderInParent()
    if( !loadImageThreadStarted_ )
        loadImageAsync();

    zoomRect_ = window->getZoomRect();
}

void DynamicTexture::postRenderSync( WallToWallChannel& )
{
    clearOldChildren();
    renderedChildren_ = false;
}

QImage DynamicTexture::getRootImage() const
{
    return QImage( imagePyramidPath_+ '/' + getPyramidImageFilename( ));
}

bool DynamicTexture::isVisibleInCurrentGLView()
{
    // TODO This objects visibility should be determined by using the GLWindow
    // as a pre-render step, not retro-fitted in here!
    const QRectF screenRect = getProjectedPixelRect(true);
    return screenRect.width()*screenRect.height() > 0.;
}

bool DynamicTexture::isResolutionSufficientForCurrentGLView()
{
    const QRectF fullRect = getProjectedPixelRect(false);
    return fullRect.width() <= TEXTURE_SIZE && fullRect.height() <= TEXTURE_SIZE;
}

bool DynamicTexture::canHaveChildren()
{
    return (getRoot()->imageSize_.width() / (1 << depth_) > TEXTURE_SIZE ||
            getRoot()->imageSize_.height() / (1 << depth_) > TEXTURE_SIZE);
}

void DynamicTexture::drawTexture(const QRectF& texCoords)
{
    if(!texture_.isValid() && loadImageThreadStarted_ && loadImageThread_.isFinished())
        generateTexture();

    if(texture_.isValid())
    {
#ifdef DYNAMIC_TEXTURE_SHOW_BORDER
        renderTextureBorder();
#endif
        renderTexturedUnitQuad(texCoords);
    }
    else
    {
        // If we don't yet have a texture, try to render from parent's texture
        DynamicTexturePtr parent = parent_.lock();
        if(parent)
            parent->drawTexture(getImageRegionInParentImage(texCoords));
    }
}

void DynamicTexture::renderTextureBorder()
{
    glPushAttrib( GL_CURRENT_BIT );

    glColor4f( 0.f, 1.f, 0.f, 1.f );

    quadBorder_.setRenderMode( GL_LINE_LOOP );
    quadBorder_.render();

    glPopAttrib();
}

void DynamicTexture::renderTexturedUnitQuad( const QRectF& texCoords )
{
    quad_.setTexCoords( texCoords );
    quad_.render();
}

void DynamicTexture::clearOldChildren()
{
    if(!renderedChildren_ && !children_.empty() && getThreadsDoneDescending())
        children_.clear();

    // run on my children (if i still have any)
    for(unsigned int i=0; i<children_.size(); i++)
        children_[i]->clearOldChildren();
}

bool DynamicTexture::makeFolder( const QString& folder )
{
    if( !QDir( folder ).exists ())
    {
        if( !QDir().mkdir( folder ))
        {
            put_flog( LOG_ERROR, "error creating directory: '%s'",
                      folder.toLocal8Bit().constData( ));
            return false;
        }
    }
    return true;
}

bool DynamicTexture::generateImagePyramid( const QString& outputFolder )
{
    assert( isRoot( ));

    const QString imageName( QFileInfo( uri_ ).fileName( ));
    const QString pyramidFolder( QDir( outputFolder ).absolutePath() +
                                 "/" + imageName + pyramidFolderSuffix );
    if( loadImageThreadStarted_ )
        loadImageThread_.waitForFinished();

    if( !makeFolder( pyramidFolder ))
        return false;

    if( !writePyramidMetadataFiles( pyramidFolder ))
        return false;

    return writePyramidImagesRecursive( pyramidFolder );
}

void DynamicTexture::decrementGlobalThreadCount()
{
    if(isRoot())
    {
        QMutexLocker locker(&threadCountMutex_);
        threadCount_ = threadCount_ - 1;
    }
    else
    {
        return getRoot()->decrementGlobalThreadCount();
    }
}

DynamicTexturePtr DynamicTexture::getRoot()
{
    if(isRoot())
        return shared_from_this();
    else
        return DynamicTexturePtr(parent_)->getRoot();
}

QRect DynamicTexture::getRootImageCoordinates(float x, float y, float w, float h)
{
    if(isRoot())
    {
        // if necessary, block and wait for image loading to complete
        if( loadImageThreadStarted_ && !loadImageThread_.isFinished( ))
            loadImageThread_.waitForFinished();

        return QRect(x*imageSize_.width(), y*imageSize_.height(),
                     w*imageSize_.width(), h*imageSize_.height());
    }
    else
    {
        DynamicTexturePtr parent = parent_.lock();

        float pX = imageCoordsInParentImage_.x() + x * imageCoordsInParentImage_.width();
        float pY = imageCoordsInParentImage_.y() + y * imageCoordsInParentImage_.height();
        float pW = w * imageCoordsInParentImage_.width();
        float pH = h * imageCoordsInParentImage_.height();

        return parent->getRootImageCoordinates(pX, pY, pW, pH);
    }
}

QRectF DynamicTexture::getImageRegionInParentImage(const QRectF& imageRegion) const
{
    QRectF parentRegion;

    parentRegion.setX(imageCoordsInParentImage_.x() + imageRegion.x() * imageCoordsInParentImage_.width());
    parentRegion.setY(imageCoordsInParentImage_.y() + imageRegion.y() * imageCoordsInParentImage_.height());
    parentRegion.setWidth(imageRegion.width() * imageCoordsInParentImage_.width());
    parentRegion.setHeight(imageRegion.height() * imageCoordsInParentImage_.height());

    return parentRegion;
}

QImage DynamicTexture::getImageFromParent( const QRectF& imageRegion,
                                           DynamicTexture* start )
{
    // if we're in the starting node, we must ascend
    if( start == this )
    {
        if( isRoot( ))
        {
            put_flog( LOG_DEBUG, "root object has no parent! In file: '%s'",
                      uri_.toLocal8Bit().constData( ));
            return QImage();
        }

        DynamicTexturePtr parent = parent_.lock();
        return parent->getImageFromParent(
                    getImageRegionInParentImage( imageRegion ), this );
    }

    // wait for the load image thread to complete if it's in progress
    if(loadImageThreadStarted_ && !loadImageThread_.isFinished())
        loadImageThread_.waitForFinished();

    if(!fullscaleImage_.isNull())
    {
        // we have a valid image, return the clipped image
        return fullscaleImage_.copy(imageRegion.x()*fullscaleImage_.width(),
                                    imageRegion.y()*fullscaleImage_.height(),
                                    imageRegion.width()*fullscaleImage_.width(),
                                    imageRegion.height()*fullscaleImage_.height());
    }
    else
    {
        // we don't have a valid image
        // if we're the root object, return an empty image
        // otherwise, continue up the tree looking for an image
        if(isRoot())
            return QImage();

        DynamicTexturePtr parent = parent_.lock();
        return parent->getImageFromParent(getImageRegionInParentImage(imageRegion), start);
    }
}

void DynamicTexture::generateTexture()
{
    texture_.init( scaledImage_, GL_BGRA );

    quad_.setTexture( texture_.getTextureId( ));
    quad_.enableAlphaBlending( scaledImage_.hasAlphaChannel( ));

    scaledImage_ = QImage(); // no longer need the source image
}

void DynamicTexture::renderChildren(const QRectF& texCoords)
{
    // children rectangles
    const float inf = 1000000.;

    // texture rectangle a child quadrant may contain
    QRectF textureBounds[4];
    textureBounds[0].setCoords(-inf,-inf, 0.5,0.5);
    textureBounds[1].setCoords(0.5,-inf, inf,0.5);
    textureBounds[2].setCoords(0.5,0.5, inf,inf);
    textureBounds[3].setCoords(-inf,0.5, 0.5,inf);

    // image rectange a child quadrant contains
    QRectF imageBounds[4];
    imageBounds[0] = QRectF(0.,0.,0.5,0.5);
    imageBounds[1] = QRectF(0.5,0.,0.5,0.5);
    imageBounds[2] = QRectF(0.5,0.5,0.5,0.5);
    imageBounds[3] = QRectF(0.,0.5,0.5,0.5);

    // see if we need to generate children
    if(children_.empty())
    {
        for(unsigned int i=0; i<4; i++)
        {
            DynamicTexturePtr child(new DynamicTexture("", shared_from_this(), imageBounds[i], i));
            children_.push_back(child);
        }
    }

    // render children
    for(unsigned int i=0; i<children_.size(); i++)
    {
        // portion of texture for this child
        const QRectF childTextureRect = texCoords.intersected(textureBounds[i]);

        // translate and scale to child texture coordinates
        const QRectF childTextureRectTranslated = childTextureRect.translated(-imageBounds[i].x(), -imageBounds[i].y());

        const QRectF childTextureRectTranslatedAndScaled(childTextureRectTranslated.x() / imageBounds[i].width(),
                                                         childTextureRectTranslated.y() / imageBounds[i].height(),
                                                         childTextureRectTranslated.width() / imageBounds[i].width(),
                                                         childTextureRectTranslated.height() / imageBounds[i].height());

        // find rendering position based on portion of textureRect we occupy
        // recall the parent object (this one) is rendered as a (0,0,1,1) rectangle
        const QRectF renderRect((childTextureRect.x()-texCoords.x()) / texCoords.width(),
                                (childTextureRect.y()-texCoords.y()) / texCoords.height(),
                                childTextureRect.width() / texCoords.width(),
                                childTextureRect.height() / texCoords.height());

        glPushMatrix();
        glTranslatef(renderRect.x(), renderRect.y(), 0.);
        glScalef(renderRect.width(), renderRect.height(), 1.);

        children_[i]->render(childTextureRectTranslatedAndScaled);

        glPopMatrix();
    }
}

bool DynamicTexture::getThreadsDoneDescending()
{
    if(!loadImageThread_.isFinished())
        return false;

    for(unsigned int i=0; i<children_.size(); i++)
    {
        if(!children_[i]->getThreadsDoneDescending())
            return false;
    }

    return true;
}

int DynamicTexture::getGlobalThreadCount()
{
    if(isRoot())
    {
        QMutexLocker locker(&threadCountMutex_);
        return threadCount_;
    }
    else
    {
        return getRoot()->getGlobalThreadCount();
    }
}

void DynamicTexture::incrementGlobalThreadCount()
{
    if(isRoot())
    {
        QMutexLocker locker(&threadCountMutex_);
        threadCount_ = threadCount_ + 1;
    }
    else
    {
        return getRoot()->incrementGlobalThreadCount();
    }
}

QRectF DynamicTexture::getProjectedPixelRect( const bool clampToViewportBorders )
{
    // get four corners in object space (recall we're in normalized 0->1 coord)
    const double corners[4][3] =
    {
        {0.,0.,0.},
        {1.,0.,0.},
        {1.,1.,0.},
        {0.,1.,0.}
    };

    // get four corners in screen space
    GLdouble modelview[16];
    glGetDoublev( GL_MODELVIEW_MATRIX, modelview );

    GLdouble projection[16];
    glGetDoublev( GL_PROJECTION_MATRIX, projection );

    GLint viewport[4];
    glGetIntegerv( GL_VIEWPORT, viewport );

    GLdouble xWin[4][3];

    for(size_t i=0; i<4; i++)
    {
        gluProject( corners[i][0], corners[i][1], corners[i][2],
                    modelview, projection, viewport,
                    &xWin[i][0], &xWin[i][1], &xWin[i][2] );

        const GLdouble viewportWidth = (GLdouble)viewport[2];
        const GLdouble viewportHeight = (GLdouble)viewport[3];

        // The GL coordinates system origin is at the bottom-left corner with
        // the y-axis pointing upwards. For the QRect, we want the origin at
        // the top of the viewport with the y-axis pointing downwards.
        xWin[i][1] = viewportHeight - xWin[i][1];

        if( clampToViewportBorders )
        {
            xWin[i][0] = std::min( std::max( xWin[i][0], 0. ), viewportWidth );
            xWin[i][1] = std::min( std::max( xWin[i][1], 0. ), viewportHeight );
        }
    }

    const QPointF topleft( xWin[0][0], xWin[0][1] );
    const QPointF bottomright( xWin[2][0], xWin[2][1] );
    return QRectF( topleft, bottomright );
}
