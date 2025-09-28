/*
 * ibkr_parser.cpp
 *
 *  Created on: 16.04.2022
 *      Author: AndreasFellnhofer
 */

#include "ibkr_parser.hpp"
#include "tranche.hpp"
#include "currency.hpp"
#include "security.hpp"

#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <sstream>
#include <iostream>
#include <vector>
#include <ctime>
#include <iomanip>
#include <cmath>
#include <regex>
#include <numeric>
#include <string>


namespace ibkr {

using namespace std;


namespace csv {
const char delimiter = ',';
const char delimiter_inner = ' ';


namespace trades {

enum col : uint8_t
{
	CATEGORY = 3,
	CURRENCY = 4,
	SYMBOL = 5,
	DATE = 6,
	AMOUNT = 7,
	PRICE = 8,
	COSTN = 9,
	FEE = 10,
	COSTG = 11,
	PNL = 12,
};

} /* ns trades */


namespace forex {

enum col : uint8_t
{
	CLASS = 4,
	DATE = 5,
	CURRENCY = 6,
	QUANTI = 7,
	EARNING = 8,
	BASIS = 9,
	CODE = 11,
};

} /* ns forex */

namespace interest {

enum col : uint8_t
{
	CURRENCY = 2,
	DATE = 3,
	EARNING = 5,
};

} /* ns interest */

namespace holding {

enum col : uint8_t
{
	TYPE = 1,
	SYMBOL = 2,
	CURRENCY = 3,
	AMOUNT = 4,
	BASIS = 5,
	PRICE = 6,
};

} /* ns holding */

namespace action {

enum col : uint8_t
{
	TYPE = 2,
	CURRENCY = 3,
	DATE = 5,
	DESCR = 6,
	AMOUNT = 7,
};

} /* ns action */

} /* ns csv */

template <typename T>
inline decltype(T::id) match_to_id(const string &s, const T *array_of_structs, size_t n)
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
		token = std::regex_replace(token, std::regex("^ +"), ""); // remove one or more leading blanks
		token = std::regex_replace(token, std::regex(" +$"), ""); // remove one or more trailing blanks
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


void vectorize_inner(const string &str, vector <string> &result)
{
	auto stream = stringstream(str);

	string token;

	while (getline(stream, token, csv::delimiter_inner))
	{
		result.push_back(token);
	}
}


currency::unit currency_from_string(const std::string &str)
{
	auto id = match_to_id<const currency::unit>(str,
			currency::match, sizeof(currency::match)/sizeof(currency::match[0])
			);
	return currency::from_symbol(id);
}


void ibkr_parser::parse()
{
	string line;
	int temp_cnt = 0;

	while (getline(istream, line))
	{
		std::tm time; /* timestamp of each entry */

		const bool isTrade = line.rfind("Trades,Data,Order", 0) == 0;
		const bool isForex = line.rfind("Forex P/L Details,Data,Forex", 0) == 0;
		const bool isHolding = line.rfind("Initial Holding", 0) == 0;
		const bool isAction = line.rfind("Corporate Actions", 0) == 0;
		const bool isInterest = (line.rfind("Interest,Data", 0) == 0) && (line.rfind("Interest,Data,Total", 0) != 0);
		const bool isTaxWHold = (line.rfind("Withholding Tax,Data", 0) == 0) && (line.rfind("Withholding Tax,Data,Total", 0) != 0);
		const bool isDividend = (line.rfind("Dividends,Data", 0) == 0) && (line.rfind("Dividends,Data,Total", 0) != 0);

		vector <string> v;

		if (isTrade || isForex || isHolding || isAction || isInterest || isTaxWHold || isDividend)
		{
			if (isInterest)
			{
				printf("");
			}
			vectorize(line, v);
			/* fixme debug logging
			cout << line << endl;
			copy(v.begin(), v.end(), ostream_iterator<string>(cout, "|"));
			cout << endl;
			 */
		}

		if (isHolding)
		{
			std::string const &type {v[csv::holding::col::TYPE]};
			const bool isCash = type.rfind("Cash", 0) == 0;
			const bool isOption = type.rfind("Option", 0) == 0;

			//u = currency_from_string(symbol.substr(0, symbol.find(".")));
			const currency::unit cur { currency_from_string(v[csv::holding::col::CURRENCY]) };
			if (cur == currency::UNKNOWN)
			{
				std::cerr << "ERROR: Holding " << v[csv::holding::col::SYMBOL] << " has unknown currency '" << v[csv::holding::col::CURRENCY] << "'" << std::endl;
				exit(1);
			}

			const currency::price price_one = {cur, stod(v[csv::holding::col::PRICE])};
			const currency::price price_all = {cur, stod(v[csv::holding::col::BASIS])};

			security sec(v[csv::holding::col::SYMBOL], price_one);
			sec.setType(isCash ? security::CURRENCY : isOption ? security::OPTION : security::EQUITY);

			const double amount = stod(v[csv::holding::col::AMOUNT]);
			auto p_tranche = std::make_unique<tranche>(sec, amount, price_all, currency::price {currency::USD, 0.0});
			p_tranche->setType(tranche::HOLD);

			if (cbk_initial_holding) { cbk_initial_holding(time, p_tranche); }
		}
		else if (isInterest || isTaxWHold || isDividend)
		{
			std::stringstream ss2(v[csv::interest::col::DATE]);
			ss2 >> std::get_time(&time, "%Y-%m-%d");
			const currency::price earned = {	currency_from_string(v[csv::interest::col::CURRENCY]), 
											stod(v[csv::interest::col::EARNING])
										  };

			const char *name = isInterest ? "Zinsen" : isTaxWHold ? "Quellensteuer" : isDividend ? "Dividende" : "Sonderzahlung";
			security interest(name, currency::price {earned.unit, 1.0});
			interest.setType(ibkr::security::INTEREST);
			auto tran = std::make_unique<tranche>(interest, stod(v[csv::interest::col::EARNING]), 
												earned,  currency::price {earned.unit, 0.0});
			tran->setTimeStamp(time);
			if (earned.value > 0) { tran->setType(tranche::BUY); }
			else { tran->setType(tranche::SELL); }
			tran->makeAbsolute();

			if (cbk_forex && interest.getPrice().unit != currency::EUR) { cbk_forex(time, tran); }
		}
		else if (isAction &&
				(v[csv::action::col::TYPE] == "Stocks") &&
				(v[csv::action::col::DESCR].find("Split") != std::string::npos))
		{
			std::stringstream ss2(v[csv::action::col::DATE]);
			ss2 >> std::get_time(&time, "%Y-%m-%d %H:%M:%S");
			const std::string action = v[csv::action::col::DESCR];

			const double amount = stod(v[csv::action::col::AMOUNT]);
			const currency::price price = {currency_from_string(v[csv::action::col::CURRENCY]), 0.0};

			const std::string symbol = action.substr(0, action.find("("));
			security sec(symbol, price);

			auto p_tranche = std::make_unique<tranche>(sec, amount, price, currency::price{price.unit, 0.0});
			p_tranche->getSecurity().setType(security::EQUITY);
			p_tranche->setTimeStamp(time);
			p_tranche->setType(amount < 0 ? tranche::SPLIT_OUT : tranche::SPLIT_IN);
			p_tranche->makeAbsolute();

			if (cbk_stocksplit) { cbk_stocksplit(time, p_tranche); }
		}
		else if (isTrade)
		{
			std::stringstream ss2(v[csv::trades::col::DATE]);
			ss2 >> std::get_time(&time, "%Y-%m-%d %H:%M:%S");

			currency::price price = {
				currency_from_string(v[csv::trades::col::CURRENCY]),
				stod(v[csv::trades::col::PRICE])
			};

			const security sec(v[csv::trades::col::SYMBOL], price);

			const double amount = stod(v[csv::trades::col::AMOUNT]);
			const currency::price fee = {currency::USD, stod(v[csv::trades::col::FEE])};
			price.value = stod(v[csv::trades::col::COSTN]);

			auto p_tranche = std::make_unique<tranche>(sec, amount, price, fee, false);
			p_tranche->setTimeStamp(time);
			if (amount < 0) { p_tranche->setType(tranche::SELL); }
			p_tranche->makeAbsolute();

			switch (match_to_id<const trade::unit>(v[3], trade::match, sizeof(trade::match)/sizeof(trade::match[0])))
			{
				case trade::type::STOCKS:
				{
					p_tranche->getSecurity().setType(security::EQUITY);
					if (cbk_stock_trade)
					{
						cbk_stock_trade(time, p_tranche);
					}
				} break;

				case trade::type::FOREX:
				{
					p_tranche->getSecurity().setType(security::CURRENCY);
					if (cbk_forex_trade)
					{
						cbk_forex_trade(time, p_tranche);
					}
					//cout << "CASH " << *tr << endl;
				} break;

				case trade::type::OPTIONS:
				{
					char normalized[32];
					normalized_option_key(p_tranche->getSecurity().getName(), normalized, 31);
					p_tranche->getSecurity().setName(normalized);
					p_tranche->getSecurity().setType(security::OPTION);
					p_tranche->setQuanti(p_tranche->getQuanti() * 100);
					if (cbk_options_trade)
					{
						cbk_options_trade(time, p_tranche);
					}
				} break;

				case trade::type::FUTURES:
				{
					p_tranche->getSecurity().setType(ibkr::security::FUTURE);

					if (cbk_stock_trade)
					{
						cbk_stock_trade(time, p_tranche);
					}
					//cout << "CASH " << *tr << endl;
				} break;

				default:
				{
					cout << "unknown trade::type!" << '\n';
				} break;
			}
		}
		else if (isForex)
		{
			std::stringstream ss2(v[csv::forex::col::DATE]);
			ss2 >> std::get_time(&time, "%Y-%m-%d %H:%M:%S");

			double quanti = -stod(v[csv::forex::col::QUANTI]);
			int column_earning = csv::forex::col::EARNING;
			double sign_earning = 1.0;

			if ((quanti < 0.0) &&
				(v.size() > csv::forex::col::CODE) &&
				(v[csv::forex::col::CODE].front() == 'C'))
			{
				column_earning = csv::forex::col::BASIS;
				sign_earning = -1.0;
			}

			double const price_per_share = abs(stod(v[column_earning]) / stod(v[csv::forex::col::QUANTI]));

			currency::price price = {
				currency_from_string(v[csv::forex::col::CURRENCY]),
				price_per_share
			};

			currency::price fee = {
				currency_from_string(v[csv::forex::col::CURRENCY]),
				0.0
			};

			security cash(v[csv::forex::col::CLASS], price);
			cash.setType(security::CURRENCY);

			price.value = sign_earning * stod(v[column_earning]);

			
			if (v[csv::forex::col::CLASS].find(" Interest ") != std::string::npos)
			{
				/* Fixme: At the moment the separate entries are used with ECB exchange rates, not the Forex P/L details */
			}
			else if (v[csv::forex::col::CLASS].find(" margin") != std::string::npos)
			{
				cash.setName("MARGIN");
				auto tran = std::make_unique<tranche>(cash, quanti, price, fee, false);
				tran->setTimeStamp(time);

				if (quanti < 0) { tran->setType(tranche::BUY); }
				else { tran->setType(tranche::SELL); }

				tran->makeAbsolute();

				if (cbk_forex) { cbk_forex(time, tran); }
			}
			else if ((v[csv::forex::col::CLASS].find("(") == std::string::npos) &&
				(v[csv::forex::col::CLASS].find("Net cash activity") == std::string::npos)) /* check if dividend */
			{
				auto stream = stringstream(v[csv::forex::col::CLASS]);
				string token;
				vector <std::string> tokenized;

				while (getline(stream, token, ' '))
				{
					tokenized.push_back(token);
					//cout << token;
				}

				if (tokenized.size() > 2)
				{
					if (tokenized.size() == 3)
					{
						cash.setName(tokenized[2]);
					}
					else
					{
						cash.setName(tokenized[2] + std::accumulate(tokenized.begin() + 3, tokenized.end(), std::string(" ")));
					}
					if (tokenized[0].find("Forex") != string::npos)
					{
						std::size_t const dotpos = tokenized[2].find(".");
						if (dotpos == string::npos)
						{
							throw std::runtime_error("ERROR: Cannot forex parse.");
						}

						/* e.g. Devisen    Kauf -10000 EUR.USD USD:11941 EUR:10001,67484
						 * e.g. Devisen Verkauf   1950 EUR.USD USD:-2317 EUR:1948,3112
						 */
						if (tokenized[2].substr(0, dotpos) == "EUR")
						{
							// the fee can only be calculated, if one part was EUR.
							// otherwise it is wrong. e.g. for RUB.USD. In such cases
							// the fee is to be found in the order!
							double eur = abs(stod(tokenized[1]));
							fee.value = abs(price.value - eur);
							fee.unit = currency::EUR;
							price.value = eur;
						}
						else
						{
							std::cout << "WARNING: Parsed Forex transaction without EUR: " << tokenized[2] << std::endl;
						}
					}
					else if (tokenized[0].find("Option") != string::npos)
					{
						cash.setType(security::OPTION);
						quanti = stod(tokenized[1]) * 100; /* amount already set for FX */
						char normalized[32];
						normalized_option_key(cash.getName(), normalized, 31);
						cash.setName(normalized);
					}
					else if (tokenized[0].find("Future") != string::npos)
					{
						cash.setType(security::FUTURE);
						quanti = stod(tokenized[1]); /* amount already set for FX */

						/* future... is special. Can cost or earn forex */
						if (((stod(v[csv::forex::col::QUANTI]) < 0.0) && (quanti < 0)) ||
							((stod(v[csv::forex::col::QUANTI]) >= 0.0) && (quanti > 0)))
							{
								price.value = price.value * -1.0;
							}
					}
					else
					{
						cash.setType(security::EQUITY);
						quanti = stod(tokenized[1]); /* amount already set for FX */
					}
				}

				auto tran = std::make_unique<tranche>(cash, quanti, price, fee, false);
				tran->setTimeStamp(time);

				if (quanti < 0) { tran->setType(tranche::BUY); }
				else { tran->setType(tranche::SELL); }
				
				if (cash.getType() != security::FUTURE) /* hack. Futures selling also costs forex money. */
				{
					tran->makeAbsolute();
				}
				else
				{
					tran->setQuanti(abs(tran->getQuanti()));
				}

				if (tran->getSecurity().getName().find("GE") != std::string::npos)
				{
					printf("");
				}
				if (cbk_forex) { cbk_forex(time, tran); }
				//cout << "forex " << *tr << endl;
				temp_cnt++;
			}
			else /* net cash activity */
			{
				/* disabled --> dividend etc. is already handled above 
				cash.setName("NETCASH");
				auto tr = std::make_unique<tranche>(cash, quanti, price, fee, false);
				tr->setTimeStamp(tm);
				if (quanti < 0) tr->setType(tranche::BUY);
				else tr->setType(tranche::SELL);
				tr->makeAbsolute();

				if (cbk_forex) cbk_forex(tm, tr);
				*/
			}
		}
		else
		{
			// cout << "Cannot parse: " + line << endl;
			/* cannot parse */
		}
	} /* while getline */

	printf("ibkr_parser summary: %d\n", temp_cnt);
} /* parse */


} /* namespace ibkr */
