/*
 * Copyright 2018  Malte Veerman <malte.veerman@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "PixmapItem.h"

#include <QQuickWindow>
#include <QSGSimpleTextureNode>


PixmapItem::PixmapItem()
    : m_pixmapChanged( false )
    , m_sizeChanged( true )
{
    setFlag( ItemHasContents );
}

void PixmapItem::setSource( const QPixmap& source )
{
    if( m_source.toImage() == source.toImage() )
        return;

    m_source = source;
    m_pixmapChanged = true;
    Q_EMIT sourceChanged();

    setImplicitSize( source.width(), source.height() );

    update();
}

QSGNode* PixmapItem::updatePaintNode( QSGNode *oldNode, UpdatePaintNodeData *updatePaintNodeData )
{
    Q_UNUSED( updatePaintNodeData )

    if( m_source.isNull() || width() == 0 || height() == 0 )
    {
        delete oldNode;

        return nullptr;
    }

    QSGSimpleTextureNode *textureNode = dynamic_cast<QSGSimpleTextureNode*>( oldNode );

    if( !textureNode || m_pixmapChanged )
    {
        delete oldNode;
        textureNode = new QSGSimpleTextureNode;
        textureNode->setFiltering( QSGTexture::Linear );
        m_texture = QSharedPointer<QSGTexture>( window()->createTextureFromImage( m_source.toImage(), QQuickWindow::TextureCanUseAtlas ) );
        textureNode->setTexture( m_texture.data() );
        m_sizeChanged = true;
        m_pixmapChanged = false;
    }

    if( m_sizeChanged )
    {
        textureNode->setRect( boundingRect() );
        m_sizeChanged = false;
    }

    return textureNode;
}

void PixmapItem::geometryChange( const QRectF &newGeometry, const QRectF &oldGeometry )
{
    if( newGeometry.size() != oldGeometry.size() )
        m_sizeChanged = true;

    update();
    QQuickItem::geometryChange( newGeometry, oldGeometry );
}
