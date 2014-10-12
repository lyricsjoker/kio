/* This file is part of the KDE project
   Copyright (C) 2002-2006 Michael Brade <brade@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef kdirlister_p_h
#define kdirlister_p_h

#include "kfileitem.h"

#include <QtCore/QMap>
#include <QtCore/QHash>
#include <QtCore/QCache>
#include <QtCore/QSet>
#include <QtCore/QTimer>
#include <QCoreApplication>
#include <QFileInfo>
#include <QUrl>

#include <kio/global.h>
#include <kdirwatch.h>

/**
 * KCoreDirListerCache stores pointers to KFileItems internally and expects that
 * these pointers remain valid, even if the number of items in the list 'lstItems'
 * in KCoreDirLister::DirItem changes. Since such changes in a QList<KFileItem>
 * may change the internal memory layout of the QList, pointers to KFileItems in a
 * QList<KFileItem> might become invalid.
 *
 * Therefore, we make 'lstItems' a QList<NonMovableFileItem>, where
 * NonMovableFileItem is a class that inherits KFileItem, but is not declared as a
 * Q_MOVABLE_TYPE. This forces QList to never move these items in memory, which is
 * achieved by allocating space for each individual item, and store pointers to
 * these items in the contiguous region in memory.
 *
 * TODO:   Try to get rid of the raw KFileItem pointers in KCoreDirListerCache, and
 *         replace all occurrences of NonMovableFileItem(List) with KFileItem(List).
 */
class NonMovableFileItem : public KFileItem
{
public:
    NonMovableFileItem(const KFileItem &item) :
        KFileItem(item)
    {}
};

class NonMovableFileItemList : public QList<NonMovableFileItem>
{
public:
    NonMovableFileItemList()
    {}

    KFileItem findByName(const QString &fileName) const
    {
        const_iterator it = begin();
        const const_iterator itend = end();
        for (; it != itend; ++it) {
            if ((*it).name() == fileName) {
                return *it;
            }
        }
        return KFileItem();
    }

    KFileItemList toKFileItemList() const
    {
        KFileItemList result;
        result.reserve(count());

        foreach (const NonMovableFileItem &item, *this) {
            result.append(item);
        }

        return result;
    }
};

class KCoreDirLister;
namespace KIO
{
class Job;
class ListJob;
}
class OrgKdeKDirNotifyInterface;
struct KCoreDirListerCacheDirectoryData;

class KCoreDirLister::Private
{
public:
    Private(KCoreDirLister *parent)
        : m_parent(parent)
    {
        complete = false;

        autoUpdate = false;

        delayedMimeTypes = false;

        rootFileItem = KFileItem();

        lstNewItems = 0;
        lstRefreshItems = 0;
        lstMimeFilteredItems = 0;
        lstRemoveItems = 0;

        hasPendingChanges = false;
    }

    void _k_emitCachedItems(const QUrl &, bool, bool);
    void _k_slotInfoMessage(KJob *, const QString &);
    void _k_slotPercent(KJob *, unsigned long);
    void _k_slotTotalSize(KJob *, qulonglong);
    void _k_slotProcessedSize(KJob *, qulonglong);
    void _k_slotSpeed(KJob *, unsigned long);

    bool doMimeExcludeFilter(const QString &mimeExclude, const QStringList &filters) const;
    void connectJob(KIO::ListJob *);
    void jobDone(KIO::ListJob *);
    uint numJobs();
    void addNewItem(const QUrl &directoryUrl, const KFileItem &item);
    void addNewItems(const QUrl &directoryUrl, const NonMovableFileItemList &items);
    void addRefreshItem(const QUrl &directoryUrl, const KFileItem &oldItem, const KFileItem &item);
    void emitItems();
    void emitItemsDeleted(const KFileItemList &items);

    /**
     * Redirect this dirlister from oldUrl to newUrl.
     * @param keepItems if true, keep the fileitems (e.g. when renaming an existing dir);
     * if false, clear out everything (e.g. when redirecting during listing).
     */
    void redirect(const QUrl &oldUrl, const QUrl &newUrl, bool keepItems);

    /**
     * Should this item be visible according to the current filter settings?
     */
    bool isItemVisible(const KFileItem &item) const;

    void prepareForSettingsChange()
    {
        if (!hasPendingChanges) {
            hasPendingChanges = true;
            oldSettings = settings;
        }
    }

    void emitChanges();

    class CachedItemsJob;
    CachedItemsJob *cachedItemsJobForUrl(const QUrl &url) const;

    KCoreDirLister *m_parent;

