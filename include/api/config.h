#ifndef API_CONFIG_H_
#define API_CONFIG_H_

#include <memory>
#include <string>
#include <QString>

namespace api {

struct Config {
    typedef std::shared_ptr<Config> Ptr;

    /*
     * The root of all API request URLs
     */
    std::string apiroot { "http://live.nhl.com/GameData/GCScoreboard" };

    /*
     * The custom HTTP user agent string for this library
     */
    std::string user_agent { "NHL Scope-board 0.1; " };

    QString cache_dir;
    QString track_url   { "http://www.nhl.com/gamecenter/en/icetracker?id=%1" }; // %1 is the game id
    QString remote_logo { "http://cdn.nhle.com/nhl/images/logos/teams/%1_logo.svgz" }; // %1 is team name
    QString team_logo   { "%1/logo_%2.svg" };  // %1 is the cache dir, %2 is the team name
    QString game_logo   { "%1/game_%2_%3.svg" };  // %1 is the cache dir, %2 is the home team, %3 is the away team

};

}

#endif /* API_CONFIG_H_ */

