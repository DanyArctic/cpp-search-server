#include <algorithm>
#include <iostream>
#include <utility>
#include <cassert>
#include <numeric>
#include <vector>
#include <string>
#include <cmath>
#include <map>
#include <set>

#define RUN_TEST(func) RunTestImpl((func), #func)
#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)
#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))
#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)
#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
constexpr double EPSILON = 1e-6;

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
	for (const char c : text)
	{
		if (c == ' ')
		{
			if (!word.empty())
			{
				words.push_back(word);
				word.clear();
			}
		}
		else
		{
			word += c;
		}
	}
	if (!word.empty())
	{
		words.push_back(word);
	}
	return words;
}

struct Document
{
	int id = 0;
	double relevance = 0.0;
	int rating = 0;
};

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
	void SetStopWords(const string& text)
	{
		for (const string& word : SplitIntoWords(text))
		{
			stop_words_.insert(word);
		}
	}

	void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings)
	{
		const vector<string> words = SplitIntoWordsNoStop(document);
		const double inv_word_count = 1.0 / words.size();
		for (const string& word : words)
		{
			word_to_document_freqs_[word][document_id] += inv_word_count;
		}
		documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
	}

	vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus requested_status) const
	{
		return FindTopDocuments(raw_query, [requested_status]([[maybe_unused]] int document_id, DocumentStatus current_status, [[maybe_unused]] int rating) { return current_status == requested_status; });
	}

	vector<Document> FindTopDocuments(const string& raw_query) const
	{
		return FindTopDocuments(raw_query, []([[maybe_unused]] int document_id, DocumentStatus status, [[maybe_unused]] int rating) { return status == DocumentStatus::ACTUAL; });
	}

	template <typename DocumentPredicate>
	vector<Document> FindTopDocuments(const string& raw_query, DocumentPredicate document_predicate) const
	{
		const Query query = ParseQuery(raw_query);
		auto matched_documents = FindAllDocuments(query, document_predicate);

		sort(matched_documents.begin(), matched_documents.end(),
			[](const Document& lhs, const Document& rhs)
			{
				if (abs(lhs.relevance - rhs.relevance) < EPSILON)
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

	int GetDocumentCount() const
	{
		return documents_.size();
	}

	tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const
	{
		const Query query = ParseQuery(raw_query);
		vector<string> matched_words;
		for (const string& word : query.plus_words)
		{
			if (word_to_document_freqs_.count(word) == 0)
			{
				continue;
			}
			if (word_to_document_freqs_.at(word).count(document_id))
			{
				matched_words.push_back(word);
			}
		}
		for (const string& word : query.minus_words)
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
		return { matched_words, documents_.at(document_id).status };
	}

private:
	struct DocumentData
	{
		int rating = 0;
		DocumentStatus status;
	};

	set<string> stop_words_;
	map<string, map<int, double>> word_to_document_freqs_;
	map<int, DocumentData> documents_;

	bool IsStopWord(const string& word) const
	{
		return stop_words_.count(word) > 0;
	}

	vector<string> SplitIntoWordsNoStop(const string& text) const
	{
		vector<string> words;
		for (const string& word : SplitIntoWords(text))
		{
			if (!IsStopWord(word))
			{
				words.push_back(word);
			}
		}
		return words;
	}

	static int ComputeAverageRating(const vector<int>& ratings)
	{
		if (ratings.empty())
		{
			return 0;
		}
		int rating_sum = accumulate(ratings.begin(), ratings.end(), 0);
		return rating_sum / static_cast<int>(ratings.size());
	}

	struct QueryWord
	{
		string data;
		bool is_minus;
		bool is_stop;
	};

	QueryWord ParseQueryWord(string text) const
	{
		bool is_minus = false;
		// Word shouldn't be empty
		if (text[0] == '-')
		{
			is_minus = true;
			text = text.substr(1);
		}
		return { text, is_minus, IsStopWord(text) };
	}

	struct Query
	{
		set<string> plus_words;
		set<string> minus_words;
	};

	Query ParseQuery(const string& text) const
	{
		Query query;
		for (const string& word : SplitIntoWords(text))
		{
			const QueryWord query_word = ParseQueryWord(word);
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

	// Existence required
	double ComputeWordInverseDocumentFreq(const string& word) const
	{
		return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
	}

	template <typename KeyMapper>
	vector<Document> FindAllDocuments(const Query& query, KeyMapper key_mapper) const
	{
		map<int, double> document_to_relevance;

		for (const string& word : query.plus_words)
		{
			if (word_to_document_freqs_.count(word) == 0)
			{
				continue;
			}
			const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
			for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word))
			{
				auto const& current_doc = documents_.at(document_id);
				if (key_mapper(document_id, current_doc.status, current_doc.rating))
				{
					document_to_relevance[document_id] += term_freq * inverse_document_freq;
				}
			}
		}

		for (const string& word : query.minus_words)
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

		vector<Document> matched_documents;
		for (const auto [document_id, relevance] : document_to_relevance)
		{
			matched_documents.push_back(
				{ document_id, relevance, documents_.at(document_id).rating });
		}
		return matched_documents;
	}
};

void PrintDocument(const Document& document)
{
	cout << "{ "s
		<< "document_id = "s << document.id << ", "s
		<< "relevance = "s << document.relevance << ", "s
		<< "rating = "s << document.rating
		<< " }"s << endl;
}

template <typename FUNC>
void RunTestImpl(FUNC test_function, const string& func_name)
{
	test_function();
	cerr << func_name << " OK"s << endl;
}

void AssertImpl(bool value, const string& expr_str, const string& file, const string& func, unsigned line, const string& hint)
{
	if (!value)
	{
		cerr << file << "("s << line << "): "s << func << ": "s;
		cerr << "ASSERT("s << expr_str << ") failed."s;
		if (!hint.empty())
		{
			cerr << " Hint: "s << hint;
		}
		cerr << endl;
		abort();
	}
}

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file, const string& func, unsigned line, const string& hint)
{
	if (t != u)
	{
		cerr << boolalpha;
		cerr << file << "("s << line << "): "s << func << ": "s;
		cerr << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
		cerr << t << " != "s << u << "."s;
		if (!hint.empty())
		{
			cerr << " Hint: "s << hint;
		}
		cerr << endl;
		abort();
	}
}

// -------- Begin of unit tests of search system ----------

void TestExcludeStopWordsFromAddedDocumentContent()
{
	// The test checks that the search engine excludes stop words when adding documents
	const int doc_id = 42;
	const string content = "cat in the city"s;
	const vector<int> ratings = { 1, 2, 3 };
	// First, we make sure that the search for a word that is not included in the list of stop words,
	// finds the desired document
	{
		SearchServer server;
		server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
		const auto found_docs = server.FindTopDocuments("in"s);
		ASSERT_EQUAL(found_docs.size(), 1u);
		const Document& doc0 = found_docs[0];
		ASSERT_EQUAL(doc0.id, doc_id);
	}

	// Then we make sure that the search for the same word included in the list of stop words
	// returns an empty result
	{
		SearchServer server;
		server.SetStopWords("in the"s);
		server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
		ASSERT(server.FindTopDocuments("in"s).empty());
	}
}

void TestAddDocument()
{
	const int doc_id = 44;
	const string content = "white cat with fancy white collar"s;
	const vector<int> ratings = { 7, 2, 7 };

	{
		SearchServer server;
		server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
		const auto found_docs = server.FindTopDocuments("white"s);
		ASSERT_EQUAL(found_docs.size(), 1);
		const Document& doc0 = found_docs[0];
		ASSERT_EQUAL(doc0.id, doc_id);
	}

	{
		SearchServer server;
		server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
		const auto found_docs = server.FindTopDocuments("dog"s);
		ASSERT_EQUAL_HINT(found_docs.size(), 0, "This document should not exist."s);
	}
}

SearchServer AddFewDocsForTests()
{
	SearchServer search_server;
	search_server.SetStopWords("and in on with"s);
	search_server.AddDocument(0, "white cat with fancy white collar"s, DocumentStatus::ACTUAL, { 8, -3 });
	search_server.AddDocument(1, "fluffy cat fluffy tail tufts on the ears"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
	search_server.AddDocument(2, "well-groomed dog expressive eyes"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
	search_server.AddDocument(3, "well-groomed starling evgeny"s, DocumentStatus::ACTUAL, { 9 });
	search_server.AddDocument(4, "cute cat with brown eyes"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
	return search_server;
}

void TestExcludeDocumentsWithMinusWordsFromSearchResults()
{
	SearchServer test_serv = AddFewDocsForTests();
	{
		const auto found_docs = test_serv.FindTopDocuments("well-groomed dog"s);
		ASSERT_EQUAL(found_docs.size(), static_cast<size_t>(2));
		const Document& doc0 = found_docs[0];
		const Document& doc1 = found_docs[1];
		ASSERT_EQUAL(doc0.id, 2u);
		ASSERT_EQUAL(doc1.id, 3u);
	}
	const auto found_docs = test_serv.FindTopDocuments("well-groomed -dog"s);
	ASSERT_EQUAL_HINT(found_docs.size(), static_cast<size_t>(1), "Now size should be 1."s);

	const Document& doc0 = found_docs[0];
	ASSERT_EQUAL_HINT(doc0.id, 3u, "It should be only document number 3."s);
}

void TestMatchDocuments()
{
	SearchServer test_serv = AddFewDocsForTests();
	{
		auto [words, status] = test_serv.MatchDocument("brown cat"s, 4);
		ASSERT_EQUAL(words.size(), static_cast<int>(2));
		ASSERT_EQUAL(words[0], "brown"s);
		ASSERT_EQUAL(words[1], "cat"s);
	}
	{
		auto [words, status] = test_serv.MatchDocument("dog -eyes"s, 2);
		ASSERT_EQUAL(words.size(), static_cast<size_t>(0));
	}
}

void TestFoundDocsSortInDescendingOrderOfRelevance()
{
	SearchServer test_serv = AddFewDocsForTests();

	const auto found_docs = test_serv.FindTopDocuments("yellow eyed cat"s);

	ASSERT(found_docs[0].relevance > found_docs[1].relevance && found_docs[1].relevance > found_docs[2].relevance);
}

void TestRatingCalc()
{
	vector<int> rating_one = { 8, -3 };
	vector<int> rating_two = { 7, 2, 7 };
	vector<int> rating_three = { 5, -12, 2, 1 };

	SearchServer search_server;
	search_server.SetStopWords("and in on with"s);
	search_server.AddDocument(0, "white cat with fancy white collar"s, DocumentStatus::ACTUAL, { rating_one });
	search_server.AddDocument(1, "fluffy cat fluffy tail tufts on the ears"s, DocumentStatus::ACTUAL, { rating_two });
	search_server.AddDocument(2, "well-groomed cat expressive eyes"s, DocumentStatus::ACTUAL, { rating_three });
	const auto found_docs = search_server.FindTopDocuments("yellow eyed cat"s);

	ASSERT(found_docs[0].rating == (7 + 2 + 7) / 3 && found_docs[1].rating == (8 + (-3)) / 2 && found_docs[2].rating == ((5 + (-12) + 2 + 1) / 4));
}

void TestFilterDocumentsByUsingPredicate()
{
	SearchServer search_server;
	search_server.SetStopWords("and in on with"s);
	search_server.AddDocument(0, "white cat with fancy white collar"s, DocumentStatus::ACTUAL, { 8, -3 });
	search_server.AddDocument(1, "fluffy cat fluffy tail tufts on the ears"s, DocumentStatus::IRRELEVANT, { 7, 2, 7 });
	search_server.AddDocument(2, "well-groomed cat dog expressive eyes"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
	search_server.AddDocument(3, "well-groomed cat starling evgeny"s, DocumentStatus::ACTUAL, { 9 });
	search_server.AddDocument(4, "cute cat with brown eyes"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
	search_server.AddDocument(5, "white cat with fancy white collar"s, DocumentStatus::ACTUAL, { 8, -3 });
	search_server.AddDocument(6, "fluffy cat fluffy tail tufts on the ears"s, DocumentStatus::IRRELEVANT, { 7, 2, 7 });
	search_server.AddDocument(7, "well-groomed cat dog expressive eyes"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
	search_server.AddDocument(8, "well-groomed cat starling evgeny"s, DocumentStatus::ACTUAL, { 9 });
	search_server.AddDocument(9, "cute cat with brown eyes"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
	{
		vector<Document> found_docs = search_server.FindTopDocuments("cat"s,
			[](int document_id, DocumentStatus status, int rating) { return document_id == 7; });
		ASSERT(found_docs.size() == 1 && found_docs.at(0).id == 7 && found_docs.at(0).rating == -1);
	}
	{
		vector<Document> found_docs = search_server.FindTopDocuments("cat"s,
			[](int document_id, DocumentStatus status, int rating) { return rating == -1; });
		ASSERT(found_docs.size() == 2 && found_docs.at(0).id == 2 && found_docs.at(1).id == 7 && found_docs.at(0).rating == -1 && found_docs.at(1).rating == -1);
	}
	{
		vector<Document> found_docs = search_server.FindTopDocuments("cat"s,
			[](int document_id, DocumentStatus status, int rating) { return status == DocumentStatus::IRRELEVANT; });
		ASSERT(found_docs.size() == 2 && found_docs.at(0).id == 1 && found_docs.at(1).id == 6);
	}
}

void TestSearchDocsByStatus()
{
	SearchServer search_server;
	search_server.SetStopWords("and in on with"s);
	search_server.AddDocument(0, "white cat with fancy white collar"s, DocumentStatus::ACTUAL, { 8, -3 });
	search_server.AddDocument(1, "fluffy cat fluffy tail tufts on the ears"s, DocumentStatus::IRRELEVANT, { 7, 2, 7 });
	search_server.AddDocument(2, "well-groomed dog expressive eyes"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
	search_server.AddDocument(3, "well-groomed starling evgeny"s, DocumentStatus::ACTUAL, { 9 });
	search_server.AddDocument(4, "cute cat with brown eyes"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
	const auto found_docs = search_server.FindTopDocuments("yellow eyed cat"s, DocumentStatus::IRRELEVANT);
	ASSERT_EQUAL(found_docs.size(), static_cast<size_t>(1));

	const Document& doc0 = found_docs[0];
	ASSERT_EQUAL(doc0.id, 1);
}

void TestRelevanceCalculate()
{
	const int doc_id_0 = 0;
	const string content_0 = "white cat in good mood"s;
	const vector<int> ratings_0 = { 1, 2, 3 };

	const int doc_id_1 = 1;
	const string content_1 = "cute cat with green eyes"s;
	const vector<int> ratings_1 = { 4, 5, 6 };

	const int doc_id_2 = 2;
	const string content_2 = "little cute cat brown tail"s;
	const vector<int> ratings_2 = { 7, 8, 9 };

	const string query = "brown cute cat"s;
	{
		SearchServer server;
		server.AddDocument(doc_id_0, content_0, DocumentStatus::ACTUAL, ratings_0);
		server.AddDocument(doc_id_1, content_1, DocumentStatus::ACTUAL, ratings_1);
		server.AddDocument(doc_id_2, content_2, DocumentStatus::ACTUAL, ratings_2);

		vector<Document> found_docs = server.FindTopDocuments(query);

		ASSERT(found_docs[0].id == 2 && found_docs[1].id == 1 && found_docs[2].id == 0);

		double idf_brown = log(3.0 / 1);
		double idf_cute = log(3.0 / 2);
		double idf_cat = log(3.0 / 3);

		double tf_brown_in_id0 = 0.0 / 5, tf_cute_in_id0 = 0.0 / 5, tf_cat_in_id0 = 1.0 / 5;
		double tf_brown_in_id1 = 0.0 / 5, tf_cute_in_id1 = 1.0 / 5, tf_cat_in_id1 = 1.0 / 5;
		double tf_brown_in_id2 = 1.0 / 5, tf_cute_in_id2 = 1.0 / 5, tf_cat_in_id2 = 1.0 / 5;

		double relevance_id0 = idf_brown * tf_brown_in_id0 +
			idf_cute * tf_cute_in_id0 +
			idf_cat * tf_cat_in_id0;

		double relevance_id1 = idf_brown * tf_brown_in_id1 +
			idf_cute * tf_cute_in_id1 +
			idf_cat * tf_cat_in_id1;

		double relevance_id2 = idf_brown * tf_brown_in_id2 +
			idf_cute * tf_cute_in_id2 +
			idf_cat * tf_cat_in_id2;

		ASSERT_EQUAL_HINT(relevance_id2, found_docs[0].relevance, "Manually calculated result for doc 2 should match."s);
		ASSERT_EQUAL_HINT(relevance_id1, found_docs[1].relevance, "Manually calculated result for doc 1 should match."s);
		ASSERT_EQUAL_HINT(relevance_id0, found_docs[2].relevance, "Manually calculated result for doc 0 should match."s);
	}
}

void TestSearchServer()
{
	RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
	RUN_TEST(TestAddDocument);
	RUN_TEST(TestExcludeDocumentsWithMinusWordsFromSearchResults);
	RUN_TEST(TestMatchDocuments);
	RUN_TEST(TestFoundDocsSortInDescendingOrderOfRelevance);
	RUN_TEST(TestRatingCalc);
	RUN_TEST(TestFilterDocumentsByUsingPredicate);
	RUN_TEST(TestSearchDocsByStatus);
	RUN_TEST(TestRelevanceCalculate);
}

// --------- End of search engine unit tests -----------

int main()
{
	TestSearchServer();
	// If this line appears, then all tests were successful
	cout << "Search server testing finished"s << endl;

	SearchServer search_server;
	search_server.SetStopWords("и в на"s);
	search_server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 8, -3 });
	search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
	search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
	search_server.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED, { 9 });
	cout << "ACTUAL by default:"s << endl;
	for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s))
	{
		PrintDocument(document);
	}
	cout << "BANNED:"s << endl;
	for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::BANNED))
	{
		PrintDocument(document);
	}
	cout << "Even ids:"s << endl;
	for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id, [[maybe_unused]] DocumentStatus status, [[maybe_unused]] int rating) { return document_id % 2 == 0; }))
	{
		PrintDocument(document);
	}
	return 0;
}