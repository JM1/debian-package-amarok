/***************************************************************************
 *   Copyright (c) 2006, 2007                                              *
 *        Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02111-1307, USA.          *
 ***************************************************************************/
#include "MagnatuneStore.h"

#include "amarok.h"
#include "statusbar.h"

#include "ServiceSqlRegistry.h"


#include "magnatuneinfoparser.h"
#include "playlist.h"
#include "debug.h"
//#include "../../contextview/contextview.h"
//#include "../../contextview/cloudbox.h"
//#include "../../contextview/graphicsitemfader.h"

#include <kstandarddirs.h> //locate()
#include <kurl.h>
#include <kiconloader.h>   //multiTabBar icons

#include <QGraphicsScene>
#include <QSplitter>
#include <q3dragobject.h>
#include <QLabel>
#include <QMenu>

#include <QTextStream>

#include <QStandardItemModel>
#include <QDirModel>

using namespace Meta;

MagnatuneStore::MagnatuneStore( const char *name )
        : ServiceBase( "Magnatune Store" )
        , m_currentAlbum( 0 )
{
    setObjectName(name);
    DEBUG_BLOCK
    //initTopPanel( );


    setShortDescription("A very friendly service!");
    setIcon( KIcon( Amarok::icon( "amarok_magnatune" ) ) );

    //QSplitter *spliter = new QSplitter( Qt::Vertical, this );

    debug() << "Magnatune browser starting..." << endl;

    //m_popupMenu = new QMenu( spliter);
    //m_infoBox = new MagnatuneArtistInfoBox( spliter, "ArtistInfoBox" );


    //initBottomPanel();

    m_currentInfoUrl = "";

    m_purchaseHandler = 0;
  //  m_redownloadHandler = 0;

    m_purchaseInProgress = 0;

//    m_currentlySelectedItem = 0;

    m_polished = false;
    //polish( );  //FIXME not happening when shown for some reason

}

void MagnatuneStore::addTrackToPlaylist( MagnatuneTrack *item )
{
    if ( !item ) return ; // sanity check

    debug() << "Magnatune browser: adding single track" << endl;
    QString url = item->url();
    Playlist * playlist = Playlist::instance();
    playlist->insertMedia( KUrl( url ) );
}



void MagnatuneStore::purchaseButtonClicked( )
{
    if ( m_purchaseInProgress )
        return;

    m_purchaseInProgress = true;
    m_purchaseAlbumButton->setEnabled( false );

    if ( !m_purchaseHandler )
    {
        m_purchaseHandler = new MagnatunePurchaseHandler();
        m_purchaseHandler->setParent( this );
        connect( m_purchaseHandler, SIGNAL( purchaseCompleted( bool ) ), this, SLOT( purchaseCompleted( bool ) ) );
    }

    if (m_currentAlbum != 0)
        m_purchaseHandler->purchaseAlbum( m_currentAlbum );
}



void MagnatuneStore::initTopPanel( )
{

    KHBox *hBox = new KHBox( m_topPanel);
    //m_topPanel->setMaximumHeight( 24 );
    //QLabel *label = new QLabel ( i18n( "Genre: " ), hBox );
    //label->setObjectName( "genreLabel" );

   // m_genreComboBox = new QComboBox;
   // m_genreComboBox->setParent( hBox );
   // m_genreComboBox->setObjectName( "genreComboBox" );

//    updateGenreBox();

    m_advancedFeaturesButton = new QPushButton;
    m_advancedFeaturesButton->setParent( hBox );
    m_advancedFeaturesButton->setText( i18n( "Advanced" ) );
    m_advancedFeaturesButton->setObjectName( "advancedButton" );
    //connect( m_advancedFeaturesButton, SIGNAL( clicked() ), this, SLOT( processRedownload() ) );

    //connect( m_genreComboBox, SIGNAL( currentIndexChanged ( const QString ) ), this, SLOT( genreChanged( QString ) ) );
}

