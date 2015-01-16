#ifndef API_CLIENT_H_
#define API_CLIENT_H_

#include <api/config.h>

#include <atomic>
#include <deque>
#include <map>
#include <string>
#include <core/net/http/request.h>
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


    /**
     * A list of games
     */
    typedef std::deque<Game> GameList;

    Client(Config::Ptr config);

    virtual ~Client() = default;

    /**
     * Get the weather forecast for the specified location and duration
     */
    virtual GameList gamesFor(QDate date, bool showScores);
    virtual GameList games();

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
};

}

#endif // API_CLIENT_H_