    /**
     * List of dirs handled by this dirlister. The first entry is the base URL.
     * For a tree view, it contains all the dirs shown.
     */
    QList<QUrl> lstDirs;

    // toplevel URL
    QUrl url;

    bool complete: 1;

    bool autoUpdate: 1;

    bool delayedMimeTypes: 1;

    bool hasPendingChanges: 1; // i.e. settings != oldSettings

    struct JobData {
        long unsigned int percent, speed;
        KIO::filesize_t processedSize, totalSize;
    };

    QMap<KIO::ListJob *, JobData> jobData;

    // file item for the root itself (".")
    KFileItem rootFileItem;

    typedef QHash<QUrl, KFileItemList> NewItemsHash;
    NewItemsHash *lstNewItems;
    QList<QPair<KFileItem, KFileItem> > *lstRefreshItems;
    KFileItemList *lstMimeFilteredItems, *lstRemoveItems;

    QList<CachedItemsJob *> m_cachedItemsJobs;

    QString nameFilter; // parsed into lstFilters

    struct FilterSettings {
        FilterSettings() : isShowingDotFiles(false), dirOnlyMode(false) {}
        bool isShowingDotFiles;
        bool dirOnlyMode;
        QList<QRegExp> lstFilters;
        QStringList mimeFilter;
        QStringList mimeExcludeFilter;
    };
    FilterSettings settings;
    FilterSettings oldSettings;

    friend class KCoreDirListerCache;
};

/**
 * Design of the cache:
 * There is a single KCoreDirListerCache for the whole process.
 * It holds all the items used by the dir listers (itemsInUse)
 * as well as a cache of the recently used items (itemsCached).
 * Those items are grouped by directory (a DirItem represents a whole directory).
 *
 * KCoreDirListerCache also runs all the jobs for listing directories, whether they are for
 * normal listing or for updates.
 * For faster lookups, it also stores a hash table, which gives for a directory URL:
 * - the dirlisters holding that URL (listersCurrentlyHolding)
 * - the dirlisters currently listing that URL (listersCurrentlyListing)
 */
class KCoreDirListerCache : public QObject
{
    Q_OBJECT
public:
    KCoreDirListerCache(); // only called by K_GLOBAL_STATIC
    ~KCoreDirListerCache();

    void updateDirectory(const QUrl &dir);

    KFileItem itemForUrl(const QUrl &url) const;
    NonMovableFileItemList *itemsForDir(const QUrl &dir) const;

    bool listDir(KCoreDirLister *lister, const QUrl &_url, bool _keep, bool _reload);

    // stop all running jobs for lister
    void stop(KCoreDirLister *lister, bool silent = false);
    // stop just the job listing url for lister
    void stopListingUrl(KCoreDirLister *lister, const QUrl &_url, bool silent = false);

    void setAutoUpdate(KCoreDirLister *lister, bool enable);

    void forgetDirs(KCoreDirLister *lister);
    void forgetDirs(KCoreDirLister *lister, const QUrl &_url, bool notify);

    KFileItem findByName(const KCoreDirLister *lister, const QString &_name) const;
    // findByUrl returns a pointer so that it's possible to modify the item.
    // See itemForUrl for the version that returns a readonly kfileitem.
    // @param lister can be 0. If set, it is checked that the url is held by the lister
    KFileItem *findByUrl(const KCoreDirLister *lister, const QUrl &url) const;

    // Called by CachedItemsJob:
    // Emits the cached items, for this lister and this url
    void emitItemsFromCache(KCoreDirLister::Private::CachedItemsJob *job, KCoreDirLister *lister,
                            const QUrl &_url, bool _reload, bool _emitCompleted);
    // Called by CachedItemsJob:
    void forgetCachedItemsJob(KCoreDirLister::Private::CachedItemsJob *job, KCoreDirLister *lister,
                              const QUrl &url);

public Q_SLOTS:
    /**
     * Notify that files have been added in @p directory
     * The receiver will list that directory again to find
     * the new items (since it needs more than just the names anyway).
     * Connected to the DBus signal from the KDirNotify interface.
     */
    void slotFilesAdded(const QString &urlDirectory);

    /**
     * Notify that files have been deleted.
     * This call passes the exact urls of the deleted files
     * so that any view showing them can simply remove them
     * or be closed (if its current dir was deleted)
     * Connected to the DBus signal from the KDirNotify interface.
     */
    void slotFilesRemoved(const QStringList &fileList);

    /**
     * Notify that files have been changed.
     * At the moment, this is only used for new icon, but it could be
     * used for size etc. as well.
     * Connected to the DBus signal from the KDirNotify interface.
     */
    void slotFilesChanged(const QStringList &fileList);
    void slotFileRenamed(const QString &srcUrl, const QString &dstUrl);

private Q_SLOTS:
    void slotFileDirty(const QString &_file);
    void slotFileCreated(const QString &_file);
    void slotFileDeleted(const QString &_file);

