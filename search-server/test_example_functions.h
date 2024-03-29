#pragma once
#include "search_server.h"
#include "log_duration.h"
#include <string>
#include <random>
#include <iostream>

#define RUN_TEST(func) RunTestImpl((func), #func)
#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)
#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))
#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)
#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

using namespace std;

template <typename FUNC>
void RunTestImpl(FUNC test_function, const std::string& func_name)
{
	test_function();
	std::cerr << func_name << " OK"s << std::endl;
}

void AssertImpl(bool value, const std::string& expr_str, const std::string& file, const std::string& func, unsigned line, const std::string& hint);

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const std::string& t_str, const std::string& u_str, const std::string& file, const std::string& func, unsigned line, const std::string& hint)
{
	if (t != u)
	{
		std::cerr << boolalpha;
		std::cerr << file << "("s << line << "): "s << func << ": "s;
		std::cerr << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
		std::cerr << t << " != "s << u << "."s;
		if (!hint.empty())
		{
			cerr << " Hint: "s << hint;
		}
		std::cerr << std::endl;
		abort();
	}
}

void TestExcludeStopWordsFromAddedDocumentContent();
void TestAddDocument();
SearchServer AddFewDocsForTests();
void TestExcludeDocumentsWithMinusWordsFromSearchResults();
void TestMatchDocuments();
void TestFoundDocsSortInDescendingOrderOfRelevance();
void TestRatingCalc();
void TestFilterDocumentsByUsingPredicate();
void TestSearchDocsByStatus();
void TestRelevanceCalculate();
void TestSearchServer();
void ParallelSearchBenchmark();