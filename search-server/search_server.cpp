#include "search_server.h"
#include <numeric>
#include <cmath>

using namespace std;

std::set<int>::iterator SearchServer::begin()
{
	return documents_ids_.begin();
}

std::set<int>::iterator SearchServer::end()
{
	return documents_ids_.end();
}

const std::map<std::string_view, double>& SearchServer::GetWordFrequencies(int document_id) const
{
	static std::map<std::string_view, double> empty_map;
	return document_to_word_freqs_.count(document_id) == 0 ? empty_map : document_to_word_freqs_.at(document_id);
}

void SearchServer::RemoveDocument(int document_id)
{
	if (!document_to_word_freqs_.count(document_id))
	{
		throw std::invalid_argument("Invalid ID for deleting");
	}
	std::map<std::string_view, double>& word_n_freqs = document_to_word_freqs_.at(document_id);

	for (auto& [word, id_and_freq] : word_n_freqs)
	{
		word_to_document_freqs_.at(word).erase(document_id);
	}

	document_to_word_freqs_.erase(document_id);
	documents_.erase(document_id);
	documents_ids_.erase(document_id);
}

void SearchServer::AddDocument(int document_id, std::string_view document, DocumentStatus status, const std::vector<int>& ratings)
{
	if (document_id < 0 || static_cast<bool>(documents_.count(document_id)))
	{
		throw invalid_argument("Wrong document ID"s);
	}

	auto words = SplitIntoWordsNoStop(document);
	const double inv_word_count = 1.0 / words.size(); // First stage of calculating TF
	const std::string document_string{ document };
	documents_.emplace(document_id, SearchServer::DocumentData{ ComputeAverageRating(ratings), status, document_string });
	words = SplitIntoWordsNoStop(documents_.at(document_id).document_text_);
	for (const std::string_view word : words)
	{
		word_to_document_freqs_[word][document_id] += inv_word_count; // Final calculating TF of each word
		document_to_word_freqs_[document_id][word] += word_to_document_freqs_[word][document_id];
	}
	
	documents_ids_.insert(document_id);
}

std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query, DocumentStatus status) const
{
	return FindTopDocuments(std::execution::seq, raw_query, [&status](int document_id, DocumentStatus document_status, int rating)
		{
			return document_status == status;
		});
}

std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query) const
{
	return FindTopDocuments(std::execution::seq, raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const
{
	return documents_.size();
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(std::string_view raw_query, int document_id) const
{
	//LOG_DURATION_STREAM("Operation time: ", std::cout);
	if (document_id < 0)
	{
		throw std::out_of_range("Document ID is negative"s);
	}
	if (documents_.count(document_id) == 0)
	{
		throw std::invalid_argument("Document ID is missing"s);
	}
	const SearchServer::Query query = ParseQuery(raw_query, false); //bool with_execution_policy
	std::vector<std::string_view> matched_words;
	for (const std::string_view word : query.minus_words)
	{
		if (word_to_document_freqs_.count(word) == 0)
		{
			continue;
		}
		if (word_to_document_freqs_.at(word).count(document_id))
		{
			return { {}, documents_.at(document_id).status };
		}
	}
	for (const std::string_view word : query.plus_words)
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
	return { matched_words, documents_.at(document_id).status };
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(std::execution::sequenced_policy policy, std::string_view raw_query, int document_id) const
{
	return MatchDocument(raw_query, document_id);
}
std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(std::execution::parallel_policy policy, std::string_view raw_query, int document_id) const
{
	if (document_id < 0)
	{
		throw std::out_of_range("Document ID is negative"s);
	}
	if (documents_.count(document_id) == 0)
	{
		throw std::invalid_argument("Document ID is missing"s);
	}
	if (!IsValidWord(raw_query))
	{
		throw std::invalid_argument("Invalid query"s);
	}

	const SearchServer::Query& query = ParseQuery(raw_query, true); //bool with_execution_policy
	const std::map<std::string_view, double>& word_and_frequency = document_to_word_freqs_.at(document_id);

	if (std::any_of(policy, query.minus_words.begin(), query.minus_words.end(), [&word_and_frequency](const std::string_view minus_word)
		{
			return word_and_frequency.count(minus_word) > 0;
		}))
	{
		return { {}, documents_.at(document_id).status };
	}
	std::vector<std::string_view> matched_words;
	matched_words.reserve(query.plus_words.size());
	auto last_copied_it = std::copy_if(policy, query.plus_words.begin(), query.plus_words.end(), matched_words.begin(), [&word_and_frequency](const std::string_view plus_word)
		{
			return word_and_frequency.count(plus_word) > 0;
		});
	matched_words.erase(last_copied_it, matched_words.end());
	std::sort(policy, matched_words.begin(), matched_words.end());
	std::vector<std::string_view>::iterator it = std::unique(matched_words.begin(), matched_words.end());
	matched_words.erase(it, matched_words.end());
	return { matched_words, documents_.at(document_id).status };
}

bool SearchServer::IsValidWord(std::string_view word)
{
	// A valid word must not contain special characters
	return std::none_of(word.begin(), word.end(), [](char c)
		{
			return c >= '\0' && c < ' ';
		});
}

bool SearchServer::ContainsInvalidDashes(std::string_view word)
{
	if ((word.size() == 1u && word[0] == '-') || (word[0] == '-' && word[1] == '-'))
	{
		return true;
	}
	// A valid word must not contain double dashes and dashes without word afterwards
	return false;
}

bool SearchServer::IsStopWord(std::string_view word) const
{
	return stop_words_.count(word) > 0;
}

std::vector<std::string_view> SearchServer::SplitIntoWordsNoStop(std::string_view text) const
{
	std::vector<std::string_view> words;
	for (const std::string_view word : SplitIntoWords(text))
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

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string_view text) const
{
	bool is_minus = false;
	if (ContainsInvalidDashes(text))
	{
		throw std::invalid_argument("Invalid query"s);
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
		throw std::invalid_argument("Invalid query"s);
	}
	return { text, is_minus, IsStopWord(text) }; // Write all data to the QueryWord structure
}

SearchServer::Query SearchServer::ParseQuery(std::string_view text, bool with_execution_policy) const
{
	SearchServer::Query query;
	for (const std::string_view word : SplitIntoWords(text))
	{
		const SearchServer::QueryWord query_word = ParseQueryWord(word);
		if (!query_word.is_stop)
		{
			if (query_word.is_minus)
			{
				query.minus_words.push_back(query_word.data);
			}
			else
			{
				query.plus_words.push_back(query_word.data);
			}
		}
	}
	if (!with_execution_policy)
	{
		std::sort(query.minus_words.begin(), query.minus_words.end());
		std::sort(query.plus_words.begin(), query.plus_words.end());

		auto last_minus = std::unique(query.minus_words.begin(), query.minus_words.end());
		auto last_plus = std::unique(query.plus_words.begin(), query.plus_words.end());

		size_t newSize = last_minus - query.minus_words.begin();
		query.minus_words.resize(newSize);

		newSize = last_plus - query.plus_words.begin();
		query.plus_words.resize(newSize);
	}
	return query;
}

double SearchServer::ComputeWordInverseDocumentFreq(std::string_view word) const
{
	return std::log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}

void AddDocument(SearchServer& search_server, int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings)
{
	search_server.AddDocument(document_id, document, status, ratings);
}