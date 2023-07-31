#include "remove_document_benchmark.h"
#include "parallel_search_benchmark.h"
#include "match_document_benchmark.h"
#include "test_example_functions.h"
#include "remove_duplicates.h"
#include "process_queries.h"
#include "search_server.h"
#include "request_queue.h"
#include "paginator.h"
#include <iostream>
#include <string>
#include <execution>

using namespace std;

string GenerateWord(mt19937& generator, int max_length)
{
	const int length = uniform_int_distribution(1, max_length)(generator);
	string word;
	word.reserve(length);
	for (int i = 0; i < length; ++i)
	{
		word.push_back(uniform_int_distribution(static_cast<int>('a'), static_cast<int>('z'))(generator));
	}
	return word;
}
vector<string> GenerateDictionary(mt19937& generator, int word_count, int max_length)
{
	vector<string> words;
	words.reserve(word_count);
	for (int i = 0; i < word_count; ++i)
	{
		words.push_back(GenerateWord(generator, max_length));
	}
	words.erase(unique(words.begin(), words.end()), words.end());
	return words;
}
string GenerateQuery(mt19937& generator, const vector<string>& dictionary, int word_count, double minus_prob = 0)
{
	string query;
	for (int i = 0; i < word_count; ++i)
	{
		if (!query.empty())
		{
			query.push_back(' ');
		}
		if (uniform_real_distribution<>(0, 1)(generator) < minus_prob)
		{
			query.push_back('-');
		}
		query += dictionary[uniform_int_distribution<int>(0, dictionary.size() - 1)(generator)];
	}
	return query;
}
vector<string> GenerateQueries(mt19937& generator, const vector<string>& dictionary, int query_count, int max_word_count)
{
	vector<string> queries;
	queries.reserve(query_count);
	for (int i = 0; i < query_count; ++i)
	{
		queries.push_back(GenerateQuery(generator, dictionary, max_word_count));
	}
	return queries;
}
template <typename ExecutionPolicy>
void Test(string_view mark, const SearchServer& search_server, const vector<string>& queries, ExecutionPolicy&& policy)
{
	LOG_DURATION(mark);
	double total_relevance = 0;
	for (const string_view query : queries)
	{
		for (const auto& document : search_server.FindTopDocuments(policy, query))
		{
			total_relevance += document.relevance;
		}
	}
	cout << total_relevance << endl;
}
#define TEST(policy) Test(#policy, search_server, queries, execution::policy)



