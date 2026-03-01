#include "stockmaster/ui/viewmodels/settings_view_model.h"

#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSaveFile>
#include <QStandardPaths>
#include <QUrl>
#include <QVersionNumber>
#include <QtGlobal>

#include "stockmaster/infra/db/database_service.h"

namespace stockmaster::ui::viewmodels {

namespace {

const QUrl kLatestReleaseUrl(QStringLiteral("https://api.github.com/repos/Tuanee2/StockMaster/releases/latest"));

}

SettingsViewModel::SettingsViewModel(infra::db::DatabaseService &databaseService, QObject *parent)
    : QObject(parent)
    , m_databaseService(databaseService)
    , m_statusText(QStringLiteral("Bấm \"Kiểm tra phiên bản\" để kiểm tra GitHub Release mới, sau đó bấm \"Cập nhật\" khi có bản phù hợp."))
{
}

SettingsViewModel::~SettingsViewModel()
{
    if (m_checkReply) {
        m_checkReply->abort();
    }

    if (m_downloadReply) {
        m_downloadReply->abort();
    }

    resetDownloadState();
}

QString SettingsViewModel::currentVersion() const
{
    const QString version = QCoreApplication::applicationVersion().trimmed();
    if (version.isEmpty()) {
        return QStringLiteral("0.0.0");
    }

    return formatVersionForDisplay(version);
}

QString SettingsViewModel::latestVersion() const
{
    return formatVersionForDisplay(m_latestVersion);
}

QString SettingsViewModel::statusText() const
{
    return m_statusText;
}

QString SettingsViewModel::updateActionLabel() const
{
    if (m_downloading) {
        return QStringLiteral("Đang tải bản cập nhật...");
    }

    if (!canUpdate()) {
        return QStringLiteral("Cập nhật");
    }

    const QFileInfo targetInfo(availableTargetFilePath());
    if (targetInfo.exists() && targetInfo.size() > 0) {
        return QStringLiteral("Mở gói cập nhật");
    }

    return QStringLiteral("Tải và mở bản cập nhật");
}

QString SettingsViewModel::databasePath() const
{
    return QDir::toNativeSeparators(m_databaseService.databaseFilePath());
}

QString SettingsViewModel::downloadedFilePath() const
{
    return m_downloadedFilePath;
}

bool SettingsViewModel::checking() const
{
    return m_checking;
}

bool SettingsViewModel::downloading() const
{
    return m_downloading;
}

bool SettingsViewModel::busy() const
{
    return m_checking || m_downloading;
}

bool SettingsViewModel::updateAvailable() const
{
    return !m_latestVersion.isEmpty() && compareVersions(m_latestVersion, currentVersion()) > 0;
}

bool SettingsViewModel::canUpdate() const
{
    return updateAvailable() && m_availableAssetUrl.isValid() && !m_availableAssetName.isEmpty();
}

int SettingsViewModel::downloadProgress() const
{
    return m_downloadProgress;
}

void SettingsViewModel::checkForUpdates()
{
    if (busy()) {
        return;
    }

    m_checking = true;
    m_availableAssetName.clear();
    m_availableAssetUrl = QUrl();
    m_downloadProgress = -1;
    setStatusText(QStringLiteral("Đang kiểm tra phiên bản mới từ GitHub Releases..."));
    emitStateChanged();

    QNetworkRequest request(kLatestReleaseUrl);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      QStringLiteral("StockMaster/%1").arg(currentVersion()));
    request.setRawHeader("Accept", "application/vnd.github+json");
    m_checkReply = m_networkManager.get(request);

