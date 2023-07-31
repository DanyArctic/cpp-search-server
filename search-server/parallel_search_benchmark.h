#pragma once
#include "search_server.h"

#include <execution>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "process_queries.h"
#include "log_duration.h"

#define PAR_SEARCH_BENCHMARK(processor) ParallelSearchQueries(#processor, processor, search_server, queries)

using namespace std;

template <typename QueriesProcessor>
void ParallelSearchQueries(std::string_view mark, QueriesProcessor processor, const SearchServer& search_server, const std::vector<std::string>& queries)
{
	LOG_DURATION(mark);
	const auto documents_lists = processor(search_server, queries);
}

std::string GenerateWordParallelSearch(mt19937& generator, int max_length)
{
	const int length = std::uniform_int_distribution(1, max_length)(generator);
	std::string word;
	word.reserve(length);
	for (int i = 0; i < length; ++i)
	{
		word.push_back(std::uniform_int_distribution(static_cast<int>('a'), static_cast<int>('z'))(generator));
	}
	return word;
}

std::vector<std::string> GenerateDictionaryParallelSearch(std::mt19937& generator, int word_count, int max_length)
{
	std::vector<std::string> words;
	words.reserve(word_count);
	for (int i = 0; i < word_count; ++i)
	{
		words.push_back(GenerateWordParallelSearch(generator, max_length));
	}
	sort(words.begin(), words.end());
	words.erase(std::unique(words.begin(), words.end()), words.end());
	return words;
}

string GenerateQueryParallelSearch(std::mt19937& generator, const vector<string>& dictionary, int max_word_count)
{
	const int word_count = std::uniform_int_distribution(1, max_word_count)(generator);
	std::string query;
	for (int i = 0; i < word_count; ++i)
	{
		if (!query.empty())
		{
			query.push_back(' ');
		}
		query += dictionary[std::uniform_int_distribution<int>(0, dictionary.size() - 1)(generator)];
	}
	return query;
}

std::vector<std::string> GenerateQueriesParallelSearch(std::mt19937& generator, const std::vector<std::string>& dictionary, int query_count, int max_word_count)
{
	std::vector<std::string> queries;
	queries.reserve(query_count);
	for (int i = 0; i < query_count; ++i)
	{
		queries.push_back(GenerateQueryParallelSearch(generator, dictionary, max_word_count));
	}
	return queries;
}

void ParallelSearchBenchmark()
{
	std::mt19937 generator;
	const auto dictionary = GenerateDictionaryParallelSearch(generator, 2'000, 25);
	const auto documents = GenerateQueriesParallelSearch(generator, dictionary, 20'000, 10);
	SearchServer search_server(dictionary[0]);
	for (size_t i = 0; i < documents.size(); ++i)
	{
		search_server.AddDocument(i, documents[i], DocumentStatus::ACTUAL, { 1, 2, 3 });
	}
	const auto queries = GenerateQueriesParallelSearch(generator, dictionary, 2'000, 7);
	PAR_SEARCH_BENCHMARK(ProcessQueries);
}

void ParallelJoinedSearchBenchmark()
{
	std::mt19937 generator;
	const auto dictionary = GenerateDictionaryParallelSearch(generator, 2'000, 25);
	const auto documents = GenerateQueriesParallelSearch(generator, dictionary, 20'000, 10);
	SearchServer search_server(dictionary[0]);
	for (size_t i = 0; i < documents.size(); ++i)
	{
		search_server.AddDocument(i, documents[i], DocumentStatus::ACTUAL, { 1, 2, 3 });
	}
	const auto queries = GenerateQueriesParallelSearch(generator, dictionary, 2'000, 7);
	PAR_SEARCH_BENCHMARK(ProcessQueriesJoined);
}