int main()
{
	setlocale(LC_ALL, "Russian");
	/*
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

	std::cout << "//////////////////////////////////////////////////////////////"s << '\n';

	{
		SearchServer search_server("and with"s);
		int id = 0;
		for (
			const string& text : {
				"funny pet and nasty rat"s,
				"funny pet with curly hair"s,
				"funny pet and not very nasty rat"s,
				"pet with rat and rat and rat"s,
				"nasty rat with curly hair"s,
			}
			)
		{
			search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, { 1, 2 });
		}
		const vector<string> queries = {
			"nasty rat -not"s,
			"not very funny nasty pet"s,
			"curly hair"s
		};
		id = 0;
		for (
			const auto& documents : ProcessQueries(search_server, queries)
			)
		{
			cout << documents.size() << " documents for query ["s << queries[id++] << "]"s << endl;
		}
		//3 documents for query[nasty rat - not]
		//5 documents for query[not very funny nasty pet]
		//2 documents for query[curly hair]
	}

	{
		SearchServer search_server("and with"s);
		int id = 0;
		for (
			const string& text : {
				"funny pet and nasty rat"s,
				"funny pet with curly hair"s,
				"funny pet and not very nasty rat"s,
				"pet with rat and rat and rat"s,
				"nasty rat with curly hair"s,
			}
			)
		{
			search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, { 1, 2 });
		}
		const vector<string> queries = {
			"nasty rat -not"s,
			"not very funny nasty pet"s,
			"curly hair"s
		};
		for (const Document& document : ProcessQueriesJoined(search_server, queries))
		{
			cout << "Document "s << document.id << " matched with relevance "s << document.relevance << endl;
		}
		//Document 1 matched with relevance 0.183492
		//Document 5 matched with relevance 0.183492
		//Document 4 matched with relevance 0.167358
		//Document 3 matched with relevance 0.743945
		//Document 1 matched with relevance 0.311199
		//Document 2 matched with relevance 0.183492
		//Document 5 matched with relevance 0.127706
		//Document 4 matched with relevance 0.0557859
		//Document 2 matched with relevance 0.458145
		//Document 5 matched with relevance 0.458145
	}

	ParallelSearchBenchmark();
	//ParallelJoinedSearchBenchmark();

	//{
	//	SearchServer search_server("and with"s);

	//	int id = 0;
	//	for (
	//		const string& text : {
	//			"funny pet and nasty rat"s,
	//			"funny pet with curly hair"s,
	//			"funny pet and not very nasty rat"s,
	//			"pet with rat and rat and rat"s,
	//			"nasty rat with curly hair"s,
	//		}
	//		)
	//	{
	//		search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, { 1, 2 });
	//	}

	//	const string query = "curly and funny"s;

	//	auto report = [&search_server, &query]
	//	{
	//		cout << search_server.GetDocumentCount() << " documents total, "s
	//			<< search_server.FindTopDocuments(query).size() << " documents for query ["s << query << "]"s << endl;
	//	};

	//	report();
	//	// однопоточная версия
	//	search_server.RemoveDocument(5);
	//	report();
	//	// однопоточная версия
	//	search_server.RemoveDocument(std::execution::seq, 1);
	//	report();
	//	// многопоточная версия
	//	search_server.RemoveDocument(std::execution::par, 2);
	//	report();
	//}

	//{
	//	mt19937 generator;

	//	const auto dictionary = GenerateDictionaryRemoveBenchmark(generator, 10'000, 25);
	//	const auto documents = GenerateQueriesRemoveBenchmark(generator, dictionary, 10'000, 100);
	//	// my sequential method
	//	{
	//		SearchServer search_server(dictionary[0]);
	//		for (size_t i = 0; i < documents.size(); ++i)
	//		{
	//			search_server.AddDocument(i, documents[i], DocumentStatus::ACTUAL, { 1, 2, 3 });
	//		}

	//		REMOVE_BENCHMARK_SEQ(my seqentual method);
	//	}
	//	// method with passed sequential policy
	//	{
	//		SearchServer search_server(dictionary[0]);
	//		for (size_t i = 0; i < documents.size(); ++i)
	//		{
	//			search_server.AddDocument(i, documents[i], DocumentStatus::ACTUAL, { 1, 2, 3 });
	//		}

	//		REMOVE_BENCHMARK(seq);
	//	}
	//	// method with passed parallel policy
	//	{
	//		SearchServer search_server(dictionary[0]);
	//		for (size_t i = 0; i < documents.size(); ++i)
	//		{
	//			search_server.AddDocument(i, documents[i], DocumentStatus::ACTUAL, { 1, 2, 3 });
	//		}

	//		REMOVE_BENCHMARK(par);
	//	}
	//}

	{
		SearchServer search_server("and with"s);

		int id = 0;
		for (
			const string& text : {
				"funny pet and nasty rat"s,
				"funny pet with curly hair"s,
				"funny pet and not very nasty rat"s,
				"pet with rat and rat and rat"s,
				"nasty rat with curly hair"s,
			}
			)
		{
			search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, { 1, 2 });
		}

		const string query = "curly and funny -not"s;

		{
			const auto [words, status] = search_server.MatchDocument(query, 1);
			cout << words.size() << " words for document 1"s << endl;
			// 1 words for document 1
		}

		{
			const auto [words, status] = search_server.MatchDocument(std::execution::par, query, 2);
			cout << words.size() << " words for document 2"s << endl;
			// 2 words for document 2
		}

		{
			const auto [words, status] = search_server.MatchDocument(std::execution::par, query, 3);
			cout << words.size() << " words for document 3"s << endl;
			// 0 words for document 3
		}
	}
	TestMatchingDocumentWithPolicy();
	
	*/

	{
		SearchServer search_server("and with"s);
		int id = 0;
		for (
			const string& text : {
				"white cat and yellow hat"s,
				"curly cat curly tail"s,
				"nasty dog with big eyes"s,
				"nasty pigeon john"s,
			}
			)
		{
			search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, { 1, 2 });
		}
		cout << "ACTUAL by default:"s << endl;
		// последовательная версия
		for (const Document& document : search_server.FindTopDocuments("curly nasty cat"s))
		{
			PrintDocument(document);
		}
		cout << "BANNED:"s << endl;
		// последовательная версия
		for (const Document& document : search_server.FindTopDocuments(execution::seq, "curly nasty cat"s, DocumentStatus::BANNED))
		{
			PrintDocument(document);
		}
		cout << "Even ids:"s << endl;
		// параллельная версия
		for (const Document& document : search_server.FindTopDocuments(execution::par, "curly nasty cat"s, [](int document_id, DocumentStatus status, int rating)
			{
				return document_id % 2 == 0;
			}))
		{
			PrintDocument(document);
		}
	}
	
	mt19937 generator;
	const auto dictionary = GenerateDictionary(generator, 1000, 10);
	const auto documents = GenerateQueries(generator, dictionary, 10'000, 70);
	SearchServer search_server(dictionary[0]);
	for (size_t i = 0; i < documents.size(); ++i)
	{
		search_server.AddDocument(i, documents[i], DocumentStatus::ACTUAL, { 1, 2, 3 });
	}
	const auto queries = GenerateQueries(generator, dictionary, 100, 70);
	TEST(seq);
	TEST(par);

	return 0;
}