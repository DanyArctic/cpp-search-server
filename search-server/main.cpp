#include <algorithm>
#include <iostream>
#include <utility>
#include <string>
#include <vector>
#include <cmath>
#include <set>
#include <map>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine()
{
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber()
{
    int result = 0;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string& text)
{
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        }
        else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}

struct Document
{
    int id;
    double relevance;
};

class SearchServer
{
public:
    void SetStopWords(const string& text)
    {
        for (const string& word : SplitIntoWords(text))
        {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document)
    {
        const vector<string> words = SplitIntoWordsNoStop(document);
        for (const string& word : words)
        {
            double TermFrequency = 1.0 / static_cast<double>(words.size());
            word_to_document_freqs_[word][document_id] += TermFrequency;
        }
        document_count_++;
    }

    void PrintDocuments() const
    {
        for (const auto& map_item : word_to_document_freqs_)
        {
            cout << map_item.first << " ";
            for (const auto& [id, TF] : map_item.second)
            {
                std::cout << "ID: " << id << "  TF: " << TF << endl;
            }
            std::cout << std::endl;
        }
    }

    vector<Document> FindTopDocuments(const string& raw_query) const
    {
        Query query_words = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query_words);

        sort(matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs) {
                return lhs.relevance > rhs.relevance;
            });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

private:
    struct Query
    {
        set<string> plus_words;
        set<string> minus_words;
    };

    map<string, map<int, double>> word_to_document_freqs_;
    int document_count_ = 0;
    set<string> stop_words_;

    bool IsStopWord(const string& word) const
    {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const
    {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word))
            {
                words.push_back(word);
            }
        }
        return words;
    }

    Query ParseQuery(const string& text) const
    {
        Query query_words;
        for (const string& word : SplitIntoWordsNoStop(text))
        {
            if (word.at(0) == '-')
            {
                query_words.minus_words.insert(word.substr(1));
            }
            else
            {
                query_words.plus_words.insert(word);
            }
        }
        return query_words;
    }

    double InverseDocumentFrequency(const int number_of_matches) const
    {
        return log(static_cast<double>(document_count_) / static_cast<double>(number_of_matches));
    }

    vector<Document> FindAllDocuments(const Query& query_words) const
    {
        map<int, double> relevance_table;
        for (const string& word : query_words.plus_words)
        {
            if (word_to_document_freqs_.count(word) > 0)
            {
                int number_of_matches = word_to_document_freqs_.at(word).size();
                double IDF = InverseDocumentFrequency(number_of_matches);
                for (const auto& [id, TF] : word_to_document_freqs_.at(word))
                {
                    relevance_table[id] += IDF * TF;
                }
            }
        }

        vector<Document> matched_documents;
        for (const auto& map_item : relevance_table)
        {
            matched_documents.push_back({ map_item.first, map_item.second });
        }
        return matched_documents;
    }
};

SearchServer CreateSearchServer()
{
    SearchServer search_server;
    search_server.SetStopWords(ReadLine());

    const int document_count = ReadLineWithNumber();
    for (int document_id = 0; document_id < document_count; ++document_id)
    {
        search_server.AddDocument(document_id, ReadLine());
    }
    return search_server;
}

int main()
{
    const SearchServer search_server = CreateSearchServer();

    const string query = ReadLine();
    for (const auto& [document_id, relevance] : search_server.FindTopDocuments(query))
    {
        cout << "{ document_id = "s << document_id << ", "
            << "relevance = "s << relevance << " }"s << endl;
    }
}