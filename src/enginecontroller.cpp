/***************************************************************************
                      enginecontroller.cpp  -  Wraps engine and adds some functionality
                         -------------------
begin                : Mar 15 2004
copyright            : (C) 2004 by Frederik Holljen
                       (C) 2004 by Max Howell
email                : fh@ez.no
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "amarok.h"
#include "amarokconfig.h"
#include "enginebase.h"
#include "enginecontroller.h"
#include "pluginmanager.h"
#include "streamprovider.h"

#include <kdebug.h>
#include <kio/global.h>
#include <kio/job.h>
#include <kmessagebox.h>


ExtensionCache EngineController::s_extensionCache;


static
class DummyEngine : public Engine::Base
{
    //Does nothing, just here to prevent crashes on startup
    //and in case no engines are found

    virtual bool init() { return true; }
    virtual bool canDecode( const KURL& ) { return false; }
    virtual uint length() const { return 0; }
    virtual uint position() const { return 0; }
    virtual bool isStream() const { return false; }
    virtual bool load( const KURL&, bool ) { return false; }
    virtual bool play( uint ) { return false; }
    virtual void stop() {}
    virtual void pause() {}
    virtual void setVolumeSW( uint ) {}
    virtual void seek( uint ) {}

    virtual Engine::State state() const { return Engine::Empty; }

public: DummyEngine() : EngineBase() {}

} dummyEngine;


EngineController::EngineController()
    : m_pEngine( &dummyEngine )
    , m_pMainTimer( new QTimer( this ) )
    , m_delayTime( 0 )
    , m_muteVolume( 0 )
    , m_xFadeThisTrack( false )
    , m_stream( 0 )
{
    connect( &m_timer, SIGNAL( timeout() ), SLOT( slotMainTimer() ) );
}


void EngineController::previous()
{
    emit orderPrevious();
}

void EngineController::next()
{
    emit orderNext();
}

void EngineController::playPause()
{
    //this is used by the TrayIcon, PlayPauseAction and DCOP

    if( m_pEngine->state() == Engine::Playing )
    {
        pause();
    }
    else play();
}

void EngineController::play()
{
    if ( m_pEngine->state() == Engine::Paused )
    {
        m_pEngine->pause();
    }
    else
        emit orderCurrent(); // keep currenttrack to avoid signal?
}

void EngineController::pause()
{
    if ( m_pEngine->loaded() )
    {
        m_pEngine->pause();
    }
}

void EngineController::stop()
{
    m_pEngine->stop();
}


void EngineController::play( const MetaBundle &bundle )
{
    const KURL &url = bundle.url();

    if ( url.protocol() == "http" ) {
        // Detect mimetype of remote file
        KIO::MimetypeJob* job = KIO::mimetype( url, false );
        connect( job, SIGNAL( result( KIO::Job* ) ), SLOT( playRemote( KIO::Job* ) ) );
        m_bundle = bundle;
        return; //don't do notify
    }
    else if( !m_pEngine->play( url ) )
    {
        //NOTE it is up to the engine to present a graphical error message

        return; //don't do notify
    }
    else
    {
        //non stream is now playing

        m_xFadeThisTrack = AmarokConfig::crossfade() &&
                           m_pEngine->hasXFade() &&
                           !m_pEngine->isStream() &&
                           m_bundle.length()*1000 - AmarokConfig::crossfadeLength()*2 > 0;
    }

    //don't assign until we are sure it is playing
    m_bundle = bundle;

    newMetaDataNotify( bundle, true /* track change */ );
}


void EngineController::playRemote( KIO::Job* job ) //SLOT
{
    const QString mimetype = static_cast<KIO::MimetypeJob*>( job )->mimetype();
    kdDebug() << "[controller] Detected mimetype: " << mimetype << endl;

    const KURL &url = m_bundle.url();
    const bool isStream = mimetype.isEmpty() || mimetype == "text/html";

    if ( isStream &&
         AmarokConfig::titleStreaming() &&
         m_pEngine->streamingMode() != Engine::NoStreaming )
    {
        delete m_stream;
        m_stream = new amaroK::StreamProvider( url, m_pEngine->streamingMode() );

        if ( !m_stream->initSuccess() ) {
            delete m_stream;
            m_stream = 0;
            emit orderNext();
            return;
        }

        m_pEngine->play( m_stream->proxyUrl(), isStream );

        connect( m_stream,   SIGNAL( metaData( const MetaBundle& ) ),
                 this,         SLOT( slotNewMetaData( const MetaBundle& ) ) );
        connect( m_stream,   SIGNAL( streamData( char*, int ) ),
                 m_pEngine,    SLOT( newStreamData( char*, int ) ) );
        connect( m_stream,   SIGNAL( sigError() ),
                 this,       SIGNAL( orderNext() ) );
    }
    else if( !m_pEngine->play( url, isStream ) )
    {
        return; //don't notify
    }

    newMetaDataNotify( m_bundle, true /* track change */ );
}