    connect(m_checkReply, &QNetworkReply::finished, this, [this]() {
        QNetworkReply *reply = m_checkReply;
        m_checkReply = nullptr;
        m_checking = false;

        if (reply == nullptr) {
            setStatusText(QStringLiteral("Yêu cầu kiểm tra cập nhật đã bị hủy."));
            emitStateChanged();
            return;
        }

        reply->deleteLater();

        const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (reply->error() != QNetworkReply::NoError) {
            m_latestVersion.clear();
            if (httpStatus == 404) {
                setStatusText(QStringLiteral("Chưa có GitHub Release nào được publish để kiểm tra cập nhật."));
            } else {
                setStatusText(QStringLiteral("Không thể kiểm tra cập nhật: %1").arg(reply->errorString()));
            }

            emitStateChanged();
            return;
        }

        const QJsonDocument document = QJsonDocument::fromJson(reply->readAll());
        if (!document.isObject()) {
            m_latestVersion.clear();
            setStatusText(QStringLiteral("Phản hồi kiểm tra cập nhật không hợp lệ."));
            emitStateChanged();
            return;
        }

        const QJsonObject release = document.object();
        const QString releaseVersion = normalizeVersion(release.value(QStringLiteral("tag_name")).toString());
        if (releaseVersion.isEmpty()) {
            m_latestVersion.clear();
            setStatusText(QStringLiteral("GitHub Release chưa có tag version hợp lệ."));
            emitStateChanged();
            return;
        }

        m_latestVersion = releaseVersion;
        if (compareVersions(m_latestVersion, currentVersion()) <= 0) {
            m_availableAssetName.clear();
            m_availableAssetUrl = QUrl();
            setStatusText(QStringLiteral("Bạn đang dùng bản mới nhất (%1).").arg(currentVersion()));
            emitStateChanged();
            return;
        }

        QString assetName;
        const QString assetUrl = findCompatibleAssetUrl(release.value(QStringLiteral("assets")).toArray(),
                                                        assetName);
        if (assetUrl.isEmpty()) {
            m_availableAssetName.clear();
            m_availableAssetUrl = QUrl();
            setStatusText(QStringLiteral("Đã có bản %1 nhưng chưa có gói cài đặt phù hợp cho nền tảng hiện tại.")
                              .arg(m_latestVersion));
            emitStateChanged();
            return;
        }

        m_availableAssetName = assetName;
        m_availableAssetUrl = QUrl(assetUrl);

        const QFileInfo targetInfo(availableTargetFilePath());
        if (targetInfo.exists() && targetInfo.size() > 0) {
            m_downloadedFilePath = QDir::toNativeSeparators(targetInfo.filePath());
            setStatusText(QStringLiteral("Đã tìm thấy bản %1 và đã có sẵn gói tại %2. Bấm \"Cập nhật\" để mở gói cài đặt. DB hiện tại sẽ được giữ nguyên.")
                              .arg(m_latestVersion, m_downloadedFilePath));
            emitStateChanged();
            return;
        }

        setStatusText(QStringLiteral("Đã tìm thấy bản %1. Bấm \"Cập nhật\" để tải và mở gói cài đặt phù hợp cho nền tảng hiện tại.")
                          .arg(m_latestVersion));
        emitStateChanged();
    });
}

void SettingsViewModel::downloadUpdate()
{
    if (busy()) {
        return;
    }

    if (!canUpdate()) {
        setStatusText(QStringLiteral("Chưa có bản cập nhật sẵn sàng. Hãy bấm \"Kiểm tra phiên bản\" trước."));
        emitStateChanged();
        return;
    }

    const QString downloadDirectory = resolveDownloadDirectory();
    if (downloadDirectory.isEmpty()) {
        setStatusText(QStringLiteral("Không xác định được thư mục tải bản cập nhật."));
        emitStateChanged();
        return;
    }

    QDir directory(downloadDirectory);
    if (!directory.mkpath(QStringLiteral("."))) {
        setStatusText(QStringLiteral("Không thể tạo thư mục lưu bản cập nhật: %1")
                          .arg(QDir::toNativeSeparators(downloadDirectory)));
        emitStateChanged();
        return;
    }

    const QString targetFilePath = availableTargetFilePath();
    if (targetFilePath.isEmpty()) {
        setStatusText(QStringLiteral("Thiếu thông tin gói cài đặt để cập nhật."));
        emitStateChanged();
        return;
    }

    const QFileInfo targetInfo(targetFilePath);
    if (targetInfo.exists() && targetInfo.size() > 0) {
        m_downloadedFilePath = QDir::toNativeSeparators(targetInfo.filePath());
        const bool opened = openDownloadedPackage();
        if (opened) {
            setStatusText(QStringLiteral("Đang mở gói cập nhật %1 tại %2. Cài đặt gói mới sẽ giữ nguyên DB local tại %3.")
                              .arg(m_latestVersion,
                                   m_downloadedFilePath,
                                   QDir::toNativeSeparators(m_databaseService.databaseFilePath())));
        } else {
            setStatusText(QStringLiteral("Đã có sẵn gói cập nhật %1 tại %2 nhưng không thể tự mở. Cài đặt gói mới vẫn sẽ giữ nguyên DB local tại %3.")
                              .arg(m_latestVersion,
                                   m_downloadedFilePath,
                                   QDir::toNativeSeparators(m_databaseService.databaseFilePath())));
        }
        emitStateChanged();
        return;
    }

    startDownload(m_availableAssetUrl, targetFilePath);
}

