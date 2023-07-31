#pragma once
#include "search_server.h"

#include <execution>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "log_duration.h"

#define MATCH_DOC_TEST(policy) MatchDocumentWithPolicyTest(#policy, search_server, query, execution::policy)

using namespace std;

string GenerateWordMatchingBenchmark(mt19937& generator, int max_length)
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

vector<string> GenerateDictionaryMatchingBenchmark(mt19937& generator, int word_count, int max_length)
{
    vector<string> words;
    words.reserve(word_count);
    for (int i = 0; i < word_count; ++i)
    {
        words.push_back(GenerateWordMatchingBenchmark(generator, max_length));
    }
    sort(words.begin(), words.end());
    words.erase(unique(words.begin(), words.end()), words.end());
    return words;
}

string GenerateQueryMatchingBenchmark(mt19937& generator, const vector<string>& dictionary, int word_count, double minus_prob = 0)
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

vector<string> GenerateQueriesMatchingBenchmark(mt19937& generator, const vector<string>& dictionary, int query_count, int max_word_count)
{
    vector<string> queries;
    queries.reserve(query_count);
    for (int i = 0; i < query_count; ++i)
    {
        queries.push_back(GenerateQueryMatchingBenchmark(generator, dictionary, max_word_count));
    }
    return queries;
}

template <typename ExecutionPolicy>
void MatchDocumentWithPolicyTest(string_view mark, SearchServer search_server, const string& query, ExecutionPolicy&& policy)
{
    LOG_DURATION(mark);
    const int document_count = search_server.GetDocumentCount();
    int word_count = 0;
    for (int id = 0; id < document_count; ++id)
    {
        const auto [words, status] = search_server.MatchDocument(policy, query, id);
        word_count += words.size();
    }
    cout << word_count << endl;
}

void TestMatchingDocumentWithPolicy()
{
    mt19937 generator;

    const auto dictionary = GenerateDictionaryMatchingBenchmark(generator, 1000, 10);
    const auto documents = GenerateQueriesMatchingBenchmark(generator, dictionary, 10'000, 70);

    const string query = GenerateQueryMatchingBenchmark(generator, dictionary, 500, 0.1);

    SearchServer search_server(dictionary[0]);
    for (size_t i = 0; i < documents.size(); ++i)
    {
        search_server.AddDocument(i, documents[i], DocumentStatus::ACTUAL, { 1, 2, 3 });
    }

    MATCH_DOC_TEST(seq);
    MATCH_DOC_TEST(par);
}