#include <boost/algorithm/string/trim.hpp>

#include <scope/localization.h>
#include <scope/query.h>

#include <unity/scopes/Annotation.h>
#include <unity/scopes/CategorisedResult.h>
#include <unity/scopes/CategoryRenderer.h>
#include <unity/scopes/QueryBase.h>
#include <unity/scopes/SearchReply.h>
#include <unity/scopes/Department.h>

#include <iomanip>
#include <sstream>

namespace sc = unity::scopes;
namespace alg = boost::algorithm;

using namespace std;
using namespace api;
using namespace scope;


/**
 * Define the layout for the forecast results
 *
 * The icon size is small, and ask for the card layout
 * itself to be horizontal. I.e. the text will be placed
 * next to the image.
 */
const static string TEMPLATE =
        R"(
{
        "schema-version": 1,
        "template": {
        "category-layout": "grid",
        "card-layout": "horizontal",
        "card-size": "medium"
        },
        "components": {
        "title": "title",
        "art" : {
        "field": "art"
        },
        "subtitle": "subtitle"
        }
        }
        )";

Query::Query(const sc::CannedQuery &query, const sc::SearchMetadata &metadata,
             Config::Ptr config) :
    sc::SearchQueryBase(query, metadata), client_(config), config_(config) {
}

void Query::initScope() {
    s_showScores = settings()["showScores"].get_bool();
}

void Query::cancelled() {
    client_.cancel();
}


void Query::run(sc::SearchReplyProxy const& reply) {
    initScope();
    try {
        // Create the root department
        sc::Department::SPtr today = sc::Department::create("", query(), "Today's Games");
        sc::Department::SPtr yesterday = sc::Department::create("yesterday", query(), "Yesterday's Games");
        sc::Department::SPtr tomorrow = sc::Department::create("tomorrow", query(), "Tomorrow's Games");

        today->set_subdepartments({yesterday, tomorrow});
        reply->register_departments(today);

        // Start by getting information about the query
        const sc::CannedQuery &query(sc::SearchQueryBase::query());

        // Trim the query string of whitespace
        string query_string = alg::trim_copy(query.query_string());

        Client::GameList games;
        if (!query.department_id().empty()) {
            QDate date = QDate::currentDate();
            if (query.department_id() == "yesterday") {
                date = date.addDays(-1);
            } else {
                date = date.addDays(1);
            }
            games = client_.gamesFor(date, s_showScores);
        } else if (query_string.empty()) {
            games = client_.gamesFor(QDate::currentDate(), s_showScores);
        } else {
            // TODO: search filters for team
        }

        // Register a category for the current weather, with the title we just built
        auto forecast_cat = reply->register_category("schedule", _("Schedule"), "", sc::CategoryRenderer(TEMPLATE));

        // For each of the forecast days
        for (const auto &game : games) {
            // Create a result
            sc::CategorisedResult res(forecast_cat);

            // We must have a URI
            QString uri("http://www.nhl.com/gamecenter/en/icetracker?id=%1");
            res.set_uri(uri.arg(game.id).toStdString());
            res.set_title((game.away.name + " at " + game.home.name).toStdString());

            QString away = game.away.name.toLower().replace(" ", "");
            QString home = game.home.name.toLower().replace(" ", "");
            std::string homeLogo = (config_->cache_dir.c_str() + away + "_" + home + ".svg").toStdString();
            // Set the rest of the attributes
            res["subtitle"] = game.start.toStdString();
            res["description"] = "";
            res.set_art(homeLogo);

            // Push the result
            if (!reply->push(res)) {
                // If we fail to push, it means the query has been cancelled.
                // So don't continue;
                return;
            }
        }


    } catch (domain_error &e) {
        // Handle exceptions being thrown by the client API
        cerr << e.what() << endl;
        reply->error(current_exception());
    }
}

