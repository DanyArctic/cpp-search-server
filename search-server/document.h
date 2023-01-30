#pragma once
#include <iostream>
#include <string>

struct Document
{
	Document() = default;

	Document(int id, double relevance, int rating);

	int id = 0;
	double relevance = 0.0;
	int rating = 0;
};

std::ostream& operator<<(std::ostream& output, const Document& document);

void PrintDocument(const Document& document);