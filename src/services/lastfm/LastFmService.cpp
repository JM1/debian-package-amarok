/****************************************************************************************
 * Copyright (c) 2007 Shane King <kde@dontletsstart.com>                                *
 * Copyright (c) 2008 Leo Franchi <lfranchi@kde.org>                                    *
 * Copyright (c) 2009 Casey Link <unnamedrambler@gmail.com>                             *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#define DEBUG_PREFIX "LastFmService"
#include "core/support/Debug.h"

#include "LastFmService.h"

#include "AvatarDownloader.h"
#include "EngineController.h"
#include "biases/LastFmBias.h"
#include "biases/WeeklyTopBias.h"
#include "LastFmServiceCollection.h"
#include "LastFmServiceConfig.h"
#include "LoveTrackAction.h"
#include "SimilarArtistsAction.h"
#include "LastFmTreeModel.h"
#include "LastFmTreeView.h"
#include "ScrobblerAdapter.h"
#include "GlobalCurrentTrackActions.h"
#include "core/support/Components.h"
#include "core/logger/Logger.h"
#include "meta/LastFmMeta.h"
#include "SynchronizationAdapter.h"
#include "statsyncing/Controller.h"
#include "widgets/SearchWidget.h"

#include <QCryptographicHash>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPixmap>
#include <QStandardPaths>
#include <QTimer>

#include <XmlQuery.h>


LastFmServiceFactory::LastFmServiceFactory()
    : ServiceFactory()
{}

void
LastFmServiceFactory::init()
{
    if( m_initialized )
        return;

    ServiceBase *service = new LastFmService( this, QStringLiteral("Last.fm") );
    m_initialized = true;
    Q_EMIT newService( service );
}

QString
LastFmServiceFactory::name()
{
    return QStringLiteral("Last.fm");
}

KConfigGroup
LastFmServiceFactory::config()
{
    return Amarok::config( LastFmServiceConfig::configSectionName() );
}

bool
LastFmServiceFactory::possiblyContainsTrack( const QUrl &url ) const
{
    return url.scheme() == QStringLiteral("lastfm");
}


LastFmService::LastFmService( LastFmServiceFactory *parent, const QString &name )
    : ServiceBase( name, parent, false )
    , m_collection( nullptr )
    , m_polished( false )
    , m_avatarLabel( nullptr )
    , m_profile( nullptr )
    , m_userinfo( nullptr )
    , m_subscriber( false )
    , m_config( LastFmServiceConfig::instance() )
{
    DEBUG_BLOCK
    setShortDescription( i18n( "Last.fm: The social music revolution" ) );
    setIcon( QIcon::fromTheme( QStringLiteral("view-services-lastfm-amarok") ) );
    setLongDescription( i18n( "Last.fm is a popular online service that provides personal radio stations and music recommendations. A personal listening station is tailored based on your listening habits and provides you with recommendations for new music. It is also possible to play stations with music that is similar to a particular artist as well as listen to streams from people you have added as friends" ) );
    setImagePath( QStandardPaths::locate( QStandardPaths::GenericDataLocation, QStringLiteral("amarok/images/hover_info_lastfm.png") ) );

    //We have no use for searching currently..
    m_searchWidget->setVisible( false );

    // set the global static Lastfm::Ws stuff
    lastfm::ws::ApiKey = Amarok::lastfmApiKey();
    lastfm::ws::SharedSecret = Amarok::lastfmApiSharedSecret();

    // HTTPS is the only scheme supported by Auth
    lastfm::ws::setScheme(lastfm::ws::Https);

    // set the nam TWICE. Yes. It prevents liblastfm from deleting it, see their code
    lastfm::setNetworkAccessManager( The::networkAccessManager() );
    lastfm::setNetworkAccessManager( The::networkAccessManager() );

    // enable custom bias
    m_biasFactories << new Dynamic::LastFmBiasFactory();
    Dynamic::BiasFactory::instance()->registerNewBiasFactory( m_biasFactories.last() );
    m_biasFactories << new Dynamic::WeeklyTopBiasFactory();
    Dynamic::BiasFactory::instance()->registerNewBiasFactory( m_biasFactories.last() );

    The::globalCollectionActions()->addTrackAction( new LoveTrackAction( this ) );

    QAction *loveAction = new QAction( QIcon::fromTheme( QStringLiteral("love-amarok") ), i18n( "Last.fm: Love" ), this );
    connect( loveAction, &QAction::triggered, this, &LastFmService::loveCurrentTrack );
    loveAction->setShortcut( i18n( "Ctrl+L" ) );
    The::globalCurrentTrackActions()->addAction( loveAction );

    connect( m_config.data(), &LastFmServiceConfig::updated, this, &LastFmService::slotReconfigure );
    QTimer::singleShot(0, this, &LastFmService::slotReconfigure); // call reconfigure but only after constructor is finished (because it might call virtual methods)
}

LastFmService::~LastFmService()
{
    DEBUG_BLOCK
    using namespace Dynamic;
    QMutableListIterator<AbstractBiasFactory *> it( m_biasFactories );
    while( it.hasNext() )
    {
        AbstractBiasFactory *factory = it.next();
        it.remove();

        BiasFactory::instance()->removeBiasFactory( factory );
        delete factory;
    }

    if( m_collection )
    {
        CollectionManager::instance()->removeTrackProvider( m_collection );
        m_collection->deleteLater();
        m_collection = nullptr;
    }

    StatSyncing::Controller *controller = Amarok::Components::statSyncingController();
    if( m_scrobbler && controller )
        controller->unregisterScrobblingService( m_scrobbler.staticCast<StatSyncing::ScrobblingService>() );
    if( m_synchronizationAdapter && controller )
        controller->unregisterProvider( m_synchronizationAdapter );
}

void
LastFmService::slotReconfigure()
{
    lastfm::ws::Username = m_config->username();
    bool ready = !m_config->username().isEmpty(); // core features require just username

    /* create ServiceCollection only once the username is known (remember, getting
     * username from KWallet is async! */
    if( !m_collection && ready )
    {
        m_collection = new Collections::LastFmServiceCollection( m_config->username() );
        CollectionManager::instance()->addTrackProvider( m_collection );
    }

    // create Model once the username is known, it depends on it implicitly
    if( !model() && ready )
    {
        setModel( new LastFmTreeModel( this ) );
    }

    setServiceReady( ready ); // emits ready(), which needs to be done *after* creating collection

    // now check our session key and username status, which should be handled by serviceconfig in background
    if( !m_config->sessionKey().isEmpty() && !m_config->username().isEmpty() )
    {
        debug() << __PRETTY_FUNCTION__ << "using saved session key for last.fm";
        continueReconfiguring();
    }
    else if( !( m_config->sessionKey().isEmpty() && m_config->username().isEmpty() ) )
    {
        debug() << __PRETTY_FUNCTION__ << "got no saved last.fm" << (m_config->sessionKey().isEmpty() ? "session," : "username,") <<
            "serviceconfig will notify user in case interaction is needed";
        continueReconfiguring();
    }
    else
    {
        debug() << __PRETTY_FUNCTION__ << "no last.fm login details found";
        continueReconfiguring();
    }
}

