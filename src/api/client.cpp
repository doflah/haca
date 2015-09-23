#include <api/client.h>

#include <core/net/error.h>
#include <core/net/http/content_type.h>
#include <core/net/http/response.h>
#include <QVariantMap>

namespace http = core::net::http;
namespace net = core::net;

using namespace api;
using namespace std;

Client::Client(Config::Ptr config) :
    config_(config), cancelled_(false) {
    #define add_team(abbr, location, name) teamLookup[abbr] = teamLookup[QString(name).toLower().replace(" ", "")] = {abbr, location, name};

    add_team("ANA", "Anaheim", "Ducks")
    add_team("BOS", "Boston", "Bruins")
    add_team("BUF", "Buffalo", "Sabres")
    add_team("CGY", "Calgary", "Flames")
    add_team("CAR", "Carolina", "Hurricanes")
    add_team("CHI", "Chicago", "Blackhawks")
    add_team("COL", "Colorado", "Avalanche")
    add_team("CBJ", "Columbus", "Blue Jackets")
    add_team("DAL", "Dallas", "Stars")
    add_team("DET", "Detroit", "Red Wings")
    add_team("EDM", "Edmonton", "Oilers")
    add_team("FLA", "Florida", "Panthers")
    add_team("LAK", "Los Angeles", "Kings")
    add_team("MIN", "Minnesota", "Wild")
    add_team("MTL", "Montreal", "Canadiens")
    add_team("NJD", "New Jersey", "Devils")
    add_team("NSH", "Nashville", "Predators")
    add_team("NYI", "New York", "Islanders")
    add_team("NYR", "New York", "Rangers")
    add_team("OTT", "Ottowa", "Senators")
    add_team("PHI", "Philadelphia", "Flyers")
    add_team("ARI", "Arizona", "Coyotes")
    add_team("PIT", "Pittsburgh", "Penguins")
    add_team("SJS", "San Jose", "Sharks")
    add_team("STL", "St. Louis", "Blues")
    add_team("TBL", "Tampa Bay", "Lightning")
    add_team("TOR", "Toronto", "Maple Leafs")
    add_team("VAN", "Vancouver", "Canucks")
    add_team("WSH", "Washington", "Capitals")
    add_team("WPG", "Winnipeg", "Jets")

}


void Client::get(const net::Uri::Path &path,
                 const net::Uri::QueryParameters &parameters, QJsonDocument &root, int padding) {
    // Create a new HTTP client
    auto client = http::make_client();

    // Start building the request configuration
    http::Request::Configuration configuration;

    // Build the URI from its components
    net::Uri uri = net::make_uri(config_->apiroot, path, parameters);
    configuration.uri = client->uri_to_string(uri);

    // Give out a user agent string
    configuration.header.add("User-Agent", config_->user_agent);

    // Build a HTTP request object from our configuration
    auto request = client->head(configuration);

    try {
        // Synchronously make the HTTP request
        // We bind the cancellable callback to #progress_report
        auto response = request->execute(
                    bind(&Client::progress_report, this, placeholders::_1));

        // Check that we got a sensible HTTP status code
        if (response.status != http::Status::ok) {
            throw domain_error(response.body);
        }
        std::string body = padding == 0 ? response.body : response.body.substr(padding, response.body.size() - padding - 2);
        // Parse the JSON from the response
        root = QJsonDocument::fromJson(body.c_str());

    } catch (net::Error &) {
    }
}

bool gameOrder(const Client::Game &v1, const Client::Game &v2)
{
    return (v1.away.name < v2.away.name);
}

QString Client::getTeamIcon(QString icon, std::shared_ptr<http::Client> client) {
    QString filename = config_->team_logo.arg(icon);
    QFile cached(filename);
    if (!cached.exists()) {
        http::Request::Configuration configuration;
        configuration.uri = config_->remote_logo.arg(icon).toStdString();
        configuration.header.add("User-Agent", config_->user_agent);
        auto request = client->head(configuration);
        try {
            auto response = request->execute(NULL);
            if (response.status != http::Status::ok) {
                throw domain_error(response.body);
            }
            cached.open(QFile::ReadWrite);
            QTextStream out(&cached);
            out << response.body.c_str();
            cached.close();
        } catch (net::Error &) {
        }
    }
    return (filename);
}

QString inProgress("%1 (%2) - %3 (%4) / %5");
QString awayWins("<b>%1 (%2)</b> - %3 (%4) / %5");
QString homeWins("%1 (%2) - <b>%3 (%4)</b> / %5");

QString dateFormat("MM/dd/yyyy");

