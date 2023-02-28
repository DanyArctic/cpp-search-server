#include "remove_duplicates.h"
#include "search_server.h"
#include "ostream"

void RemoveDuplicates(SearchServer& search_server)
{
	auto documents = search_server.GetDocuments();
	std::set<int> ids_to_remove;
	for (auto it = documents.begin(); it != documents.end(); it = next(it))
	{
		for (auto internal_it = next(it); internal_it != documents.end(); internal_it = next(internal_it))
		{
			if (it->second == internal_it->second)
			{
				std::cout << "Found duplicate document id " << internal_it->first << std::endl;
				ids_to_remove.emplace(internal_it->first);
			}
		}
	}
	for (int id : ids_to_remove)
	{
  		search_server.RemoveDocument(id);
	}
}