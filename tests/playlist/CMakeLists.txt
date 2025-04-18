include_directories(
  ${AMAROK_TEST_TREE}
  ${AMAROK_SOURCE_TREE}
  ${AMAROK_UTILITY_TREE}
  ${CMAKE_BINARY_DIR}/src
  ${CMAKE_BINARY_DIR}/tests
  ${AMAROK_COLLECTION_SUPPORT_DIR}
)

include_directories(SYSTEM
  ${GOOGLEMOCK_INCLUDE_DIR}
  )

#------------------------ Test CueFileSupport -----------------------------

set( testplaylistmodels_SRCS
        TestPlaylistModels.cpp
        ${AMAROK_SOURCE_TREE}/EngineController.cpp
        ${AMAROK_SOURCE_TREE}/browsers/CollectionSortFilterProxyModel.cpp
        ${AMAROK_SOURCE_TREE}/browsers/collectionbrowser/CollectionBrowserTreeView.cpp
        ${AMAROK_SOURCE_TREE}/browsers/collectionbrowser/CollectionWidget.cpp
        ${AMAROK_SOURCE_TREE}/browsers/CollectionTreeItemModel.cpp
        ${AMAROK_SOURCE_TREE}/browsers/CollectionTreeItemModelBase.cpp
        ${AMAROK_SOURCE_TREE}/browsers/CollectionTreeView.cpp
        ${AMAROK_SOURCE_TREE}/core/capabilities/BoundedPlaybackCapability.cpp
        ${AMAROK_SOURCE_TREE}/core/capabilities/MultiSourceCapability.cpp
        ${AMAROK_SOURCE_TREE}/core-impl/collections/support/TrashCollectionLocation.cpp
        ${AMAROK_SOURCE_TREE}/core-impl/collections/support/Expression.cpp
        ${AMAROK_SOURCE_TREE}/core-impl/collections/support/TextualQueryFilter.cpp
        ${AMAROK_SOURCE_TREE}/playback/PowerManager.cpp
        ${AMAROK_SOURCE_TREE}/playlist/PlaylistDefines.cpp
        ${AMAROK_SOURCE_TREE}/playlist/PlaylistModel.cpp
        ${AMAROK_SOURCE_TREE}/playlist/PlaylistModelStack.cpp
        ${AMAROK_SOURCE_TREE}/playlist/proxymodels/ProxyBase.cpp
        ${AMAROK_SOURCE_TREE}/playlist/proxymodels/GroupingProxy.cpp
        ${AMAROK_SOURCE_TREE}/playlist/proxymodels/SearchProxy.cpp
        ${AMAROK_SOURCE_TREE}/playlist/proxymodels/SortAlgorithms.cpp
        ${AMAROK_SOURCE_TREE}/playlist/proxymodels/SortFilterProxy.cpp
        ${AMAROK_SOURCE_TREE}/playlist/proxymodels/SortScheme.cpp
        ${AMAROK_SOURCE_TREE}/core-impl/playlists/types/file/PlaylistFileSupport.cpp
        ${AMAROK_SOURCE_TREE}/playlist/PlaylistItem.cpp
        ${AMAROK_SOURCE_TREE}/core/support/Amarok.cpp
        ${AMAROK_SOURCE_TREE}/scripting/scriptengine/AmarokCollectionViewScript.cpp
        ${AMAROK_SOURCE_TREE}/scripting/scriptengine/ScriptingDefines.cpp
        ${AMAROK_SOURCE_TREE}/widgets/PrettyTreeDelegate.cpp

        ${GOOGLEMOCK_SRCS}
)

add_executable( testplaylistmodels ${testplaylistmodels_SRCS} )
add_test(NAME testplaylistmodels COMMAND $<TARGET_FILE:testplaylistmodels>)
ecm_mark_as_test(testplaylistmodels)

add_dependencies( testplaylistmodels amaroklib )

target_link_libraries( testplaylistmodels
    amarokcore
    amaroklib
    KF${KF_MAJOR_VERSION}::KIOCore

    Qt::Test
    Qt::Core
    ${GOOGLEMOCK_LIBRARIES}
    ${CMAKE_DL_LIBS}
)

#------------------------ Test PlaylistLayout -----------------------------

set( testplaylistlayout_SRCS
        TestPlaylistLayout.cpp
        ${AMAROK_SOURCE_TREE}/playlist/layouts/LayoutManager.cpp
        ${AMAROK_SOURCE_TREE}/playlist/layouts/LayoutItemConfig.cpp
        ${AMAROK_SOURCE_TREE}/playlist/proxymodels/ProxyBase.cpp
        ${AMAROK_SOURCE_TREE}/playlist/proxymodels/GroupingProxy.cpp
        ${AMAROK_SOURCE_TREE}/playlist/PlaylistDefines.cpp
        ${AMAROK_SOURCE_TREE}/core-impl/playlists/types/file/PlaylistFileSupport.cpp
        ${AMAROK_SOURCE_TREE}/core/support/Amarok.cpp
        ${GOOGLEMOCK_SRCS}
)

add_executable( testplaylistlayout ${testplaylistlayout_SRCS} )
add_test(NAME testplaylistlayout COMMAND $<TARGET_FILE:testplaylistlayout>)
ecm_mark_as_test(testplaylistlayout)

add_dependencies( testplaylistlayout amaroklib )

target_link_libraries( testplaylistlayout
    amarokcore
    amaroklib
    KF${KF_MAJOR_VERSION}::KIOCore

    Qt::Test
    Qt::Core
    ${GOOGLEMOCK_LIBRARIES}
    ${CMAKE_DL_LIBS}
)
