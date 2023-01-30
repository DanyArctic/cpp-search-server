#include "request_queue.h"

RequestQueue::RequestQueue(const SearchServer& search_server)
	: search_server_(search_server),
	no_result_requests_(0)
{}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentStatus status)
{
	const auto search_results = search_server_.FindTopDocuments(raw_query, status);
	AddRequest(search_results);
	return search_results;
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query)
{
	const auto search_results = search_server_.FindTopDocuments(raw_query);
	AddRequest(search_results);
	return search_results;
}

int RequestQueue::GetNoResultRequests() const
{
	return no_result_requests_;
}

void RequestQueue::AddRequest(const std::vector<Document>& search_results)
{
	QueryResult new_result;
	if (search_results.empty())
	{
		new_result.no_result = true;
		++no_result_requests_;
	}
	requests_.push_back(new_result);
	if (requests_.size() > min_in_day_)
	{
		if (requests_.front().no_result == true)
		{
			--no_result_requests_;
		}
		requests_.pop_front();
	}
}