    void slotEntries(KIO::Job *job, const KIO::UDSEntryList &entries);
    void slotResult(KJob *j);
    void slotRedirection(KIO::Job *job, const QUrl &url);

    void slotUpdateEntries(KIO::Job *job, const KIO::UDSEntryList &entries);
    void slotUpdateResult(KJob *job);
    void processPendingUpdates();

private:
    void itemsAddedInDirectory(const QUrl &url);

    class DirItem;
    DirItem *dirItemForUrl(const QUrl &dir) const;

    bool validUrl(KCoreDirLister *lister, const QUrl &_url) const;

    void stopListJob(const QString &url, bool silent);

    KIO::ListJob *jobForUrl(const QString &url, KIO::ListJob *not_job = 0);
    const QUrl &joburl(KIO::ListJob *job);

    void killJob(KIO::ListJob *job);

    // Called when something tells us that the directory @p url has changed.
    // Returns true if @p url is held by some lister (meaning: do the update now)
    // otherwise mark the cached item as not-up-to-date for later and return false
    bool checkUpdate(const QUrl &url);

    // Helper method for slotFileDirty
    void handleFileDirty(const QUrl &url);
    void handleDirDirty(const QUrl &url);

    // when there were items deleted from the filesystem all the listers holding
    // the parent directory need to be notified, the items have to be deleted
    // and removed from the cache including all the children.
    void deleteUnmarkedItems(const QList<KCoreDirLister *>&, NonMovableFileItemList &lstItems, const QHash<QString, KFileItem *> &itemsToDelete);

    // Helper method called when we know that a list of items was deleted
    void itemsDeleted(const QList<KCoreDirLister *> &listers, const KFileItemList &deletedItems);
    void slotFilesRemoved(const QList<QUrl> &urls);
    // common for slotRedirection and slotFileRenamed
    void renameDir(const QUrl &oldUrl, const QUrl &url);
    // common for deleteUnmarkedItems and slotFilesRemoved
    void deleteDir(const QUrl &dirUrl);
    // remove directory from cache (itemsCached), including all child dirs
    void removeDirFromCache(const QUrl &dir);
    // helper for renameDir
    void emitRedirections(const QUrl &oldUrl, const QUrl &url);

    /**
     * Emits refreshItem() in the directories that cared for oldItem.
     * The caller has to remember to call emitItems in the set of dirlisters returned
     * (but this allows to buffer change notifications)
     */
    QSet<KCoreDirLister *> emitRefreshItem(const KFileItem &oldItem, const KFileItem &fileitem);

    /**
     * When KDirWatch tells us that something changed in "dir", we need to
     * also notify the dirlisters that are listing a symlink to "dir" (#213799)
     */
    QList<QUrl> directoriesForCanonicalPath(const QUrl &dir) const;

#ifndef NDEBUG
    void printDebug();
#endif

    class DirItem
    {
    public:
        DirItem(const QUrl &dir, const QString &canonicalPath)
            : url(dir), m_canonicalPath(canonicalPath)
        {
            autoUpdates = 0;
            complete = false;
            watchedWhileInCache = false;
        }

        ~DirItem()
        {
            if (autoUpdates) {
                if (KDirWatch::exists() && url.isLocalFile()) {
                    KDirWatch::self()->removeDir(m_canonicalPath);
                }
                // Since sendSignal goes through D-Bus, QCoreApplication has to be available
                // which might not be the case anymore from a global static dtor like the
                // lister cache
                if (QCoreApplication::instance()) {
                    sendSignal(false, url);
                }
            }
            lstItems.clear();
        }

        void sendSignal(bool entering, const QUrl &url)
        {
            // Note that "entering" means "start watching", and "leaving" means "stop watching"
            // (i.e. it's not when the user leaves the directory, it's when the directory is removed from the cache)
            if (entering) {
                org::kde::KDirNotify::emitEnteredDirectory(url);
            } else {
                org::kde::KDirNotify::emitLeftDirectory(url);
            }
        }

        void redirect(const QUrl &newUrl)
        {
            if (autoUpdates) {
                if (url.isLocalFile()) {
                    KDirWatch::self()->removeDir(m_canonicalPath);
                }
                sendSignal(false, url);

                if (newUrl.isLocalFile()) {
                    m_canonicalPath = QFileInfo(newUrl.toLocalFile()).canonicalFilePath();
                    KDirWatch::self()->addDir(m_canonicalPath);
                }
                sendSignal(true, newUrl);
            }

            url = newUrl;

            if (!rootItem.isNull()) {
                rootItem.setUrl(newUrl);
            }
        }

