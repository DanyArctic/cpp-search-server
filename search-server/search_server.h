#pragma once
#include "string_processing.h"
#include "document.h"
#include <algorithm>
#include <vector>
#include <string>
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
	explicit SearchServer(const StringContainer& stop_words)
		: stop_words_(MakeUniqueNonEmptyStrings(stop_words)) // Extract non-empty stop words
	{
		if (!all_of(stop_words.begin(), stop_words.end(), IsValidWord))
		{
			throw std::invalid_argument("Invalid character in stop words");
		}
	}

	explicit SearchServer(const std::string& stop_words_text)
		: SearchServer(SplitIntoWords(stop_words_text))  // Invoke delegating constructor from string container
	{}

	int GetDocumentId(int index) const;

	void AddDocument(int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings);

	template <typename DocumentPredicate>
	std::vector<Document> FindTopDocuments(const std::string& raw_query, DocumentPredicate document_predicate) const
	{
		const Query query = ParseQuery(raw_query);
		std::vector matched_documents = FindAllDocuments(query, document_predicate);

		std::sort(matched_documents.begin(), matched_documents.end(),
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

	std::vector<Document> FindTopDocuments(const std::string& raw_query, DocumentStatus status) const;

	std::vector<Document> FindTopDocuments(const std::string& raw_query) const;

	int GetDocumentCount() const;

	std::tuple<std::vector<std::string>, DocumentStatus> MatchDocument(const std::string& raw_query, int document_id) const;

private:
	struct DocumentData
	{
		int rating = 0;
		DocumentStatus status;
	};
	const std::set<std::string> stop_words_;
	std::map<std::string, std::map<int, double>> word_to_document_freqs_; // Table of [words]: IDs and Term Frequencies
	std::map<int, DocumentData> documents_;
	std::vector<int> documents_ids_count_;

	static bool IsValidWord(const std::string& word);

	static bool ContainsInvalidDashes(const std::string& word);

	bool IsStopWord(const std::string& word) const;

	std::vector<std::string> SplitIntoWordsNoStop(const std::string& text) const;

	static int ComputeAverageRating(const std::vector<int>& ratings);

	struct QueryWord
	{
		std::string data;
		bool is_minus;
		bool is_stop;
	};

	QueryWord ParseQueryWord(std::string text) const;

	struct Query
	{
		std::set<std::string> plus_words;
		std::set<std::string> minus_words;
	};

	Query ParseQuery(const std::string& text) const;

	double ComputeWordInverseDocumentFreq(const std::string& word) const;

	template <typename DocumentPredicate>
	std::vector<Document> FindAllDocuments(const Query& query,
		DocumentPredicate document_predicate) const
	{
		std::map<int, double> document_to_relevance;
		for (const std::string& word : query.plus_words)
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

		for (const std::string& word : query.minus_words)
		{
			if (word_to_document_freqs_.count(word) == 0)
			{
				continue;
			}
			for (const auto [document_id, _] : word_to_document_freqs_.at(word))
			{
				document_to_relevance.erase(document_id);
			}
		}

		std::vector<Document> matched_documents;
		for (const auto [document_id, relevance] : document_to_relevance)
		{
			matched_documents.push_back(
				{ document_id, relevance, documents_.at(document_id).rating });
		}
		return matched_documents;
	}
};