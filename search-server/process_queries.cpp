#include "process_queries.h"
#include <execution>

std::vector<std::vector<Document>> ProcessQueries(const SearchServer& search_server, const std::vector<std::string>& queries)
{
	std::vector<std::vector<Document>> result(queries.size());
	std::transform(std::execution::par, queries.begin(), queries.end(), result.begin(),
		[&search_server](const std::string &query)
		{
			return search_server.FindTopDocuments(query);
		}
	);
	return result;
}

//std::list<Document> ProcessQueriesJoined(const SearchServer& search_server, const std::vector<std::string>& queries)
//{
//	std::list<Document> documents;
//	for (const std::vector<Document> &bunch_of_documents : ProcessQueries(search_server, queries))
//	{
//		documents.insert(documents.end(), bunch_of_documents.begin(), bunch_of_documents.end());
//	}
//	return documents;
//}

std::list<Document> ProcessQueriesJoined(const SearchServer& search_server, const std::vector<std::string>& queries)
{
return std::transform_reduce(
	std::execution::par,
	queries.begin(), queries.end(),
	std::list<Document>{},
	[](std::list<Document> lhs, std::list<Document> rhs)
	{
		//const_cast<std::list<Document>&>(lhs).splice(const_cast<std::list<Document>&>(lhs).end(), const_cast<std::list<Document>&>(rhs));
		lhs.splice(lhs.end(), rhs); // sacrificing constancy for speed
		return lhs;
	},
	[&search_server](const std::string& query)
	{
		std::vector<Document> documents = search_server.FindTopDocuments(query);
		return std::list<Document>{documents.begin(), documents.end()};
	});
}