void MagnatuneStore::initBottomPanel()
{
    m_bottomPanel->setMaximumHeight( 54 );


    KHBox *hBox = new KHBox( m_bottomPanel);
    hBox->setMaximumHeight( 24 );
    hBox->setSpacing( 2 );
    //hBox->setMargin( 2 );

    m_purchaseAlbumButton = new QPushButton;
    m_purchaseAlbumButton->setParent( m_bottomPanel );
    m_purchaseAlbumButton->setText( i18n( "Purchase Album" ) );
    m_purchaseAlbumButton->setObjectName( "purchaseButton" );
    m_purchaseAlbumButton->setIcon( KIcon( Amarok::icon( "download" ) ) );
    m_purchaseAlbumButton->setEnabled( false );
    m_purchaseAlbumButton->setMaximumHeight( 24 );

    m_updateListButton = new QPushButton;
    m_updateListButton->setParent( hBox );
    m_updateListButton->setText( i18n( "Update" ) );
    m_updateListButton->setObjectName( "updateButton" );
    m_updateListButton->setIcon( KIcon( Amarok::icon( "rescan" ) ) );
    m_showInfoToggleButton = new QPushButton;
    m_showInfoToggleButton->setText( i18n( "Show Info" ) );
    m_showInfoToggleButton->setParent( hBox );
    m_showInfoToggleButton->setObjectName( "showInfoCheckbox" );
    m_showInfoToggleButton->setCheckable( true );
    m_showInfoToggleButton->setIcon( KIcon( Amarok::icon( "info" ) ) );
    m_showInfoToggleButton->setChecked( true );

    m_isInfoShown = true;

    connect( m_showInfoToggleButton, SIGNAL( toggled( bool ) ), this, SLOT( showInfo( bool ) ) );
    connect( m_updateListButton, SIGNAL( clicked() ), this, SLOT( updateButtonClicked() ) );
    connect( m_purchaseAlbumButton, SIGNAL( clicked() ) , this, SLOT( purchaseButtonClicked() ) );
}

void MagnatuneStore::updateButtonClicked()
{
    m_updateListButton->setEnabled( false );
    updateMagnatuneList();


    //HACK for testing
    /*debug() << "MagnatuneStore: create xml parser" << endl;
    MagnatuneXmlParser * parser = new MagnatuneXmlParser( "/tmp/album_info.xml" );
    parser->setDbHandler( new MagnatuneDatabaseHandler() );
    connect( parser, SIGNAL( doneParsing() ), SLOT( doneParsing() ) );

    ThreadManager::instance() ->queueJob( parser );*/
}
bool MagnatuneStore::updateMagnatuneList()
{
    //download new list from magnatune

     debug() << "MagnatuneStore: start downloading xml file" << endl;

    m_listDownloadJob = KIO::file_copy( KUrl( "http://magnatune.com/info/album_info.xml" ), KUrl("/tmp/album_info.xml"), 0774 , true, false, true );
    Amarok::StatusBar::instance() ->newProgressOperation( m_listDownloadJob )
    .setDescription( i18n( "Downloading Magnatune.com Database" ) )
    .setAbortSlot( this, SLOT( listDownloadCancelled() ) );

    connect( m_listDownloadJob, SIGNAL( result( KJob * ) ),
            this, SLOT( listDownloadComplete( KJob * ) ) );


    return true;
}


void MagnatuneStore::listDownloadComplete( KJob * downLoadJob )
{

   debug() << "MagnatuneStore: xml file download complete" << endl;

    if ( downLoadJob != m_listDownloadJob )
        return ; //not the right job, so let's ignore it

    m_updateListButton->setEnabled( true );
    if ( !downLoadJob->error() == 0 )
    {
        //TODO: error handling here
        return ;
    }


    debug() << "MagnatuneStore: create xml parser" << endl;
    MagnatuneXmlParser * parser = new MagnatuneXmlParser( "/tmp/album_info.xml" );
    parser->setDbHandler( new MagnatuneDatabaseHandler() );
    connect( parser, SIGNAL( doneParsing() ), SLOT( doneParsing() ) );

    ThreadManager::instance() ->queueJob( parser );
}

