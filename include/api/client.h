#ifndef API_CLIENT_H_
#define API_CLIENT_H_

#include <api/config.h>

#include <atomic>
#include <deque>
#include <map>
#include <string>
#include <core/net/http/request.h>
#include <core/net/http/client.h>
#include <core/net/uri.h>

#include <QJsonDocument>
#include <QDateTime>
#include <QTimeZone>
#include <QDir>
#include <QFile>
#include <QTextStream>

namespace api {

/**
 * Provide a nice way to access the HTTP API.
 *
 * We don't want our scope's code to be mixed together with HTTP and JSON handling.
 */
class Client {
public:

    struct Team {
        QString abbr;
        QString location;
        QString name;
    };

    struct Game {
        int id;
        int homeScore;
        int awayScore;
        Team home;
        Team away;
        QString start;
    };

    struct Schedule {
        std::vector<Game> games;
        QDate prev;
        QDate date;
        QDate next;
    };

    Client(Config::Ptr config);

    virtual ~Client() = default;

    /**
     * Get the schedule for a particular day
     */
    virtual Schedule gamesFor(QDate date, bool showScores);

    /**
     * Cancel any pending queries (this method can be called from a different thread)
     */
    virtual void cancel();

    virtual Config::Ptr config();

protected:
    void get(const core::net::Uri::Path &path,
             const core::net::Uri::QueryParameters &parameters,
             QJsonDocument &root, int padding = 0);
    /**
     * Progress callback that allows the query to cancel pending HTTP requests.
     */
    core::net::http::Request::Progress::Next progress_report(
            const core::net::http::Request::Progress& progress);

    /**
     * Hang onto the configuration information
     */
    Config::Ptr config_;

    /**
     * Thread-safe cancelled flag
     */
    std::atomic<bool> cancelled_;
    std::map<QString, Team> teamLookup;

private:
    QString getTeamIcon(QString icon, std::shared_ptr<core::net::http::Client> client);
};

}

#endif // API_CLIENT_H_