QString SettingsViewModel::normalizeVersion(const QString &versionText)
{
    QString normalized = versionText.trimmed();
    while (!normalized.isEmpty() && normalized.at(0).toLower() == QLatin1Char('v')) {
        normalized.remove(0, 1);
    }

    const int dashIndex = normalized.indexOf(QLatin1Char('-'));
    if (dashIndex > 0) {
        normalized = normalized.left(dashIndex);
    }

    return normalized;
}

QString SettingsViewModel::formatVersionForDisplay(const QString &versionText)
{
    const QString normalized = normalizeVersion(versionText);
    if (normalized.isEmpty()) {
        return QString();
    }

    const QVersionNumber parsed = QVersionNumber::fromString(normalized);
    if (parsed.isNull()) {
        return normalized;
    }

    QList<int> segments = parsed.segments();
    while (segments.size() < 3) {
        segments.append(0);
    }

    return QVersionNumber(segments).toString();
}

int SettingsViewModel::compareVersions(const QString &left, const QString &right)
{
    const QVersionNumber leftVersion = QVersionNumber::fromString(normalizeVersion(left));
    const QVersionNumber rightVersion = QVersionNumber::fromString(normalizeVersion(right));
    return QVersionNumber::compare(leftVersion, rightVersion);
}

bool SettingsViewModel::isMacAsset(const QString &assetName)
{
    const QString lowered = assetName.trimmed().toLower();
    return lowered.contains(QStringLiteral("mac"))
           || lowered.endsWith(QStringLiteral(".dmg"))
           || lowered.endsWith(QStringLiteral(".pkg"));
}

bool SettingsViewModel::isWindowsAsset(const QString &assetName)
{
    const QString lowered = assetName.trimmed().toLower();
    return lowered.contains(QStringLiteral("windows"))
           || lowered.endsWith(QStringLiteral(".zip"))
           || lowered.endsWith(QStringLiteral(".msi"))
           || lowered.endsWith(QStringLiteral(".exe"));
}

QString SettingsViewModel::findCompatibleAssetUrl(const QJsonArray &assets, QString &assetName) const
{
    for (const QJsonValue &assetValue : assets) {
        if (!assetValue.isObject()) {
            continue;
        }

        const QJsonObject asset = assetValue.toObject();
        const QString candidateName = asset.value(QStringLiteral("name")).toString();
        const QString candidateUrl = asset.value(QStringLiteral("browser_download_url")).toString();
        if (candidateName.isEmpty() || candidateUrl.isEmpty()) {
            continue;
        }

#if defined(Q_OS_MACOS)
        if (!isMacAsset(candidateName)) {
            continue;
        }
#elif defined(Q_OS_WIN)
        if (!isWindowsAsset(candidateName)) {
            continue;
        }
#endif

        assetName = candidateName;
        return candidateUrl;
    }

    return QString();
}

QString SettingsViewModel::resolveDownloadDirectory() const
{
    QString basePath = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    if (basePath.isEmpty()) {
        basePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    }

    if (basePath.isEmpty()) {
        return QString();
    }

    return QDir(basePath).filePath(QStringLiteral("StockMasterUpdates"));
}

QString SettingsViewModel::availableTargetFilePath() const
{
    if (!canUpdate()) {
        return QString();
    }

    const QString downloadDirectory = resolveDownloadDirectory();
    if (downloadDirectory.isEmpty()) {
        return QString();
    }

    return QDir(downloadDirectory).filePath(QStringLiteral("%1-%2").arg(m_latestVersion, m_availableAssetName));
}

