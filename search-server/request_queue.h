#pragma once
#include "search_server.h"
#include "document.h"
#include <deque>

class RequestQueue
{
public:
	explicit RequestQueue(const SearchServer& search_server);

	// make "wrappers" for all search methods to store results for our statistics
	template <typename DocumentPredicate>
	std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate)
	{
		const auto search_results = search_server_.FindTopDocuments(raw_query, document_predicate);
		AddRequest(search_results);
		return search_results;
	}
	std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);

	std::vector<Document> AddFindRequest(const std::string& raw_query);

	int GetNoResultRequests() const;
private:
	const SearchServer& search_server_;
	struct QueryResult
	{
		bool no_result = false;
	};
	std::deque<QueryResult> requests_;
	int no_result_requests_;
	const static int min_in_day_ = 1440;

	void AddRequest(const std::vector<Document>& search_results);
};