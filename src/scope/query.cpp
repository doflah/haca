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


const static string TEMPLATE =
        R"(
{
        "schema-version": 1,
        "template": {
        "category-layout": "grid",
        "card-layout": "vertical",
        "card-size": "small"
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

const static string TEMPLATE_TODAY =
        R"(
{
        "schema-version": 1,
        "template": {
        "category-layout": "grid",
        "card-layout": "horizontal",
        "card-size": "small"
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

sc::CategorisedResult Query::buildResult(sc::Category::SCPtr cat, Client::Game game, bool full) {
    sc::CategorisedResult res(cat);
    res.set_uri(config_->track_url.arg(game.id).toStdString());
    if (full) {
        res.set_title((game.away.name + " at " + game.home.name).toStdString());
    } else {
        res.set_title((game.away.abbr + " at " + game.home.abbr).toStdString());
    }

    QString away = game.away.name.toLower().replace(" ", "");
    QString home = game.home.name.toLower().replace(" ", "");
    std::string homeLogo = (config_->game_logo.arg(home).arg(away)).toStdString();
    res["long-title"] = (game.away.name + " at " + game.home.name).toStdString();
    res["subtitle"] = game.start.toStdString();
    res.set_art(homeLogo);

    return (res);
}

void Query::run(sc::SearchReplyProxy const& reply) {
    initScope();
    try {
        Client::Schedule today  = client_.gamesFor(QDate::currentDate(), s_showScores);
        Client::Schedule next;
        Client::Schedule prev;

        QDate nope;

        if (today.next != nope) {
            next = client_.gamesFor(today.next, s_showScores);
        }

        if (today.prev != nope) {
            prev = client_.gamesFor(today.prev, s_showScores);
        }

        // TODO: let user query for a specific team
        //const sc::CannedQuery &query(sc::SearchQueryBase::query());
        //string query_string = alg::trim_copy(query.query_string());

        if (today.games.size() > 0) {
            auto cat = reply->register_category("schedule", _("Today"), "", sc::CategoryRenderer(TEMPLATE_TODAY));
            for (const auto &game : today.games) {
                sc::CategorisedResult res = buildResult(cat, game, true);
                if (!reply->push(res)) return;
            }
        }

        if (next.games.size() > 0) {
            string label = today.date.daysTo(next.date) == 1 ? _("Tomorrow") : next.date.toString(Qt::SystemLocaleLongDate).toStdString();
            auto cat = reply->register_category("next", label, "", sc::CategoryRenderer(TEMPLATE));
            for (const auto &game : next.games) {
                sc::CategorisedResult res = buildResult(cat, game, false);
                if (!reply->push(res)) return;
            }
        }

        if (prev.games.size() > 0) {
            string label = prev.date.daysTo(today.date) == 1 ? _("Yesterday") : prev.date.toString(Qt::SystemLocaleLongDate).toStdString();
            auto cat = reply->register_category("prev", label, "", sc::CategoryRenderer(TEMPLATE));
            for (const auto &game : prev.games) {
                sc::CategorisedResult res = buildResult(cat, game, false);
                if (!reply->push(res)) return;
            }
        }


    } catch (domain_error &e) {
        // Handle exceptions being thrown by the client API
        cerr << e.what() << endl;
        reply->error(current_exception());
    }
}

