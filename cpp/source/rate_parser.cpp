#include "rate_parser.hpp"
#include <fstream>
#include <sstream>
#include <iostream>


bool RateParser::open(const std::string& filename)
{
	std::ifstream file(filename);

	if (!file.is_open()) 
	{
		std::cerr << "Failed to open file: " << filename << "\n";
		return false;
	}

	parse(file);
	return true;
}


void RateParser::parse(std::istream& stream)
{
	std::string line;

	// First line is the header
	if (std::getline(stream, line))
	{
		std::stringstream headerStream(line);
		std::string cell;
		
		// Skip the first column ("date")
		std::getline(headerStream, cell, ',');
		
		while (std::getline(headerStream, cell, ','))
		{
			currencies.push_back(cell);
		}
	}

	// Parse the rest of the lines
	while (std::getline(stream, line))
	{
		std::stringstream lineStream(line);
		std::string cell, date;
		
		std::getline(lineStream, date, ',');
		auto& currencyMap = data[date];

		for (const auto& currency : currencies)
		{
			if (!std::getline(lineStream, cell, ',')) break;
			currencyMap[currency] = std::stod(cell);
		}
	}
}


double RateParser::get(const std::string& currency, const std::tm &tm) const
{
	char date[48];
	snprintf(date, 47, "%04u-%02u-%02u", 1900 + tm.tm_year, 1 + tm.tm_mon, tm.tm_mday);
	auto dateIt = data.find(date);

	if (dateIt == data.end())
	{
		return -1.0;
	}
	else if (currency == "EUR")
	{
		return 1.0;
	}
	else
	{
		const auto& currencyMap = dateIt->second;
		auto currencyIt = currencyMap.find(currency);

		if (currencyIt == currencyMap.end())
		{
			throw std::runtime_error("ERROR: Cannot find rate for " + currency + ".");
		}

		return currencyIt->second;
	}
}
