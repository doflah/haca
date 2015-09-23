#ifndef SCOPE_QUERY_H_
#define SCOPE_QUERY_H_

#include <api/client.h>

#include <unity/scopes/SearchQueryBase.h>
#include <unity/scopes/ReplyProxyFwd.h>
#include <unity/scopes/Category.h>

namespace scope {

/**
 * Represents an individual query.
 *
 * A new Query object will be constructed for each query. It is
 * given query information, metadata about the search, and
 * some scope-specific configuration.
 */
class Query: public unity::scopes::SearchQueryBase {
public:
    Query(const unity::scopes::CannedQuery &query,
          const unity::scopes::SearchMetadata &metadata, api::Config::Ptr config);

    ~Query() = default;

    void cancelled() override;

    void run(const unity::scopes::SearchReplyProxy &reply) override;

private:

    unity::scopes::CategorisedResult buildResult(unity::scopes::Category::SCPtr cat, api::Client::Game game, bool abbr);

    api::Config::Ptr config_;
    api::Client client_;
    void initScope();
    bool s_showScores;
};

}

#endif // SCOPE_QUERY_H_


