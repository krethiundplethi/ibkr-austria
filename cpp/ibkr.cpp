
#include "currency.hpp"
#include "security.hpp"
#include "tranche.hpp"
#include "ledger.hpp"
#include "ibkr_parser.hpp"

#include "CLI11.hpp"

#include <iostream>

using namespace std;
using namespace ibkr;


std::map <std::string, std::unique_ptr<tranche>> map_stock_trades;
std::map <std::string, std::unique_ptr<tranche>> map_forex_trades;


void cbk_stock(const std::tm &tm, std::unique_ptr<tranche> &p_tranche)
{
	char buf[32];
	// cout << "STONK " << *p_tranche << endl;
	//auto buf = std::make_unique<char[]>(32); /*only 17, better safe than sorry YYYYMMDDHHMMSSxxx*/

	/* man, c++ can be shitty. doing this in a safe way is 15 lines of code. so'll do it unsafe. */
	snprintf(buf, 31, "%04u%02u%02u%02u%02u%02u%-04s",
			1900 + tm.tm_year, 1 + tm.tm_mon, tm.tm_mday,
			tm.tm_hour, tm.tm_min, tm.tm_sec,
			p_tranche->getSecurity().getName().c_str());

	map_stock_trades[std::string(buf)] = std::move(p_tranche);

}


void cbk_forex(const std::tm &tm, std::unique_ptr<tranche> &p_tranche)
{
	char buf[32];
	// cout << "STONK " << *p_tranche << endl;
	//auto buf = std::make_unique<char[]>(32); /*only 17, better safe than sorry YYYYMMDDHHMMSSxxx*/

	/* man, c++ can be shitty. doing this in a safe way is 15 lines of code. so'll do it unsafe. */

	auto ss = stringstream(p_tranche->getSecurity().getName());
	string token;
	vector <std::string> tokenized;

	tokenized.emplace_back("");
	while (getline(ss, token, ' '))
	{
		tokenized.push_back(token);
	}

	snprintf(buf, 31, "%04u%02u%02u%02u%02u%02u%-04s",
			1900 + tm.tm_year, 1 + tm.tm_mon, tm.tm_mday,
			tm.tm_hour, tm.tm_min, tm.tm_sec,
			tokenized.back().c_str());

	map_forex_trades[std::string(buf)] = std::move(p_tranche);

}


int main(int argc, char **argv)
{
    //CLI::App app{"Tax calc"};

    std::string filename("C:\\Development\\github\\ibkr-austria\\U6443611_20210618_20211231.csv");
    //app.add_option("-f,--file", filename, "csv file from IBKR");

    //CLI11_PARSE(app, argc, argv);

	cout << "Opening file: " << filename << endl;

	/*
	currency::price p = {currency::USD, 1.2};
	security aktie("gazprom",  p);
	tranche tranche1({"gazprom", {currency::USD, 1.2}}, 100, p, {currency::USD, 3}, false);
	cout << tranche1 << endl;

	ledger l("gazprom");

	time_t rawtime;
	struct tm * ptm;
	time(&rawtime);
	ptm = gmtime(&rawtime);

	l.add_entry(entry::type::CREDIT, *ptm, tranche1);
	l.add_entry(entry::type::CREDIT, *ptm, tranche1);

	cout << l << endl;
	 */

	ibkr_parser parser(filename);
	parser.register_callback_on_stock(cbk_stock);
	parser.register_callback_on_forex(cbk_forex);
	parser.parse();

	for (auto const &elem: map_stock_trades)
	{
		cout << elem.first << ": " << *elem.second << endl;
	}

	for (auto const &elem: map_forex_trades)
	{
		cout << elem.first << ": " << *elem.second << endl;
	}
	cout.flush();

	return 0;
}