void
LastFmService::continueReconfiguring()
{
    StatSyncing::Controller *controller = Amarok::Components::statSyncingController();
    Q_ASSERT( controller );

    lastfm::ws::SessionKey = m_config->sessionKey();
    bool authenticated = serviceReady() && !m_config->sessionKey().isEmpty() && !m_config->username().isEmpty();

    if( m_scrobbler && (!authenticated || !m_config->scrobble()) )
    {
        debug() << __PRETTY_FUNCTION__ << "unregistering and destroying ScrobblerAdapter";
        controller->unregisterScrobblingService( m_scrobbler.staticCast<StatSyncing::ScrobblingService>() );
        m_scrobbler.clear();
    }
    else if( !m_scrobbler && authenticated && m_config->scrobble() )
    {
        debug() << __PRETTY_FUNCTION__ << "creating and registering ScrobblerAdapter";
        m_scrobbler = QSharedPointer<ScrobblerAdapter>( new ScrobblerAdapter( QStringLiteral("Amarok"), m_config ) );
        controller->registerScrobblingService( m_scrobbler.staticCast<StatSyncing::ScrobblingService>() );
    }

    if( m_synchronizationAdapter && !authenticated )
    {
        debug() << __PRETTY_FUNCTION__ << "unregistering and destroying SynchronizationAdapter";
        controller->unregisterProvider( m_synchronizationAdapter );
        m_synchronizationAdapter = nullptr;
    }
    else if( !m_synchronizationAdapter && authenticated )
    {
        debug() << __PRETTY_FUNCTION__ << "creating and registering SynchronizationAdapter";
        m_synchronizationAdapter = StatSyncing::ProviderPtr( new SynchronizationAdapter( m_config ) );
        controller->registerProvider( m_synchronizationAdapter );
    }

    // update possibly changed user info
    QNetworkReply *reply = lastfm::User::getInfo();
    connect( reply, &QNetworkReply::finished, this, &LastFmService::onGetUserInfo );
}

