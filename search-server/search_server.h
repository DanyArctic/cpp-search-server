#pragma once
#include "document.h"
#include "log_duration.h"
#include "concurrent_map.h"
#include "string_processing.h"
#include <string_view>
#include <algorithm>
#include <execution>
#include <vector>
#include <string>
#include <deque>
#include <map>
#include <set>

const int MAX_RESULT_DOCUMENT_COUNT = 5;
constexpr double EPSILON = 1e-6;

enum class DocumentStatus
{
	ACTUAL,
	IRRELEVANT,
	BANNED,
	REMOVED,
};

class SearchServer
{
public:
	template <typename StringContainer>
	explicit SearchServer(const StringContainer& stop_words);

	explicit SearchServer(const std::string& stop_words_text)
		: SearchServer(SplitIntoWords(stop_words_text))  // Invoke delegating constructor from string container
	{}

	explicit SearchServer(std::string_view stop_words_text)
		: SearchServer(SplitIntoWordsView(stop_words_text))
	{}

	std::set<int>::iterator begin();
	std::set<int>::iterator end();

	const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const;

	void RemoveDocument(int document_id);
	template <typename ExecutionPolicy>
	void RemoveDocument(ExecutionPolicy&& policy, int document_id);

	void AddDocument(int document_id, std::string_view document, DocumentStatus status, const std::vector<int>& ratings);
	
	template <typename DocumentPredicate, typename ExecutionPolicy>
	std::vector<Document> FindTopDocuments(ExecutionPolicy&& policy, std::string_view raw_query, DocumentPredicate document_predicate) const;
	template <typename DocumentPredicate>
	std::vector<Document> FindTopDocuments(std::string_view raw_query, DocumentPredicate document_predicate) const;
	
	template <typename ExecutionPolicy>
	std::vector<Document> FindTopDocuments(ExecutionPolicy&& policy, std::string_view raw_query, DocumentStatus status) const;
	std::vector<Document> FindTopDocuments(std::string_view raw_query, DocumentStatus status) const;

	template <typename ExecutionPolicy>
	std::vector<Document> FindTopDocuments(ExecutionPolicy&& policy, std::string_view raw_query) const;
	std::vector<Document> FindTopDocuments(std::string_view raw_query) const;

	int GetDocumentCount() const;

	std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::string_view raw_query, int document_id) const; // Returns matched words in exact document
	std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::execution::parallel_policy policy, std::string_view raw_query, int document_id) const;
	std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::execution::sequenced_policy policy, std::string_view raw_query, int document_id) const;

	std::map<int, std::set<std::string_view>>& GetDocuments();
	struct DocumentData
	{
		int rating = 0;
		DocumentStatus status;
	};
private:

	std::deque<std::string> documents_storage_;
	const std::set<std::string, std::less<>> stop_words_; // These words do not participate in the indexing of documents added by AddDocument, these words are not included in the search
	std::map<std::string_view, std::map<int, double>> word_to_document_freqs_; // Table of [words]: IDs and Term Frequencies
	std::map<int, std::map<std::string_view, double>> document_to_word_freqs_; // Table of [IDs]: words and Term Frequencies
	std::map<int, std::set<std::string_view>> ids_and_words;
	std::map<int, DocumentData> documents_;
	std::set<int> documents_ids_; // set of document IDs

	static bool IsValidWord(std::string_view word);

	bool IsStopWord(std::string_view word) const;

	static bool ContainsInvalidDashes(std::string_view word);

	std::vector<std::string_view> SplitIntoWordsNoStop(std::string_view text) const;

	static int ComputeAverageRating(const std::vector<int>& ratings);

	struct QueryWord
	{
		std::string_view data;
		bool is_minus;
		bool is_stop;
	};

	QueryWord ParseQueryWord(std::string_view text) const;

	struct Query
	{
		std::vector<std::string_view> plus_words;
		std::vector<std::string_view> minus_words; // Documents with these words will not be returned as a result of the search query
	};

	Query ParseQuery(std::string_view text) const;
	Query ParseQueryForParrallelImpl(std::string_view text) const;

	double ComputeWordInverseDocumentFreq(std::string_view word) const;

	template <typename DocumentPredicate>
	std::vector<Document> FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const;
	template <typename DocumentPredicate>
	std::vector<Document> FindAllDocuments(const std::execution::sequenced_policy, const Query& query, DocumentPredicate document_predicate) const;
	template <typename DocumentPredicate>
	std::vector<Document> FindAllDocuments(const std::execution::parallel_policy, const Query& query, DocumentPredicate document_predicate) const;
};

template <typename StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words)
	: stop_words_(MakeUniqueNonEmptyStrings(stop_words)) // Extract non-empty stop words
{
	if (!std::all_of(stop_words.begin(), stop_words.end(), IsValidWord))
	{
		throw std::invalid_argument("Invalid character in stop words");
	}
}