void MagnatuneStore::listDownloadCancelled( )
{


    Amarok::StatusBar::instance() ->endProgressOperation( m_listDownloadJob );
    m_listDownloadJob->kill();
    delete m_listDownloadJob;
    m_listDownloadJob = 0;
    debug() << "Aborted xml download" << endl;

    m_updateListButton->setEnabled( true );
}



void MagnatuneStore::doneParsing()
{

    debug() << "MagnatuneStore: done parsing" << endl;
    m_collection->emitUpdated();
}

/*void MagnatuneStore::processRedownload( )
{
    debug() << "Process redownload" << endl;

    if ( m_redownloadHandler == 0 )
    {
        m_redownloadHandler = new MagnatuneRedownloadHandler( this );
    }
    m_redownloadHandler->showRedownloadDialog();
}*/

void MagnatuneStore::purchaseCompleted( bool )
{

    if ( m_purchaseHandler != 0 )
    {
        delete m_purchaseHandler;
        m_purchaseHandler = 0;
    }

    m_purchaseAlbumButton->setEnabled( true );
    m_purchaseInProgress = false;

    debug() << "Purchase operation complete" << endl;

    //TODO: display some kind of success dialog here?


}


void MagnatuneStore::itemSelected( CollectionTreeItem * selectedItem ){

    DEBUG_BLOCK

    //we only enable the purchase button if there is only one item selected and it happens to
    //be an album or a track
    DataPtr dataPtr = selectedItem->data();

    if ( typeid( * dataPtr.data() ) == typeid( MagnatuneTrack ) )  {

        debug() << "is right type (track)" << endl;
        MagnatuneTrack * track = static_cast<MagnatuneTrack *> ( dataPtr.data() );
        m_currentAlbum = static_cast<MagnatuneAlbum *> ( track->album().data() );
        m_purchaseAlbumButton->setEnabled( true );

    } else if ( typeid( * dataPtr.data() ) == typeid( MagnatuneAlbum ) ) {

        m_currentAlbum = static_cast<MagnatuneAlbum *> ( dataPtr.data() );
        debug() << "is right type (album) named " << m_currentAlbum->name() << endl;

        m_purchaseAlbumButton->setEnabled( true );
    } else {

        debug() << "is wrong type" << endl;
        m_purchaseAlbumButton->setEnabled( false );

    }



    return;
}


/*void MagnatuneStore::addMoodyTracksToPlaylist(QString mood)
{
   debug() << "addMoody: " << mood << endl;
   SimpleServiceTrackList tracks = m_dbHandler->getTracksByMood( mood );

   int numberOfTracks = tracks.size();

   if ( numberOfTracks < 11 ) {

       foreach (SimpleServiceTrack * track, tracks) {
           addTrackToPlaylist( dynamic_cast<MagnatuneTrack *> ( track ) );
}
} else {

       int randomIndex;
       for ( int i = 0; i < 10; i++ ) {
           randomIndex = rand() % (numberOfTracks - i);
           addTrackToPlaylist( dynamic_cast<MagnatuneTrack *> ( tracks.takeAt( randomIndex ) ) );

}
}


    qDeleteAll( tracks );

}*/



//using namespace Context;

