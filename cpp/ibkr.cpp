
#include "currency.hpp"
#include "security.hpp"
#include "tranche.hpp"
#include "ledger.hpp"
#include "ibkr_parser.hpp"

#include "CLI11.hpp"

#include <iostream>

using namespace std;
using namespace ibkr;


std::map <std::string, tranche> map_stock_trades;


void cbk_stock(const std::tm &tm, const std::unique_ptr<tranche> &p_tranche)
{
	char buf[32];
	cout << "STONK " << *p_tranche << endl;
	//auto buf = std::make_unique<char[]>(32); /*only 17, better safe than sorry YYYYMMDDHHMMSSxxx*/
	snprintf(buf, 31, "%04u%02u%02u%02u%02u%02u%03s",
			1900 + tm.tm_year, 1 + tm.tm_mon, tm.tm_mday,
			tm.tm_hour, tm.tm_min, tm.tm_sec,
			p_tranche->getSecurity().getName());

	printf(buf);
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
	parser.parse();

	cout.flush();

	return 0;
}
