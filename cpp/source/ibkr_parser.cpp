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
const char delimiter_inner = ' ';


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

} /* ns trades */


namespace forex {

enum col
{
	CLASS = 4,
	DATE = 5,
	CURRENCY = 6,
	AMOUNT = 7,
	PRICEG = 8,
	PRICEN = 9,
	FEE = 10,
};

} /* ns forex */

} /* ns csv */



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


void vectorize(const string &s, vector <string> &result)
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
}


void vectorize_inner(const string &s, vector <string> &result)
{
	auto ss = stringstream(s);

	string token;

	while (getline(ss, token, csv::delimiter_inner))
	{
		result.push_back(token);
	}
}


currency::unit currency_from_vector(vector <string> &v, size_t col)
{
	auto id = match_to_id<const currency::unit>(v[col],
			currency::match, sizeof(currency::match)/sizeof(currency::match[0])
			);
	return currency::from_symbol(id);
}


void ibkr_parser::parse(void)
{
	string line;
	timepoint tp;
	bool isTrade = false;
	bool isForex = false;

	int temp_cnt = 0;

	while (getline(istream, line))
	{
    	std::tm tm; /* timestamp of each entry */

		isTrade = line.rfind("Trades,Data,Order", 0) == 0;
		isForex = line.rfind("Forex P/L Details,Data,Forex", 0) == 0;

		vector <string> v;

	    if (isTrade || isForex)
	    {
		    cout << line << endl;
	    	vectorize(line, v);
	    	copy(v.begin(), v.end(), ostream_iterator<string>(cout, "|"));
	    	cout << endl;
	    }

	    if (isTrade)
		{
	    	std::stringstream ss2(v[csv::trades::col::DATE]);
	    	ss2 >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");

	    	switch (match_to_id<const trade::unit>(v[3], trade::match, sizeof(trade::match)/sizeof(trade::match[0])))
		    {
		    	case trade::type::STOCKS:
		    	{
					currency::price price = {currency::USD, 0.0};
					price.unit = currency_from_vector(v, csv::trades::col::CURRENCY);
					price.value = stof(v[csv::trades::col::PRICE]);
					const security aktie(v[csv::trades::col::SYMBOL].c_str(),  price);

					int amount = stoi(v[csv::trades::col::AMOUNT]);
					currency::price fee = {currency::USD, stof(v[csv::trades::col::FEE])};
					price.value = stof(v[csv::trades::col::COSTN]);

					auto p_tranche = std::make_unique<tranche>(aktie, amount, price, fee, false);
					if (amount < 0) p_tranche->setType(tranche::SELL);
					p_tranche->makeAbsolute();
					if (cbk_stock)
					{
						cbk_stock(tm, p_tranche);
					}
		    	}break;

		    	case trade::type::FOREX:
		    	{
					auto currency = currency_from_vector(v, csv::trades::col::CURRENCY);
					currency::price price = {
							currency,
							stof(v[csv::forex::col::AMOUNT]) / stof(v[csv::forex::col::PRICEG])
					};
					security cash(v[csv::forex::col::CLASS].c_str(), price);


					currency::price fee = {currency, stof(v[csv::forex::col::FEE])};
					price.value = stof(v[csv::forex::col::PRICEN]);
					int amount = stoi(v[csv::forex::col::AMOUNT]);

					vector <string> iv;
					vectorize_inner(v[csv::forex::col::CLASS], iv);

					auto tr = std::make_unique<tranche>(cash, amount, price, fee, false);
					if (amount < 0) tr->setType(tranche::SELL);
					tr->makeAbsolute();
					if (cbk_forex)
					{
						cbk_forex(tm, tr);
					}
					cout << "CASH " << *tr << endl;
		    	}break;

		    	case trade::type::OPTIONS:
		    	{
		    		if (cbk_options)
		    		{
		    			cout << "OPTI" << endl;
		    		}
		    	} break;

		    	default:
				{
					cout << "NAN!" << endl;
				} break;
		    }
		}
		else if (isForex)
		{
	    	std::stringstream ss2(v[csv::forex::col::DATE]);
	    	ss2 >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");

	    	currency::price price = {
				currency_from_vector(v, csv::forex::col::CURRENCY),
				stof(v[csv::forex::col::AMOUNT]) / stof(v[csv::forex::col::PRICEG])
			};

			security cash(v[csv::forex::col::CLASS].c_str(), price);

			currency::price fee = {currency::USD, stof(v[csv::forex::col::FEE])};

			auto tr = std::make_unique<tranche>(cash, 9999, price, fee, false);

			//if (amount < 0) tranche.setType(tranche::BUY);
			tr->makeAbsolute();
			if (cbk_forex)
			{
				cbk_forex(tm, tr);
			}
			cout << "forex " << *tr << endl;
			temp_cnt++;
		}
		else
		{
			/* cannot parse */
		}
	} /* while getline */

	printf("Summary: %d", temp_cnt);
} /* parse */


} /* namespace ibkr */