void MagnatuneStore::polish( )
{

    DEBUG_BLOCK;


    if (!m_polished) {
        m_polished = true;




       // m_dbHandler = new MagnatuneDatabaseHandler();


        initTopPanel( );
        initBottomPanel();

        QList<int> levels;
        //levels << CategoryId::Artist << CategoryId::Album << CategoryId::None;
        levels << CategoryId::Genre << CategoryId::Artist << CategoryId::Album;


        ServiceMetaFactory * metaFactory = new MagnatuneMetaFactory( "magnatune" );
        ServiceSqlRegistry * registry = new ServiceSqlRegistry( metaFactory );
        m_collection = new ServiceSqlCollection( "magnatune", "Magnatune.com", metaFactory, registry );

        m_infoParser = new MagnatuneInfoParser();

        connect ( m_infoParser, SIGNAL( info( QString) ), this, SLOT( infoChanged( QString ) ) );


        setModel( new SingleCollectionTreeItemModel( m_collection, levels ) );
        //model->setInfoParser( infoParser );

        //setModel(new MagnatuneContentModel ( this ) );
        //setModel( model );
        //connect ( m_model, SIGNAL( infoChanged ( QString ) ), this, SLOT( infoChanged ( QString ) ) );

        connect( m_contentView, SIGNAL( itemSelected( CollectionTreeItem * ) ), this, SLOT( itemSelected( CollectionTreeItem * ) ) );

       /* m_contentView->setWindowTitle(QObject::tr("Simple Tree Model"));
        m_contentView->setSortingEnabled ( true );
        m_contentView->sortByColumn ( 0, Qt::AscendingOrder );
    */

        m_infoBox->begin( KUrl( KStandardDirs::locate( "data", "amarok/data/" ) ) );
        m_infoBox->write( "<table align='center' border='0'><tbody align='center' valign='top'>"
          "<tr align='center'><td><div align='center'>"
          "<IMG src='magnatune_logo.png' width='200' height='36' align='center' border='0'>"
          "</div></td></tr><tr><td><BR>"
        + i18n( "Welcome to Amarok's integrated Magnatune.com store. If this is the "
                "first time you run it, you must update the database by pressing the "
                "'Update' button below." )
        + "</td></tr></tbody></table>" );
        m_infoBox->end();



    }




}


/*bool MagnatuneStore::updateContextView()
{

    MagnatuneMoodMap moodMap = m_dbHandler->getMoodMap( 20 );

    int minMoodCount = 10000;
    int maxMoodCount = 0;

    //determine max and min counts
    QMapIterator<QString, int> i(moodMap);
    while (i.hasNext()) {
        i.next();
        if ( i.value() > maxMoodCount ) maxMoodCount = i.value();
        if ( i.value() < minMoodCount ) minMoodCount = i.value();
    }


    //normalize and insert into cloud view


    CloudBox * cloudBox = new CloudBox( 0, 0 );
    cloudBox->setPos( 0, 0 );
    cloudBox->setTitle( "Magnatune Moods" );

    int steps = 10;
    int stepBoundry = maxMoodCount / steps;


    i.toFront();
    while (i.hasNext()) {
        i.next();
        if ( i.value() < stepBoundry ) cloudBox->addText( i.key(), 8, this, SLOT( addMoodyTracksToPlaylist( QString ) ) );
        else  if ( i.value() < stepBoundry * 2 ) cloudBox->addText( i.key(), 12, this, SLOT( addMoodyTracksToPlaylist( QString ) ) );
        else  if ( i.value() < stepBoundry * 4 ) cloudBox->addText( i.key(), 16, this, SLOT( addMoodyTracksToPlaylist( QString ) ) );
        else  if ( i.value() < stepBoundry * 7 ) cloudBox->addText( i.key(), 20, this, SLOT( addMoodyTracksToPlaylist( QString ) ) );
        else cloudBox->addText( i.key(), 24, this, SLOT( addMoodyTracksToPlaylist( QString ) ) );
    }

    cloudBox->done();

    ContextView::instance()->clear();
    ContextView::instance()->addContextBox( cloudBox, -1 , true );

    //connect( cloudBox, SIGNAL( itemSelected( QString ) ), this, SLOT( addMoodyTracksToPlaylist( QString ) ) );

    return true;
}*/






#include "MagnatuneStore.moc"
