#include "remove_duplicates.h"
#include "search_server.h"
#include "ostream"

void RemoveDuplicates(SearchServer& search_server)
{
	std::set<int> ids_to_remove;
	auto documents = search_server.GetDocuments();
	std::set <std::set<std::string_view>> words;
	size_t documents_quantity = words.size();
	std::map<std::string_view, double> words_freq;
	for (int id : search_server)
	{
		words_freq = search_server.GetWordFrequencies(id);
		std::set<std::string_view> bunch_of_words_to_add;
		for (auto& it : words_freq)
		{
			bunch_of_words_to_add.emplace(it.first);
		}
		if (words.count(bunch_of_words_to_add))
		{
			ids_to_remove.emplace(id);
			std::cout << "Found duplicate document id " << id << std::endl;
			continue;
		}
		else
		{
			words.emplace(bunch_of_words_to_add);
		}
		/*	words.emplace(bunch_of_words_to_add);
			if (documents_quantity == words.size())
			{
				ids_to_remove.emplace(id);
				std::cout << "Found duplicate document id " << id << std::endl;
				continue;
			}*/
		/*words.emplace(documents[id]);
		if (documents_quantity == words.size())
		{
			search_server.RemoveDocument(id);
			std::cout << "Found duplicate document id " << id << std::endl;
			continue;
		}*/
			/*else
			{
				++documents_quantity;
			}*/
	}
	for (int id : ids_to_remove)
	{
  		search_server.RemoveDocument(id);
	}
}