EngineBase *EngineController::loadEngine() //static
{
    kdDebug() << "BEGIN " << k_funcinfo << endl;

    EngineController* const self = instance();
    EngineBase *engine = self->m_pEngine;

    //now load new engine
    const QString query    = "[X-KDE-amaroK-plugintype] == 'engine' and [X-KDE-amaroK-name] == '%1'";
    amaroK::Plugin* plugin = PluginManager::createFromQuery( query.arg( AmarokConfig::soundSystem() ) );

    if( !plugin )
    {
        QString query = "[X-KDE-amaroK-plugintype] == 'engine' and [X-KDE-amaroK-name] != '%1'";
        KTrader::OfferList offers = PluginManager::query( query.arg( AmarokConfig::soundSystem() ) );

        while( !plugin && !offers.isEmpty() ) {
            plugin = PluginManager::createFromService( offers.front() );
            offers.pop_front();
        }

        if( !plugin )
        {
            KMessageBox::error( 0, i18n(
                "<p>amaroK could not find any sound-engine plugins. "
                "It is likely that amaroK is installed under the wrong prefix, please fix your installation using:<pre>"
                "$ cd /path/to/amarok/source-code/<br>"
                "$ su -c \"make uninstall\"<br>"
                "$ ./configure --prefix=`kde-config --prefix` && su -c \"make install\"<br>"
                "$ kbuildsycoca<br>"
                "$ amarok</pre>"
                "More information can be found in the README file. For further assistance join us at #amarok on irc.freenode.net." ) );

            ::exit( EXIT_SUCCESS );
        }

        AmarokConfig::setSoundSystem( PluginManager::getService( plugin )->property( "X-KDE-amaroK-name" ).toString() );

        //TODO KMessageBox::error( 0, i18n( "The requested engine could not be loaded, instead %1 was loaded." ) );
        kdDebug() << "Setting soundSystem to: " << AmarokConfig::soundSystem() << endl;
    }

    if( static_cast<EngineBase*>(plugin)->init() )
    {
        //only change things if the init was successful,
        //otherwise leave amaroK with the old engine loaded

        extensionCache().clear();

        //assign new engine, unload old one. Order is thread-safe!
        self->m_pEngine = static_cast<EngineBase*>(plugin);
        if( engine != &dummyEngine ) PluginManager::unload( engine );

        engine = static_cast<EngineBase*>(plugin);

        connect( engine, SIGNAL(stateChanged( Engine::State )), self, SLOT(slotStateChanged( Engine::State )) );
        connect( engine, SIGNAL(trackEnded()), self, SLOT(slotTrackEnded()) );
        connect( engine, SIGNAL(statusText( const QString& )), self, SIGNAL(statusText( const QString& )) );

        //NOTE engine settings are set in App::applySettings()

    } else {

        //init failed - fall back to currently loaded engine
        KMessageBox::error( 0, i18n( "The new engine could not be loaded." ) );
        AmarokConfig::setSoundSystem( PluginManager::getService( engine )->property( "X-KDE-amaroK-name" ).toString() );
    }

    kdDebug() << "END " << k_funcinfo << endl;

    return engine;
}


int EngineController::setVolume( int percent )
{
    percent = uint(percent);
    if( percent > 100 ) percent = 100;

    if( (uint)percent != m_pEngine->volume() )
    {
        m_pEngine->setVolume( (uint)percent );

        percent = m_pEngine->volume();
        AmarokConfig::setMasterVolume( percent );
        volumeChangedNotify( percent );
        return percent;
    }

    return m_pEngine->volume();
}


int EngineController::increaseVolume( int ticks )
{
    return setVolume( m_pEngine->volume() + ticks );
}


int EngineController::decreaseVolume( int ticks )
{
    return setVolume( m_pEngine->volume() - ticks );
}


void EngineController::mute()
{
    if( m_muteVolume == 0 )
    {
        m_muteVolume = m_pEngine->volume();
        setVolume( 0 );
    }
    else
    {
        setVolume( m_muteVolume );
        m_muteVolume = 0;
    }
}


/////////////////////////////
/// SLOTS
/////////////////////////////

void EngineController::slotNewMetaData( const MetaBundle &bundle )
{
    m_bundle = bundle;

    newMetaDataNotify( bundle, false /* not a new track */ );
}

void EngineController::slotMainTimer()
{
    const uint position = m_pEngine->position();

    trackPositionChangedNotify( position );

    // Crossfading
    if ( m_xFadeThisTrack && (m_bundle.length()*1000 - position < (uint)AmarokConfig::crossfadeLength()) )
    {
        kdDebug() << "[controller] Crossfading...\n";
        next();
    }
}

void EngineController::slotTrackEnded()
{
    if ( AmarokConfig::trackDelayLength() > 0 )
    {
        //FIXME not perfect
        QTimer::singleShot( AmarokConfig::trackDelayLength(), this, SLOT(next()) );
    }
    else next();
}

void EngineController::slotStateChanged( Engine::State newState )
{
    switch( newState )
    {
    case Engine::Empty:

        m_bundle = MetaBundle();

        delete m_stream;
        m_stream = 0;

        //FALL THROUGH...

    case Engine::Paused:

        m_timer.stop();
        break;

    case Engine::Playing:

        m_timer.start( MAIN_TIMER );
        break;

    default:
        ;
    }

    stateChangedNotify( newState );
}


#include <kfileitem.h>
bool EngineController::canDecode( const KURL &url ) //static
{
    //TODO engine refactor branch has to be KURL aware for this function
    //TODO a KFileItem version?

    // Accept non-local files, since we can't test them for validity at this point
    if ( !url.isLocalFile() ) return true;

    const QString fileName = url.fileName();
    const QString ext = fileName.mid( fileName.findRev( '.' ) + 1 ).lower();

    if ( extensionCache().contains( ext ) )
        return s_extensionCache[ext];

    const bool valid = engine()->canDecode( url );

    // Cache this result for the next lookup
    if ( !ext.isEmpty() )
        extensionCache().insert( ext, valid );

    return valid;
}


#include "enginecontroller.moc"
