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
            int same_word_in_document = 1;
            if (word_to_document_freqs_.count(word) && word_to_document_freqs_.at(word).count(document_id))
            {
                same_word_in_document++;
            }
            // cout << '\n' << "added: " << word << " same_word_in_document: "s << same_word_in_document << " words.size: " << words.size() << " ID: " << document_id << " TF: " << TermFrequency(same_word_in_document, words.size()) << endl;
            word_to_document_freqs_[word][document_id] = TermFrequency(same_word_in_document, words.size());
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
        return (double)log((double)document_count_ / (double)number_of_matches);
    }

    double TermFrequency(const int query_in_document, const int total_words_in_document) const
    {
        return (double)query_in_document / (double)total_words_in_document;
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
                //cout << "word: "s << word << " num of matches: "s << number_of_matches <<  " idf: "s  << IDF << "docs: "s << document_count_ << " "s << (double)document_count_/(double)number_of_matches << endl;
                for (const auto& [id, TF] : word_to_document_freqs_.at(word))
                {
                    //cout << "IDF = "s << IDF << " "s << "TF = " << TF << endl;
                    relevance_table[id] += IDF * TF;
                }
            }
        }
        /*for (const auto& map_item : relevance_table)
           {
             cout << "print relevance table: " << endl;

             cout << map_item.first << '\t' << map_item.second << endl;
           }*/
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
    //search_server.PrintDocuments();
}