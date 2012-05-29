/****************************************************************************************
 * Copyright (c) 2012 Matěj Laitl <matej@laitl.cz>                                      *
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

#ifndef STATSYNCING_MATCHTRACKSJOB_H
#define STATSYNCING_MATCHTRACKSJOB_H

#include "statsyncing/Provider.h"
#include "statsyncing/TrackTuple.h"

#include <ThreadWeaver/Job>

#include <QMap>

namespace StatSyncing
{
    /**
     * Threadweaver job that matches tracks of multiple Providers.
     * Because comparisonFields() needs to be static, only one instance of this class is
     * allowed to exist at given time.
     */
    class MatchTracksJob : public ThreadWeaver::Job
    {
        Q_OBJECT

        public:
            MatchTracksJob( const QList<QSharedPointer<Provider> > &providers,
                            QObject *parent = 0 );

            virtual bool success() const;

            /**
             * Binary OR of MetaValues.h Meta::val* flags that are used to compare tracks
             * from different providers. Guaranteed to contain at least artist, album,
             * title. Valid only after run() has been called.
             */
            static qint64 comparisonFields();

            // results:
            const QList<TrackTuple> &matchedTuples() const { return m_matchedTuples; }
            const PerProviderTrackList &uniqueTracks() const { return m_uniqueTracks; }
            const PerProviderTrackList &excludedTracks() const { return m_excludedTracks; }

        public slots:
            /**
             * Abort the job as soon as possible.
             */
            void abort();

        signals:
            /**
             * Emitted when matcher gets to know total number of steps it will take to
             * match all tracks.
             */
            void totalSteps( int steps );

            /**
             * Emitted when one progress step has been finished.
             */
            void incrementProgress();

            /**
             * Emitted from worker thread when all time-consuming operations are done.
             */
            void endProgressOperation( QObject *owner );

        protected:
            virtual void run();

        private:
            /**
             * Queries each provider from @param artistProviders for tracks from @param
             * artist and separates them into m_uniqueTracks, m_excludedTracks and
             * m_matchedTuples.
             */
            void matchTracksFromArtist( const QString& artist,
                                        const QSet<Provider *> &artistProviders );

            /**
             * Finds the "smallest" track among provider track lists; assumes individual
             * lists are already sorted and non-empty
             */
            TrackPtr findSmallestTrack( const PerProviderTrackList &providerTracks );

            /**
             * Takes tracks from each provider that are equal to @param track.
             * If a list from @param delegateTracks becomes empty, whole entry for that
             * provider is removed from @param delegateTracks.
             */
            PerProviderTrackList takeTracksEqualTo( const TrackPtr &track,
                                                    PerProviderTrackList &providerTracks );

            /**
             * Adds @param tuple to m_matchedTuples and updated m_matchedTrackCounts
             */
            void addMatchedTuple( const TrackTuple &tuple );

            /**
             * Must be static because comparisonFields needs to be static.
             * @see comparisonFields()
             */
            static qint64 s_comparisonFields;

            bool m_abort;
            QList<QSharedPointer<Provider> > m_providers;

            /**
             * Per-provider list of tracks that are unique to that provider
             */
            PerProviderTrackList m_uniqueTracks;

            /**
             * Per-provider list of tracks that have been excluded from synchronization
             * for various reasons, e.g. for being duplicate within that provider
             */
            PerProviderTrackList m_excludedTracks;

            /**
             * Our raison d'etre: tuples of matched tracks
             */
            QList<TrackTuple> m_matchedTuples;

            /**
             * Per-provider count of matched tracks
             */
            QMap<const Provider *, int> m_matchedTrackCounts;
    };

} // namespace StatSyncing

#endif // STATSYNCING_MATCHTRACKSJOB_H
