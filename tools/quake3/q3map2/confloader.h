#ifndef CONFLOADER_H
#define CONFLOADER_H

#include <vector>
#include <deque>
#include <string>

struct record_t
{
	size_t key_id;
	size_t record_id;
	std::string value;
};

std::vector<char> slurp( char const* filename );

bool parse(
		std::vector<char> const &data,
		std::deque<record_t> &records,
		std::vector<std::string> &keys
);

#endif
