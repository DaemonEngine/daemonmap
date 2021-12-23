#ifndef CONFLOADER_H
#define CONFLOADER_H

#include <vector>
#include <deque>
#include <string>

struct reccord_t
{
	size_t key_id;
	size_t reccord_id;
	std::string value;
};

bool parse(
		std::vector<char> const &data,
		std::deque<reccord_t> &reccords,
		std::vector<std::string> &keys
);

#endif