template<typename ExecutionPolicy>
void SearchServer::RemoveDocument(ExecutionPolicy&& policy, int document_id)
{
	if (!document_to_word_freqs_.count(document_id))
	{
		throw std::invalid_argument("Invalid ID for deleting");
	}
	const std::map<std::string_view, double>& word_n_freqs = document_to_word_freqs_.at(document_id);
	std::vector<std::string_view*> words_to_delete(word_n_freqs.size());

	std::transform(policy,
		word_n_freqs.begin(), word_n_freqs.end(),
		words_to_delete.begin(),
		[](const auto& element)
		{
			return const_cast<std::string_view*>(&element.first);
		}
	);

	std::for_each(policy,
		words_to_delete.begin(), words_to_delete.end(),
		[this, document_id](std::string_view* word)
		{
			word_to_document_freqs_.at(*word).erase(document_id);
		});

	document_to_word_freqs_.erase(document_id);
	documents_.erase(document_id);
	ids_and_words.erase(document_id);
	documents_ids_.erase(document_id);
}

template <typename DocumentPredicate, typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy&& policy, std::string_view raw_query, DocumentPredicate document_predicate) const
{
	const Query& query = ParseQuery(raw_query);
	auto matched_documents = FindAllDocuments(policy, query, document_predicate);

	std::sort(policy, matched_documents.begin(), matched_documents.end(),
		[](const Document& lhs, const Document& rhs)
		{
			if (std::abs(lhs.relevance - rhs.relevance) < EPSILON)
			{
				return lhs.rating > rhs.rating;
			}
			else
			{
				return lhs.relevance > rhs.relevance;
			}
		});
	if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT)
	{
		matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
	}
	return matched_documents;
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query, DocumentPredicate document_predicate) const
{
	return FindTopDocuments(std::execution::seq, raw_query, document_predicate);
}

template <typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy&& policy, std::string_view raw_query, DocumentStatus status) const
{
	return FindTopDocuments(policy, raw_query, [&status](int document_id, DocumentStatus document_status, int rating)
		{
			return document_status == status;
		});
}

template <typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy&& policy, std::string_view raw_query) const
{
	return FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const
{
	//LOG_DURATION_STREAM("Operation time: ", std::cout);
	std::map<int, double> document_to_relevance;
	for (const std::string_view word : query.plus_words)
	{
		if (word_to_document_freqs_.count(word) == 0)
		{
			continue;
		}
		const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
		for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word))
		{
			const auto& document_data = documents_.at(document_id);
			if (document_predicate(document_id, document_data.status, document_data.rating))
			{
				document_to_relevance[document_id] += term_freq * inverse_document_freq;
			}
		}
	}

	for (const std::string_view word : query.minus_words)
	{
		if (word_to_document_freqs_.count(word) == 0)
		{
			continue;
		}
		for (const auto [document_id, term_frequency] : word_to_document_freqs_.at(word))
		{
			document_to_relevance.erase(document_id);
		}
	}

	std::vector<Document> matched_documents;
	for (const auto [document_id, relevance] : document_to_relevance)
	{
		matched_documents.push_back({ document_id, relevance, documents_.at(document_id).rating });
	}
	return matched_documents;
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const std::execution::sequenced_policy, const Query& query, DocumentPredicate document_predicate) const
{
	return FindAllDocuments(query, document_predicate);
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const std::execution::parallel_policy, const Query& query, DocumentPredicate document_predicate) const
{
	ConcurrentMap<int, double> document_to_relevance(100);

	std::for_each(std::execution::par, query.minus_words.begin(), query.minus_words.end(), 
		[this, &document_to_relevance](const std::string_view word)
		{
			if (word_to_document_freqs_.count(word))
			{
				for (const auto [document_id, term_frequency] : word_to_document_freqs_.at(word))
				{
					document_to_relevance.Erase(document_id);
				}
			}
		});

	std::for_each(std::execution::par, query.plus_words.begin(), query.plus_words.end(),
		[this, &document_to_relevance, &document_predicate](const std::string_view word)
		{
			if (word_to_document_freqs_.count(word))
			{
				const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
				for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word))
				{
					const auto& document_data = documents_.at(document_id);
					if (document_predicate(document_id, document_data.status, document_data.rating))
					{
						document_to_relevance[document_id].ref_to_value += term_freq * inverse_document_freq;
					}
				}
			}
		});

	std::map<int, double> document_to_relevance_accumulated = document_to_relevance.BuildOrdinaryMap();
	std::vector<Document> matched_documents;
	matched_documents.reserve(document_to_relevance_accumulated.size());
	for (const auto [document_id, relevance] : document_to_relevance_accumulated)
	{
		matched_documents.push_back({ document_id, relevance, documents_.at(document_id).rating });
	}
	return matched_documents;
}

void AddDocument(SearchServer& search_server, int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings);