/*
 * ibkr_parser.cpp
 *
 *  Created on: 16.04.2022
 *      Author: AndreasFellnhofer
 */

#include "ibkr_parser.hpp"
#include "ledger.hpp"
#include <iostream>
#include <iterator>
#include <vector>
#include <ctime>
#include <iomanip>

namespace ibkr {

using namespace std;

const char *csv_delimiter = ",";



trade_type parse_trade_type(string &s)
{
	trade_type result = trade_type::UNKNOWN;

	for (int i = 0; i < sizeof(trade_parse)/sizeof(trade_parse)[0]; ++i)
	{
		if (!s.compare(trade_parse[i].match))
		{
			result = trade_parse[i].t;
		}
	}

	return result;
}


void vectorize(const string &s, vector <string> &result, std::tm &tm)
{
	auto ss = stringstream(s);

	string token;
	string temp;
	bool escaped = false;

	while(getline(ss, token, ','))
	{
		if (escaped && (token[token.length()-1] == '\"'))
		{
			temp += token;
			escaped = false;
			result.push_back(temp.substr(1, temp.length() - 2));
		}
		else if (escaped)
		{
			temp += token;
		}
		else if (token[0] == '\"')
		{
			temp = token;
			escaped = true;
		}
		else
		{
			result.push_back(token);
		}
	}

	std::stringstream ss2(result[6]);
	ss2 >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
}


void ibkr_parser::parse(void)
{
	string line;
	timepoint tp;
	std::tm tm;
	bool isTrade = false;
	bool isForex = false;

	cout << "file:";
	while (getline(istream, line))
	{
		isTrade = line.rfind("Trades,Data,Order", 0) == 0;
		isForex = line.rfind("Forex P/L Details,Data,", 0) == 0;
		if (isTrade)
		{
			vector <string> v;
		    cout << line << endl;
		    vectorize(line, v, tm);
		    copy(v.begin(), v.end(), ostream_iterator<string>(cout, "|"));
		    cout << endl;

		    switch (parse_trade_type(v[3]))
		    {
		    	case trade_type::STOCKS: cout << "STONK" << endl; break;
		    	case trade_type::FOREX: cout << "CASH " << endl; break;
		    	case trade_type::OPTIONS: cout << "OPTI" << endl; break;
		    	default: cout << "NAN!" << endl; break;
		    }
		}
	}
}


} /* namespace ibkr */
