/*
 * ibkr_parser.cpp
 *
 *  Created on: 16.04.2022
 *      Author: AndreasFellnhofer
 */

#include "ibkr_parser.hpp"
#include "currency.hpp"
#include "ledger.hpp"
#include <iostream>
#include <iterator>
#include <vector>
#include <ctime>
#include <iomanip>

namespace ibkr {

using namespace std;


namespace csv {
const char delimiter = ',';

namespace trades {

enum col
{
	CLASS = 3,
	CURRENCY = 4,
	SYMBOL = 5,
	DATE = 6,
	AMOUNT = 7,
	PRICE = 8,
	COSTN = 9,
	FEE = 10,
	COSTG = 11,
	PNL = 12
};

}
}



template <typename T>
inline decltype(T::id) match_to_id(string &s, const T *array_of_structs, size_t n)
{
	decltype(T::id) result = decltype(T::id)::UNKNOWN;

	for (int i = 0; i < n; ++i)
	{
		if (!s.compare(array_of_structs[i].name))
		{
			result = array_of_structs[i].id;
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

	while (getline(ss, token, csv::delimiter))
	{
		if (escaped && (token[token.length() - 1] == '\"'))
		{
			temp += token;
			result.push_back(temp.substr(1, temp.length() - 2));
			escaped = false;
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

		    switch (match_to_id<const trade::unit>(v[3], trade::match, sizeof(trade::match)/sizeof(trade::match[0])))
		    {
		    	case trade::type::STOCKS:
		    		if (cbk_stock)
		    		{
		    			currency::price p = {currency::USD, 1.2};
		    			auto id = match_to_id<const currency::unit>(v[csv::trades::col::CURRENCY],
		    					currency::match, sizeof(currency::match)/sizeof(currency::match[0])
								);
		    			p.unit = currency::from_symbol(id);
		    			p.value = stof(v[csv::trades::col::PRICE]);
		    			security aktie(v[csv::trades::col::SYMBOL].c_str(),  p);
		    			p.value = stof(v[csv::trades::col::COSTN]);
		    			int amount = stoi(v[csv::trades::col::AMOUNT]);
		    			tranche tranche1(aktie, amount, p, {currency::USD, stof(v[csv::trades::col::FEE])}, false);
		    			if (amount < 0) tranche1.setType(tranche::SELL);
	    				tranche1.makeAbsolute();
		    			cbk_stock(tranche1);
			    		cout << "STONK" << tranche1 << endl;
		    		}
		    		break;
		    	case trade::type::FOREX: cout << "CASH " << endl; break;
		    	case trade::type::OPTIONS: cout << "OPTI" << endl; break;
		    	default: cout << "NAN!" << endl; break;
		    }
		}
	}
}


} /* namespace ibkr */
