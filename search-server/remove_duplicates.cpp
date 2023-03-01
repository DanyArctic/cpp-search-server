#include "remove_duplicates.h"
#include "search_server.h"
#include "ostream"

void RemoveDuplicates(SearchServer& search_server)
{
	std::set<int> ids_to_remove;
	auto documents = search_server.GetDocuments();
	std::set <std::set<std::string>> words;
	size_t documents_quantity = words.size();
	for (int id : search_server)
	{
		words.emplace(documents[id]);
		if (documents_quantity == words.size())
		{
			search_server.RemoveDocument(id);
			std::cout << "Found duplicate document id " << id << std::endl;
			continue;
		}
		else
		{
			++documents_quantity;
		}
	}
	for (int id : ids_to_remove)
	{
  		search_server.RemoveDocument(id);
	}
}