        void incAutoUpdate()
        {
            if (autoUpdates++ == 0) {
                if (url.isLocalFile()) {
                    KDirWatch::self()->addDir(m_canonicalPath);
                }
                sendSignal(true, url);
            }
        }

        void decAutoUpdate()
        {
            if (--autoUpdates == 0) {
                if (url.isLocalFile()) {
                    KDirWatch::self()->removeDir(m_canonicalPath);
                }
                sendSignal(false, url);
            }

            else if (autoUpdates < 0) {
                autoUpdates = 0;
            }
        }

        // number of KCoreDirListers using autoUpdate for this dir
        short autoUpdates;

        // this directory is up-to-date
        bool complete;

        // the directory is watched while being in the cache (useful for proper incAutoUpdate/decAutoUpdate count)
        bool watchedWhileInCache;

        // the complete url of this directory
        QUrl url;

        // the local path, with symlinks resolved, so that KDirWatch works
        QString m_canonicalPath;

        // KFileItem representing the root of this directory.
        // Remember that this is optional. FTP sites don't return '.' in
        // the list, so they give no root item
        KFileItem rootItem;
        NonMovableFileItemList lstItems;
    };

    //static const unsigned short MAX_JOBS_PER_LISTER;

    QMap<KIO::ListJob *, KIO::UDSEntryList> runningListJobs;

    // an item is a complete directory
    QHash<QString /*url*/, DirItem *> itemsInUse;
    QCache<QString /*url*/, DirItem> itemsCached;

    typedef QHash<QString /*url*/, KCoreDirListerCacheDirectoryData> DirectoryDataHash;
    DirectoryDataHash directoryData;

    // Symlink-to-directories are registered here so that we can
    // find the url that changed, when kdirwatch tells us about
    // changes in the canonical url. (#213799)
    QHash<QUrl /*canonical path*/, QList<QUrl> /*dirlister urls*/> canonicalUrls;

    // Set of local files that we have changed recently (according to KDirWatch)
    // We temporize the notifications by keeping them 500ms in this list.
    QSet<QString /*path*/> pendingUpdates;
    QSet<QString /*path*/> pendingDirectoryUpdates;
    // The timer for doing the delayed updates
    QTimer pendingUpdateTimer;

    // Set of remote files that have changed recently -- but we can't emit those
    // changes yet, we need to wait for the "update" directory listing.
    // The cmp() call can't differ mimetypes since they are determined on demand,
    // this is why we need to remember those files here.
    QSet<KFileItem *> pendingRemoteUpdates;

    // the KDirNotify signals
    OrgKdeKDirNotifyInterface *kdirnotify;

    struct ItemInUseChange;
};

// Data associated with a directory url
// This could be in DirItem but only in the itemsInUse dict...
struct KCoreDirListerCacheDirectoryData {
    // A lister can be EITHER in listersCurrentlyListing OR listersCurrentlyHolding
    // but NOT in both at the same time.
    // But both lists can have different listers at the same time; this
    // happens if more listers are requesting url at the same time and
    // one lister was stopped during the listing of files.

    // Listers that are currently listing this url
    QList<KCoreDirLister *> listersCurrentlyListing;
    // Listers that are currently holding this url
    QList<KCoreDirLister *> listersCurrentlyHolding;

    void moveListersWithoutCachedItemsJob(const QUrl &url);
};

//const unsigned short KCoreDirListerCache::MAX_JOBS_PER_LISTER = 5;

// This job tells KCoreDirListerCache to emit cached items asynchronously from listDir()
// to give the KCoreDirLister user enough time for connecting to its signals, and so
// that KCoreDirListerCache behaves just like when a real KIO::Job is used: nothing
// is emitted during the openUrl call itself.
class KCoreDirLister::Private::CachedItemsJob : public KJob
{
    Q_OBJECT
public:
    CachedItemsJob(KCoreDirLister *lister, const QUrl &url, bool reload);

    /*reimp*/ void start()
    {
        QMetaObject::invokeMethod(this, "done", Qt::QueuedConnection);
    }

    // For updateDirectory() to cancel m_emitCompleted;
    void setEmitCompleted(bool b)
    {
        m_emitCompleted = b;
    }

    QUrl url() const
    {
        return m_url;
    }

protected:
    virtual bool doKill();

public Q_SLOTS:
    void done();

private:
    KCoreDirLister *m_lister;
    QUrl m_url;
    bool m_reload;
    bool m_emitCompleted;
};

#endif