void SettingsViewModel::startDownload(const QUrl &downloadUrl, const QString &targetFilePath)
{
    resetDownloadState();

    m_downloadFile = std::make_unique<QSaveFile>(targetFilePath);
    if (!m_downloadFile->open(QIODevice::WriteOnly)) {
        m_downloadFile.reset();
        setStatusText(QStringLiteral("Không thể mở file để lưu bản cập nhật: %1")
                          .arg(QDir::toNativeSeparators(targetFilePath)));
        emitStateChanged();
        return;
    }

    QNetworkRequest request(downloadUrl);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      QStringLiteral("StockMaster/%1").arg(currentVersion()));
    m_downloadReply = m_networkManager.get(request);
    m_downloading = true;
    m_downloadProgress = 0;
    setStatusText(QStringLiteral("Đã có bản %1. Đang tải gói cập nhật về máy...").arg(m_latestVersion));
    emitStateChanged();

    connect(m_downloadReply, &QNetworkReply::readyRead, this, [this]() {
        if (m_downloadReply == nullptr || !m_downloadFile) {
            return;
        }

        const QByteArray chunk = m_downloadReply->readAll();
        if (!chunk.isEmpty() && m_downloadFile->write(chunk) != chunk.size()) {
            setStatusText(QStringLiteral("Không thể ghi dữ liệu cập nhật vào file đích."));
            m_downloadReply->abort();
        }
    });

    connect(m_downloadReply, &QNetworkReply::downloadProgress, this, [this](qint64 received, qint64 total) {
        if (total > 0) {
            m_downloadProgress = qBound(0, static_cast<int>((received * 100) / total), 100);
        } else {
            m_downloadProgress = -1;
        }

        emitStateChanged();
    });

    connect(m_downloadReply, &QNetworkReply::finished, this, [this, targetFilePath]() {
        QNetworkReply *reply = m_downloadReply;
        m_downloadReply = nullptr;
        m_downloading = false;

        if (reply == nullptr) {
            resetDownloadState();
            setStatusText(QStringLiteral("Tải bản cập nhật đã bị hủy."));
            emitStateChanged();
            return;
        }

        reply->deleteLater();

        if (m_downloadFile && reply->bytesAvailable() > 0) {
            const QByteArray remaining = reply->readAll();
            if (!remaining.isEmpty() && m_downloadFile->write(remaining) != remaining.size()) {
                setStatusText(QStringLiteral("Không thể ghi phần dữ liệu cuối của bản cập nhật."));
            }
        }

        if (reply->error() != QNetworkReply::NoError) {
            resetDownloadState();
            setStatusText(QStringLiteral("Tải bản cập nhật thất bại: %1").arg(reply->errorString()));
            emitStateChanged();
            return;
        }

        if (!m_downloadFile || !m_downloadFile->commit()) {
            resetDownloadState();
            setStatusText(QStringLiteral("Không thể hoàn tất việc lưu gói cập nhật."));
            emitStateChanged();
            return;
        }

        m_downloadFile.reset();
        m_downloadedFilePath = QDir::toNativeSeparators(targetFilePath);
        m_downloadProgress = 100;
        const bool opened = openDownloadedPackage();
        if (opened) {
            setStatusText(QStringLiteral("Đã tải xong bản %1 và đang mở gói cập nhật tại %2. Cài đặt gói mới sẽ giữ nguyên DB local tại %3.")
                              .arg(m_latestVersion,
                                   m_downloadedFilePath,
                                   QDir::toNativeSeparators(m_databaseService.databaseFilePath())));
        } else {
            setStatusText(QStringLiteral("Đã tải xong bản %1 về %2 nhưng không thể tự mở gói. Cài đặt gói mới vẫn sẽ giữ nguyên DB local tại %3.")
                              .arg(m_latestVersion,
                                   m_downloadedFilePath,
                                   QDir::toNativeSeparators(m_databaseService.databaseFilePath())));
        }
        emitStateChanged();
    });
}

bool SettingsViewModel::openDownloadedPackage()
{
    if (m_downloadedFilePath.trimmed().isEmpty()) {
        return false;
    }

    return QDesktopServices::openUrl(QUrl::fromLocalFile(m_downloadedFilePath));
}

void SettingsViewModel::setStatusText(const QString &value)
{
    m_statusText = value;
}

void SettingsViewModel::emitStateChanged()
{
    emit stateChanged();
}

void SettingsViewModel::resetDownloadState()
{
    if (m_downloadFile) {
        m_downloadFile->cancelWriting();
        m_downloadFile.reset();
    }
}

} // namespace stockmaster::ui::viewmodels
