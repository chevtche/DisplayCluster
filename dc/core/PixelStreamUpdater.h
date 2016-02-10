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

#ifndef PIXELSTREAMUPDATER_H
#define PIXELSTREAMUPDATER_H

#include "types.h"

#include "SwapSyncObject.h"

#include <deflect/SegmentDecoder.h>

#include <QObject>
#include <deque>

/**
 * Synchronize the update of PixelStreams and send new frame requests.
 */
class PixelStreamUpdater : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY( PixelStreamUpdater )

public:
    /** Constructor. */
    PixelStreamUpdater();

    /** Synchronize the update of the PixelStreams. */
    void synchronizeFramesSwap( WallToWallChannel& channel );

    /**
     * Get a segment by its frame- and tile-index.
     */
    QImage getTileImage( uint frameIndex, uint tileIndex );

    /** Get the list of tiles for use by QML repeater. */
    const Tiles& getTiles() const;

    /** Update the tiles visibility given the visible area of the window. */
    void updateVisibility( const QRectF& visibleArea );

public slots:
    /** Update the appropriate PixelStream with the given frame. */
    void updatePixelStream( deflect::FramePtr frame );

signals:
    /** Emitted when a new picture has become available. */
    void pictureUpdated( uint frameIndex );

    /** Emitted to request a new frame after a successful swap. */
    void requestFrame( QString uri );

    /** Emitted when the number of tiles has changed after a successful swap. */
    void tilesChanged();

private:
    typedef SwapSyncObject<deflect::FramePtr> SwapSyncFrame;
    SwapSyncFrame _swapSyncFrame;

    std::deque< std::pair< size_t, deflect::FramePtr > > _frames;

    uint _frameIndex;
    uint _requestedFrameIndex;

    Tiles _tiles;
    QRectF _visibleArea;

    typedef std::vector<size_t> SegmentIndices;
    SegmentIndices _visibleSet;

    void _onFrameSwapped( deflect::FramePtr frame );
    void _decodeSegments( deflect::Segments& segments );
    void _computeVisibleSet( const deflect::Segments& segments );
    void _refreshTiles( const deflect::Segments& segments );
};

#endif
