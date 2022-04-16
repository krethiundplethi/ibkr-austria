
#include "currency.hpp"
#include "security.hpp"
#include "tranche.hpp"
#include "ledger.hpp"
#include "ibkr_parser.hpp"

#include "CLI11.hpp"

#include <iostream>

using namespace std;
using namespace ibkr;


int main(int argc, char **argv)
{
    CLI::App app{"Tax calc"};

    std::string filename = "input.csv";
    app.add_option("-f,--file", filename, "csv file from IBKR");

    CLI11_PARSE(app, argc, argv);

	cout << "Opening file: " << filename << endl;

	currency::price p = {currency::USD, 1.2};
	security aktie("gazprom",  p);

	tranche tranche1(aktie, 100, p, {currency::USD, 3}, false);
	//tranche tranche2(security("gazprom", currency(currency::USD, 1.2)), 300, currency(currency::EUR, 800.0), 2.0, false);
	//tranche tranche3(security("gazprom", usd(1.3)), 50, eur(100.0), 2.0, false);

	cout << tranche1 << endl;
	//cout << tranche2 << endl;
	//cout << tranche3 << endl;

	ledger l("gazprom");

	time_t rawtime;
	struct tm * ptm;
	time(&rawtime);
	ptm = gmtime(&rawtime);

	l.add_entry(entry::type::CREDIT, *ptm, tranche1);
	l.add_entry(entry::type::CREDIT, *ptm, tranche1);

	cout << l << endl;

	ibkr_parser parser(filename);
	parser.parse();

	cout.flush();

	return 0;
}
