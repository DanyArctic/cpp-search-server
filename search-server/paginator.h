#pragma once
#include <vector>

template <typename Iterator>
class IteratorRange
{
public:
	IteratorRange(const Iterator begin, const Iterator end)
		: begin_(begin),
		end_(end),
		size_(distance(begin_, end_))
	{}

	auto Begin() const
	{
		return begin_;
	}
	auto End() const
	{
		return end_;
	}
	size_t Size() const
	{
		return std::distance(begin_, end_);
	}

private:
	Iterator begin_;
	Iterator end_;
	size_t size_;
};

template <typename Iterator>
class Paginator
{
public:
	Paginator(Iterator begin, Iterator end, const size_t page_size)
	{
		Iterator left = begin;
		Iterator right = begin;

		while (size_t(std::distance(right, end)) > page_size)
		{
			std::advance(right, page_size);
			pages_.push_back(IteratorRange(left, right));
			left = right;
		}

		if (right != end)
		{
			pages_.push_back(IteratorRange(right, end));
		}
	}

	auto begin() const
	{
		return pages_.begin();
	}

	auto end() const
	{
		return pages_.end();
	}

	auto size() const
	{
		return pages_.size();
	}

private:
	std::vector<IteratorRange<Iterator>> pages_;
};

template <typename Container>
auto Paginate(const Container& c, size_t page_size)
{
	return Paginator(begin(c), end(c), page_size);
}

template <typename Iterator>
void PrintRange(Iterator begin, Iterator end)
{
	for (auto it = begin; it != end; ++it)
	{
		std::cout << *it;
	}
}

template<typename Iterator>
std::ostream& operator<< (std::ostream& output, IteratorRange<Iterator> it_range)
{
	PrintRange(it_range.Begin(), it_range.End());
	return output;
}