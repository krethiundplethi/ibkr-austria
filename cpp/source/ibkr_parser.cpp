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
#include <cmath>


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
	QUANTI = 7,
	EARNING = 8,
	BASIS = 9,
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
					const security aktie(v[csv::trades::col::SYMBOL],  price);

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
		    	} break;

		    	case trade::type::FOREX:
		    	{
					auto currency = currency_from_vector(v, csv::trades::col::CURRENCY);
					currency::price price = {
							currency,
							stof(v[csv::trades::col::AMOUNT]) / stof(v[csv::trades::col::PRICE])
					};
					security cash(v[csv::trades::col::CLASS].c_str(), price);


					currency::price fee = {currency, stof(v[csv::trades::col::FEE])};
					price.value = stof(v[csv::trades::col::PRICE]);
					int amount = stoi(v[csv::trades::col::AMOUNT]);

					vector <string> iv;
					vectorize_inner(v[csv::trades::col::CLASS], iv);

					auto tr = std::make_unique<tranche>(cash, amount, price, fee, false);
					tr->setTimeStamp(tm);
					if (amount < 0) tr->setType(tranche::SELL);
					tr->makeAbsolute();
					if (cbk_forex)
					{
						//cbk_forex(tm, tr);
						// TODO:FIXME
					}
					//cout << "CASH " << *tr << endl;
		    	} break;

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

	    	double price_per_share = stof(v[csv::forex::col::EARNING]) / stof(v[csv::forex::col::QUANTI]);

	    	currency::price price = {
				currency_from_vector(v, csv::forex::col::CURRENCY),
				price_per_share
			};

			currency::price fee = {
				currency_from_vector(v, csv::forex::col::CURRENCY),
				0.0
			};

			security cash(v[csv::forex::col::CLASS].c_str(), price);

			if ((v[csv::forex::col::CLASS].find("(") == std::string::npos) &&
				(v[csv::forex::col::CLASS].find("Net cash activity") == std::string::npos)) /* check if dividend */
			{
				auto ss = stringstream(v[csv::forex::col::CLASS]);
				string token;
				vector <std::string> tokenized;

				while (getline(ss, token, ' '))
				{
					tokenized.push_back(token);
					cout << token;
				}

				price.value = stof(v[csv::forex::col::EARNING]);
				double quanti = -stof(v[csv::forex::col::QUANTI]);
				if (tokenized.size() > 2)
				{
					cash.setName(tokenized[2]);
					if (tokenized[2].find(".") != string::npos)
					{
						cash.setType(security::CURRENCY);
						/* e.g. Devisen    Kauf -10000 EUR.USD USD:11941 EUR:10001,67484
						 * e.g. Devisen Verkauf   1950 EUR.USD USD:-2317 EUR:1948,3112
						 */
						double eur = abs(stof(tokenized[1]));
						fee.value = abs(price.value - eur);
					}
					else
					{
						quanti = stof(tokenized[1]); /* amount already set for FX */
					}
				}

				auto tr = std::make_unique<tranche>(cash, quanti, price, fee, false);
				tr->setTimeStamp(tm);
				if (quanti < 0) tr->setType(tranche::BUY);
				else tr->setType(tranche::SELL);
				tr->makeAbsolute();
				if (cbk_forex)
				{
					cbk_forex(tm, tr);
				}
				//cout << "forex " << *tr << endl;
				temp_cnt++;
			}
		}
		else
		{
			/* cannot parse */
		}
	} /* while getline */

	printf("ibkr_parser summary: %d\n", temp_cnt);
} /* parse */


} /* namespace ibkr */
