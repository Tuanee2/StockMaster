#pragma once

#include <QNetworkAccessManager>
#include <QObject>
#include <QPointer>
#include <QString>

#include <memory>

class QNetworkReply;
class QJsonArray;
class QSaveFile;
class QUrl;

namespace stockmaster::infra::db {
class DatabaseService;
}

namespace stockmaster::ui::viewmodels {

class SettingsViewModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString currentVersion READ currentVersion NOTIFY stateChanged)
    Q_PROPERTY(QString latestVersion READ latestVersion NOTIFY stateChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY stateChanged)
    Q_PROPERTY(QString updateActionLabel READ updateActionLabel NOTIFY stateChanged)
    Q_PROPERTY(QString databasePath READ databasePath CONSTANT)
    Q_PROPERTY(QString downloadedFilePath READ downloadedFilePath NOTIFY stateChanged)
    Q_PROPERTY(bool checking READ checking NOTIFY stateChanged)
    Q_PROPERTY(bool downloading READ downloading NOTIFY stateChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY stateChanged)
    Q_PROPERTY(bool updateAvailable READ updateAvailable NOTIFY stateChanged)
    Q_PROPERTY(bool canUpdate READ canUpdate NOTIFY stateChanged)
    Q_PROPERTY(int downloadProgress READ downloadProgress NOTIFY stateChanged)

public:
    explicit SettingsViewModel(infra::db::DatabaseService &databaseService,
                               QObject *parent = nullptr);
    ~SettingsViewModel() override;

    [[nodiscard]] QString currentVersion() const;
    [[nodiscard]] QString latestVersion() const;
    [[nodiscard]] QString statusText() const;
    [[nodiscard]] QString updateActionLabel() const;
    [[nodiscard]] QString databasePath() const;
    [[nodiscard]] QString downloadedFilePath() const;
    [[nodiscard]] bool checking() const;
    [[nodiscard]] bool downloading() const;
    [[nodiscard]] bool busy() const;
    [[nodiscard]] bool updateAvailable() const;
    [[nodiscard]] bool canUpdate() const;
    [[nodiscard]] int downloadProgress() const;

    Q_INVOKABLE void checkForUpdates();
    Q_INVOKABLE void downloadUpdate();

signals:
    void stateChanged();

private:
    static QString formatVersionForDisplay(const QString &versionText);
    static QString normalizeVersion(const QString &versionText);
    static int compareVersions(const QString &left, const QString &right);
    static bool isMacAsset(const QString &assetName);
    static bool isWindowsAsset(const QString &assetName);

    [[nodiscard]] QString findCompatibleAssetUrl(const QJsonArray &assets,
                                                 QString &assetName) const;
    [[nodiscard]] QString resolveDownloadDirectory() const;
    [[nodiscard]] QString availableTargetFilePath() const;

    bool openDownloadedPackage();
    void startDownload(const QUrl &downloadUrl, const QString &targetFilePath);
    void setStatusText(const QString &value);
    void emitStateChanged();
    void resetDownloadState();

    infra::db::DatabaseService &m_databaseService;
    QNetworkAccessManager m_networkManager;
    QPointer<QNetworkReply> m_checkReply;
    QPointer<QNetworkReply> m_downloadReply;
    std::unique_ptr<QSaveFile> m_downloadFile;
    QString m_latestVersion;
    QString m_statusText;
    QString m_downloadedFilePath;
    QString m_availableAssetName;
    QUrl m_availableAssetUrl;
    bool m_checking = false;
    bool m_downloading = false;
    int m_downloadProgress = -1;
};

} // namespace stockmaster::ui::viewmodels