Client::Schedule Client::gamesFor(QDate date, bool showScores) {
    QJsonDocument root;
    Schedule schedule;
    get( { QString("%1-%2-%3.jsonp").arg(date.year())
           .arg(date.month(), 2, 10, QChar('0'))
           .arg(date.day(), 2, 10, QChar('0')).toStdString()}, {}, root, 15);

    QVariantList games = root.toVariant().toMap()["games"].toList();

    auto client = http::make_client();
    for (int i = 0; i < games.size(); i++) {
        QVariantMap json = games.at(i).toMap();
        Game game;
        game.away = teamLookup[json["ata"].toString()];
        game.home = teamLookup[json["hta"].toString()];
        game.id = json["id"].toInt();
        int status = json["gs"].toInt();
        if (status == 5) {
            if (showScores) {
                game.start = (json["atc"].toString() == "winner" ? awayWins : homeWins)
                        .arg(json["ata"].toString()).arg(json["ats"].toInt())
                        .arg(json["hta"].toString()).arg(json["hts"].toInt()).arg(json["bs"].toString());
            } else {
                game.start = json["bs"].toString();
            }
        } else if (status == 3 || status == 4) { // in progress
            if (showScores) {
                game.start = inProgress.arg(json["ata"].toString()).arg(json["ats"].toInt())
                        .arg(json["hta"].toString()).arg(json["hts"].toInt()).arg(json["bs"].toString());
            } else {
                game.start = json["bs"].toString();
            }
        } else {
            game.start = json["bs"].toString();
        }
        QString away = game.away.name.toLower().replace(" ", "");
        QString home = game.home.name.toLower().replace(" ", "");
        QFile cachedAway(getTeamIcon(away, client));
        QFile cachedHome(getTeamIcon(home, client));
        // This isn't really svg parsing, the source files just happen to be on nice line breaks
        // so I can go line by line and copy the ones I care about
        QFile combiner(config_->game_logo.arg(home).arg(away));
        if (!combiner.exists() && cachedAway.open(QFile::ReadOnly) && cachedHome.open(QFile::ReadOnly)) {
            combiner.open(QFile::WriteOnly);
            QTextStream inAway(&cachedAway), inHome(&cachedHome), out(&combiner);
            QString line;
            out << inAway.readLine() << endl;
            out << inAway.readLine() << endl;
            out << inAway.readLine() << endl;
            out << inAway.readLine() << endl;
            inAway.readLine(); // consume the sizing information and replace it with our own
            // make the image a bit larger so two team logos can fit
            out << "	 width='36px' height='24px' viewBox='0 0 36 24' enable-background='new 0 0 36 24' xml:space='preserve'>" << endl;

            out << "<g>" << endl;
            while ((line = inAway.readLine()) != "</svg>") out << line << endl;
            out << "</g>" << endl;

            // consume the first few lines of the file
            inHome.readLine(); inHome.readLine(); inHome.readLine(); inHome.readLine(); inHome.readLine();
            out << "<g transform='translate(12, 8)'>" << endl;
            while ((line = inHome.readLine()) != "</svg>") out << line << endl;
            out << "</g>" << endl << "</svg>";

            combiner.close();
        }
        cachedAway.close();
        cachedHome.close();

        schedule.games.push_back(game);
    }

    schedule.date = date;
    schedule.prev = QDate::fromString(root.toVariant().toMap()["prevDate"].toString(), dateFormat);
    schedule.next = QDate::fromString(root.toVariant().toMap()["nextDate"].toString(), dateFormat);

    qSort(schedule.games.begin(), schedule.games.end(), gameOrder);

    QDir cache(config_->cache_dir);
    QDateTime now = QDateTime::currentDateTime();

    // remove game_ logos that are older than 72 hours, just to keep the cache clean
    int seventy_two_hours = 72 * 60 * 60 * 1000;
    QFileInfoList list = cache.entryInfoList(QDir::Dirs| QDir::Files | QDir::NoDotAndDotDot);
    for (int i = 0; i < list.size(); i++) {
        QFileInfo file = list.at(i);
        if (file.fileName().startsWith("game") &&
                now.currentMSecsSinceEpoch() - file.created().currentMSecsSinceEpoch() >  seventy_two_hours) {
            QFile(file.absoluteFilePath()).remove();
        }
    }

    return (schedule);
}

http::Request::Progress::Next Client::progress_report(
        const http::Request::Progress&) {

    return cancelled_ ?
                http::Request::Progress::Next::abort_operation :
                http::Request::Progress::Next::continue_operation;
}

void Client::cancel() {
    cancelled_ = true;
}

Config::Ptr Client::config() {
    return config_;
}

