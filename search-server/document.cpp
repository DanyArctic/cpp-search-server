#include "document.h"

using namespace std;

Document::Document(int id, double relevance, int rating)
	: id(id)
	, relevance(relevance)
	, rating(rating)
{}

std::ostream& operator<<(std::ostream& output, const Document& document)
{
	output << "{ "s
		<< "document_id = "s << document.id << ", "s
		<< "relevance = "s << document.relevance << ", "s
		<< "rating = "s << document.rating
		<< " }"s;
	return output;
}

void PrintDocument(const Document& document)
{
	std::cout << "{ "s
		<< "document_id = "s << document.id << ", "s
		<< "relevance = "s << document.relevance << ", "s
		<< "rating = "s << document.rating
		<< " }"s << std::endl;
}