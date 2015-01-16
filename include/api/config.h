#ifndef API_CONFIG_H_
#define API_CONFIG_H_

#include <memory>
#include <string>

namespace api {

struct Config {
    typedef std::shared_ptr<Config> Ptr;

    /*
     * The root of all API request URLs
     */
    std::string apiroot { "http://live.nhl.com/GameData/GCScoreboard" };
    //std::string apiroot { "http://localhost:8080" };

    /*
     * The custom HTTP user agent string for this library
     */
    std::string user_agent { "NHL Scope-board 0.1; " };
};

}

#endif /* API_CONFIG_H_ */

