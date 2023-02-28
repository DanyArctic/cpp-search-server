#include "test_example_functions.h"
#include "remove_duplicates.h"
#include "search_server.h"
#include "request_queue.h"
#include "paginator.h"
#include <iostream>
#include <string>

using namespace std;

int main()
{
	setlocale(LC_ALL, "Russian");
	TestSearchServer();
	// If this line appears, then all tests were successful
	std::cout << "Search server testing finished"s << std::endl;

	try
	{
		SearchServer search_server("и в на"s);
		search_server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 8, -3 });
		search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
		search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
		search_server.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED, { 9 });
		std::cout << "ACTUAL by default:"s << std::endl;
		for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s))
		{
			PrintDocument(document);
		}
		std::cout << "BANNED:"s << std::endl;
		for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::BANNED))
		{
			PrintDocument(document);
		}
		std::cout << "Even ids:"s << std::endl;
		for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id, [[maybe_unused]] DocumentStatus status, [[maybe_unused]] int rating)
			{
				return document_id % 2 == 0;
			}))
		{
			PrintDocument(document);
		}
			//search_server.AddDocument(1, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, { 1, 2 });
			//search_server.AddDocument(-1, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, { 1, 2 });
			//search_server.AddDocument(4, "большой пёс скво\x12рец"s, DocumentStatus::ACTUAL, { 1, 3, 2 });
			//search_server.FindTopDocuments("--пушистый"s);
			//search_server.GetDocumentId(44);
	}
	catch (const invalid_argument& argument)
	{
		std::cout << "invalid_argument: "s << argument.what() << std::endl;
	}
	catch (const out_of_range& value)
	{
		std::cout << "out_of_range: "s << value.what() << std::endl;
	}


	std::cout << "//////////////////////////////////////////////////////////////"s << '\n';


	try
	{
		SearchServer search_server("and with"s);
		search_server.AddDocument(1, "funny pet and nasty rat"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
		search_server.AddDocument(2, "funny pet with curly hair"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
		search_server.AddDocument(3, "big cat nasty hair"s, DocumentStatus::ACTUAL, { 1, 2, 8 });
		search_server.AddDocument(4, "big dog cat Vladislav"s, DocumentStatus::ACTUAL, { 1, 3, 2 });
		search_server.AddDocument(5, "big dog hamster Borya"s, DocumentStatus::ACTUAL, { 1, 1, 1 });
		const std::vector<Document> search_results = search_server.FindTopDocuments("curly dog"s);
		int page_size = 2;
		const auto pages = Paginate(search_results, page_size);
		// Displaying found documents page by page
		for (auto page = pages.begin(); page != pages.end(); ++page)
		{
			std::cout << *page << std::endl;
			std::cout << "Page break"s << std::endl;
		}
	}
	catch (const invalid_argument& argument)
	{
		std::cout << "invalid_argument: "s << argument.what() << std::endl;
	}
	catch (const out_of_range& value)
	{
		std::cout << "out_of_range: "s << value.what() << std::endl;
	}


	std::cout << "//////////////////////////////////////////////////////////////"s << '\n';


	try
	{
		SearchServer search_server("and in at"s);
		RequestQueue request_queue(search_server);
		search_server.AddDocument(1, "curly cat curly tail"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
		search_server.AddDocument(2, "curly dog and fancy collar"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
		search_server.AddDocument(3, "big cat fancy collar "s, DocumentStatus::ACTUAL, { 1, 2, 8 });
		search_server.AddDocument(4, "big dog sparrow Eugene"s, DocumentStatus::ACTUAL, { 1, 3, 2 });
		search_server.AddDocument(5, "big dog sparrow Vasiliy"s, DocumentStatus::ACTUAL, { 1, 1, 1 });
		// 1439 requests with empty result
		for (int i = 0; i < 1439; ++i)
		{
			request_queue.AddFindRequest("empty request"s);
		}
		// still 1439 requests with empty result
		request_queue.AddFindRequest("curly dog"s);
		// new day, first request deleted, 1438 requests with empty result
		request_queue.AddFindRequest("big collar"s);
		// first request deleted, 1437 requests with empty result
		request_queue.AddFindRequest("sparrow"s);
		std::cout << "Total empty requests: "s << request_queue.GetNoResultRequests() << std::endl;
	}
	catch (const invalid_argument& argument)
	{
		std::cout << "invalid_argument: "s << argument.what() << std::endl;
	}
	catch (const out_of_range& value)
	{
		std::cout << "out_of_range: "s << value.what() << std::endl;
	}

	try
	{
		SearchServer search_server("and with"s);

		AddDocument(search_server, 1, "funny pet and nasty rat"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
		AddDocument(search_server, 2, "funny pet with curly hair"s, DocumentStatus::ACTUAL, { 1, 2 });

		// дубликат документа 2, будет удалён
		AddDocument(search_server, 3, "funny pet with curly hair"s, DocumentStatus::ACTUAL, { 1, 2 });

		// отличие только в стоп-словах, считаем дубликатом
		AddDocument(search_server, 4, "funny pet and curly hair"s, DocumentStatus::ACTUAL, { 1, 2 });

		// множество слов такое же, считаем дубликатом документа 1
		AddDocument(search_server, 5, "funny funny pet and nasty nasty rat"s, DocumentStatus::ACTUAL, { 1, 2 });

		// добавились новые слова, дубликатом не является
		AddDocument(search_server, 6, "funny pet and not very nasty rat"s, DocumentStatus::ACTUAL, { 1, 2 });

		// множество слов такое же, как в id 6, несмотря на другой порядок, считаем дубликатом
		AddDocument(search_server, 7, "very nasty rat and not very funny pet"s, DocumentStatus::ACTUAL, { 1, 2 });

		// есть не все слова, не является дубликатом
		AddDocument(search_server, 8, "pet with rat and rat and rat"s, DocumentStatus::ACTUAL, { 1, 2 });

		// слова из разных документов, не является дубликатом
		AddDocument(search_server, 9, "nasty rat with curly hair"s, DocumentStatus::ACTUAL, { 1, 2 });

		cout << "Before duplicates removed: "s << search_server.GetDocumentCount() << endl;
		RemoveDuplicates(search_server);
		cout << "After duplicates removed: "s << search_server.GetDocumentCount() << endl;
	}
	catch (const invalid_argument& argument)
	{
		std::cout << "invalid_argument: "s << argument.what() << std::endl;
	}
	catch (const out_of_range& value)
	{
		std::cout << "out_of_range: "s << value.what() << std::endl;
	}
	return 0;
}