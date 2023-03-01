#include "search_server.h"
#include <numeric>
#include <cmath>

using namespace std;

std::vector<int>::iterator SearchServer::begin()
{
	return documents_ids_count_.begin();
}

std::vector<int>::iterator SearchServer::end()
{
	return documents_ids_count_.end();
}

const std::map<std::string, double>& SearchServer::GetWordFrequencies(int document_id) const
{
	static std::map<std::string, double> empty_map;
	return document_to_word_freqs_.count(document_id) == 0 ? empty_map : document_to_word_freqs_.at(document_id);
}

void SearchServer::RemoveDocument(int document_id)
{
	documents_.erase(document_id);
	for (auto& [word, id_and_freq] : word_to_document_freqs_)
	{
		id_and_freq.erase(document_id);
	}
	std::remove(documents_ids_count_.begin(), documents_ids_count_.end(), document_id);
	words_n_ids_.erase(document_id);
}

void SearchServer::AddDocument(int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings)
{
	if (document_id < 0 || static_cast<bool>(documents_.count(document_id)))
	{
		throw invalid_argument("Wrong document ID"s);
	}
	const std::vector<std::string> words = SplitIntoWordsNoStop(document);
	std::set<std::string> words_set{ words.begin(), words.end() };
	words_n_ids_.emplace(document_id, words_set);
	const double inv_word_count = 1.0 / words.size(); // First stage of calculating TF
	for (const std::string& word : words)
	{
		word_to_document_freqs_[word][document_id] += inv_word_count; // Final calculating TF of each word
		document_to_word_freqs_[document_id][word] += word_to_document_freqs_[word][document_id];
	}
	documents_.emplace(document_id, SearchServer::DocumentData{ ComputeAverageRating(ratings), status });
	documents_ids_count_.push_back(document_id); // I don't find a need to use this container and associated method yet
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, DocumentStatus status) const
{
	return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating)
		{
			return document_status == status;
		});
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query) const
{
	return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const
{
	return documents_.size();
}

std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(const std::string& raw_query, int document_id) const
{
	//LOG_DURATION_STREAM("Operation time: ", std::cout);
	const SearchServer::Query query = ParseQuery(raw_query);
	std::vector<std::string> matched_words;
	for (const std::string& word : query.plus_words)
	{
		if (word_to_document_freqs_.count(word) == 0)
		{
			continue;
		}
		if (word_to_document_freqs_.at(word).count(document_id))
		{
			matched_words.emplace_back(word);
		}
	}
	for (const std::string& word : query.minus_words)
	{
		if (word_to_document_freqs_.count(word) == 0)
		{
			continue;
		}
		if (word_to_document_freqs_.at(word).count(document_id))
		{
			matched_words.clear();
			break;
		}
	}
	return std::make_tuple(matched_words, documents_.at(document_id).status);
}

std::map<int, std::set<std::string>> &SearchServer::GetDocuments()
{
	return words_n_ids_;
}

bool SearchServer::IsValidWord(const std::string& word)
{
	// A valid word must not contain special characters
	return std::none_of(word.begin(), word.end(), [](char c)
		{
			return c >= '\0' && c < ' ';
		});
}

bool SearchServer::ContainsInvalidDashes(const std::string& word)
{
	if ((word.size() == 1u && word[0] == '-') || (word[0] == '-' && word[1] == '-'))
	{
		return true;
	}
	// A valid word must not contain double dashes and dashes without word afterwards
	return false;
}

bool SearchServer::IsStopWord(const std::string& word) const
{
	return stop_words_.count(word) > 0;
}

std::vector<std::string> SearchServer::SplitIntoWordsNoStop(const std::string& text) const
{
	std::vector<std::string> words;
	for (const std::string& word : SplitIntoWords(text))
	{
		if (!IsValidWord(word))
		{
			throw invalid_argument("Invalid symbols in document"s);
		}
		if (!IsStopWord(word))
		{
			words.emplace_back(word);
		}
	}
	return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings)
{
	if (ratings.empty())
	{
		return 0;
	}
	int rating_sum = std::accumulate(ratings.begin(), ratings.end(), 0);
	return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string text) const
{
	bool is_minus = false;
	if (ContainsInvalidDashes(text))
	{
		throw invalid_argument("Invalid query"s);
	}
	// Word shouldn't be empty
	if (text[0] == '-')
	{
		if (text.size() > 1 && text[1] != '-')
		{
			is_minus = true;
			text = text.substr(1);
		}
	}
	if (!IsValidWord(text))
	{
		throw invalid_argument("Invalid query"s);
	}
	return { text, is_minus, IsStopWord(text) }; // Write all data to the QueryWord structure
}

SearchServer::Query SearchServer::ParseQuery(const std::string& text) const
{
	SearchServer::Query query;
	for (const std::string& word : SplitIntoWords(text))
	{
		const SearchServer::QueryWord query_word = ParseQueryWord(word);
		if (!query_word.is_stop)
		{
			if (query_word.is_minus)
			{
				query.minus_words.insert(query_word.data);
			}
			else
			{
				query.plus_words.insert(query_word.data);
			}
		}
	}
	return query;
}

double SearchServer::ComputeWordInverseDocumentFreq(const std::string& word) const
{
	return std::log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}

void AddDocument(SearchServer& search_server, int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings)
{
	search_server.AddDocument(document_id, document, status, ratings);
}