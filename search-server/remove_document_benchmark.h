#pragma once
#include "search_server.h"

#include <execution>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "log_duration.h"

#define REMOVE_BENCHMARK(mode) TestRemoveBenchmark(#mode, search_server, std::execution::mode)
#define REMOVE_BENCHMARK_SEQ(mode) TestRemoveBenchmarkSeq(#mode, search_server)

using namespace std;

string GenerateWordRemoveBenchmark(mt19937& generator, int max_length)
{
    const int length = uniform_int_distribution(1, max_length)(generator);
    string word;
    word.reserve(length);
    for (int i = 0; i < length; ++i)
    {
        word.push_back(char(uniform_int_distribution<int>(97, 122)(generator)));
    }
    return word;
}

vector<string> GenerateDictionaryRemoveBenchmark(mt19937& generator, int word_count, int max_length)
{
    vector<string> words;
    words.reserve(word_count);
    for (int i = 0; i < word_count; ++i)
    {
        words.push_back(GenerateWordRemoveBenchmark(generator, max_length));
    }
    sort(words.begin(), words.end());
    words.erase(unique(words.begin(), words.end()), words.end());
    return words;
}

string GenerateQueryRemoveBenchmark(mt19937& generator, const vector<string>& dictionary, int word_count, double minus_prob = 0)
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

vector<string> GenerateQueriesRemoveBenchmark(mt19937& generator, const vector<string>& dictionary, int query_count, int max_word_count)
{
    vector<string> queries;
    queries.reserve(query_count);
    for (int i = 0; i < query_count; ++i)
    {
        queries.push_back(GenerateQueryRemoveBenchmark(generator, dictionary, max_word_count));
    }
    return queries;
}

template <typename ExecutionPolicy>
void TestRemoveBenchmark(string_view mark, SearchServer search_server, ExecutionPolicy&& policy)
{
    LOG_DURATION(mark);
    const int document_count = search_server.GetDocumentCount();
    for (int id = 0; id < document_count; ++id)
    {
        search_server.RemoveDocument(policy, id);
    }
    cout << search_server.GetDocumentCount() << endl;
}

void TestRemoveBenchmarkSeq(string_view mark, SearchServer search_server)
{
    LOG_DURATION(mark);
    const int document_count = search_server.GetDocumentCount();
    for (int id = 0; id < document_count; ++id)
    {
        search_server.RemoveDocument(id);
    }
    cout << search_server.GetDocumentCount() << endl;
}