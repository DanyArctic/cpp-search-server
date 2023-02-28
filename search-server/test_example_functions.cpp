#include "test_example_functions.h"
#include <cmath>

void AssertImpl(bool value, const std::string& expr_str, const std::string& file, const std::string& func, unsigned line, const std::string& hint)
{
	if (!value)
	{
		std::cerr << file << "("s << line << "): "s << func << ": "s;
		std::cerr << "ASSERT("s << expr_str << ") failed."s;
		if (!hint.empty())
		{
			std::cerr << " Hint: "s << hint;
		}
		std::cerr << std::endl;
		abort();
	}
}

void TestExcludeStopWordsFromAddedDocumentContent()
{
	// The test checks that the search engine excludes stop words when adding documents
	const int doc_id = 42;
	const string content = "cat in the city"s;
	const vector<int> ratings = { 1, 2, 3 };
	// First, we make sure that the search for a word that is not included in the list of stop words,
	// finds the desired document
	{
		vector<string> stop_word = { "at"s };
		SearchServer server(stop_word);
		server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
		const auto found_docs = server.FindTopDocuments("in"s);
		ASSERT_EQUAL(found_docs.size(), 1u);
		const Document& doc0 = found_docs[0];
		ASSERT_EQUAL(doc0.id, doc_id);
	}

	// Then we make sure that the search for the same word included in the list of stop words
	// returns an empty result
	{
		SearchServer server("in the"s);
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
		string stop_word = "in the"s;
		SearchServer server(stop_word);
		server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
		const auto found_docs = server.FindTopDocuments("white"s);
		ASSERT_EQUAL(found_docs.size(), 1);
		const Document& doc0 = found_docs[0];
		ASSERT_EQUAL(doc0.id, doc_id);
	}

	{
		SearchServer server("in the"s);
		server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
		const auto found_docs = server.FindTopDocuments("dog"s);
		ASSERT_EQUAL_HINT(found_docs.size(), 0, "This document should not exist."s);
	}
}

SearchServer AddFewDocsForTests()
{
	SearchServer search_server("and in on with"s);
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

	SearchServer search_server("and in on with"s);
	search_server.AddDocument(0, "white cat with fancy white collar"s, DocumentStatus::ACTUAL, { rating_one });
	search_server.AddDocument(1, "fluffy cat fluffy tail tufts on the ears"s, DocumentStatus::ACTUAL, { rating_two });
	search_server.AddDocument(2, "well-groomed cat expressive eyes"s, DocumentStatus::ACTUAL, { rating_three });
	const auto found_docs = search_server.FindTopDocuments("yellow eyed cat"s);

	ASSERT(found_docs[0].rating == (7 + 2 + 7) / 3 && found_docs[1].rating == (8 + (-3)) / 2 && found_docs[2].rating == ((5 + (-12) + 2 + 1) / 4));
}

void TestFilterDocumentsByUsingPredicate()
{
	SearchServer search_server("and in on with"s);
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
			[](int document_id, DocumentStatus status, int rating)
			{
				return document_id == 7;
			});
		ASSERT(found_docs.size() == 1 && found_docs.at(0).id == 7 && found_docs.at(0).rating == -1);
	}
	{
		vector<Document> found_docs = search_server.FindTopDocuments("cat"s,
			[](int document_id, DocumentStatus status, int rating)
			{
				return rating == -1;
			});
		ASSERT(found_docs.size() == 2 && found_docs.at(0).id == 2 && found_docs.at(1).id == 7 && found_docs.at(0).rating == -1 && found_docs.at(1).rating == -1);
	}
	{
		vector<Document> found_docs = search_server.FindTopDocuments("cat"s,
			[](int document_id, DocumentStatus status, int rating)
			{
				return status == DocumentStatus::IRRELEVANT;
			});
		ASSERT(found_docs.size() == 2 && found_docs.at(0).id == 1 && found_docs.at(1).id == 6);
	}
}

void TestSearchDocsByStatus()
{
	SearchServer search_server("and in on with"s);
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
		SearchServer server("in the"s);
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