void
LastFmService::onGetUserInfo()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>( sender() );
    if( !reply )
        warning() << __PRETTY_FUNCTION__ << "null reply!";
    else
        reply->deleteLater();

    switch( reply ? reply->error() : QNetworkReply::UnknownNetworkError )
    {
        case QNetworkReply::NoError:
        {
            lastfm::XmlQuery lfm;
            if( lfm.parse( reply->readAll() ) ) {
                m_country = lfm[QStringLiteral("user")][QStringLiteral("country")].text();
                m_age = lfm[QStringLiteral("user")][QStringLiteral("age")].text();
                m_gender = lfm[QStringLiteral("user")][QStringLiteral("gender")].text();
                m_playcount = lfm[QStringLiteral("user")][QStringLiteral("playcount")].text();
                m_subscriber = lfm[QStringLiteral("user")][QStringLiteral("subscriber")].text() == QStringLiteral("1");

                debug() << "profile info "  << m_country << " " << m_age << " " << m_gender << " " << m_playcount << " " << m_subscriber;
                if( !lfm[QStringLiteral("user")][ QStringLiteral("image") ].text().isEmpty() )
                {
                    debug() << "profile avatar: " <<lfm[QStringLiteral("user")][ QStringLiteral("image") ].text();
                    AvatarDownloader* downloader = new AvatarDownloader();
                    QUrl url( lfm[QStringLiteral("user")][ QStringLiteral("image") ].text() );
                    downloader->downloadAvatar( m_config->username(),  url);
                    connect( downloader, &AvatarDownloader::avatarDownloaded,
                             this, &LastFmService::onAvatarDownloaded );
                }
                updateProfileInfo();
            }
            else
                debug() << "Got exception in parsing from last.fm:" << lfm.parseError().message();
            break;
        }
        case QNetworkReply::AuthenticationRequiredError:
            debug() << "Last.fm: errorMessage: Sorry, we don't recognise that username, or Amarok is not authorized to connect to the Last.fm account.";
            break;
        default:
            debug() << "Last.fm: errorMessage: There was a problem communicating with the Last.fm services. Please try again later.";
            break;
    }
}

void
LastFmService::onAvatarDownloaded( const QString &username, QPixmap avatar )
{
    DEBUG_BLOCK
    sender()->deleteLater();
    if( username == m_config->username() && !avatar.isNull() )
    {
        LastFmTreeModel* lfm = dynamic_cast<LastFmTreeModel*>( model() );
        if( !lfm )
            return;

        int m = lfm->avatarSize();
        avatar = avatar.scaled( m, m, Qt::KeepAspectRatio, Qt::SmoothTransformation );
        lfm->prepareAvatar( avatar, m );
        m_avatar = avatar;

        if( m_avatarLabel )
            m_avatarLabel->setPixmap( m_avatar );
    }
}


void
LastFmService::updateProfileInfo()
{
    if( m_userinfo )
    {
        m_userinfo->setText( i18n( "Username: %1", m_config->username().toHtmlEscaped() ) );
    }

    if( m_profile && !m_playcount.isEmpty() )
    {
        m_profile->setText( i18np( "Play Count: %1 play", "Play Count: %1 plays", m_playcount.toInt() ) );
    }
}

void
LastFmService::polish()
{
    if( !m_polished )
    {
        LastFmTreeView* view = new LastFmTreeView( this );
        view->setFrameShape( QFrame::NoFrame );
        view->setDragEnabled ( true );
        view->setSortingEnabled( false );
        view->setDragDropMode ( QAbstractItemView::DragOnly );
        setView( view );

        //m_bottomPanel->setMaximumHeight( 300 );
        m_bottomPanel->hide();

        m_topPanel->setMaximumHeight( 300 );
        BoxWidget * outerProfilebox = new BoxWidget( false, m_topPanel );
        outerProfilebox->layout()->setSpacing(1);

        m_avatarLabel = new QLabel(outerProfilebox);
        if( !m_avatar )
        {
            int m = LastFmTreeModel::avatarSize();
            m_avatarLabel->setPixmap( QIcon::fromTheme( QStringLiteral("filename-artist-amarok") ).pixmap(m, m) );
            m_avatarLabel->setFixedSize( m, m );
        }
        else
        {
            m_avatarLabel->setPixmap( m_avatar );
            m_avatarLabel->setFixedSize( m_avatar.width(), m_avatar.height() );
            m_avatarLabel->setMargin( 5 );
        }

        BoxWidget * innerProfilebox = new BoxWidget( true, outerProfilebox );
        innerProfilebox->setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Minimum );
        m_userinfo = new QLabel(innerProfilebox);
        m_userinfo->setText( m_config->username() );
        m_profile = new QLabel(innerProfilebox);
        m_profile->setText(QString());
        updateProfileInfo();


        QList<int> levels;
        levels << CategoryId::Genre << CategoryId::Album;
        m_polished = true;
    }
}

void
LastFmService::loveCurrentTrack()
{
    love( The::engineController()->currentTrack() );
}

void
LastFmService::love( Meta::TrackPtr track )
{
    if( m_scrobbler )
        m_scrobbler->loveTrack( track );
}

Collections::Collection * LastFmService::collection()
{
    return m_collection;
}
