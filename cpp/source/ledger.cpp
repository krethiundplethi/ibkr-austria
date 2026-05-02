/*
 * depot.cpp
 *
 *  Created on: 27.03.2022
 *      Author: AndreasFellnhofer
 */

#include "ledger.hpp"
#include <ctime>

namespace ibkr {

ledger::~ledger()
{
}


void ledger::add_entry(entry::type type, const timepoint &timep, const tranche &trch)
{
	if (type == entry::type::CREDIT)
	{
		credit_entries.push_back(entry(type, timep, trch));
	}
}


std::ostream &operator<<(std::ostream &out, const ledger &lgr)
{
	vector <entry> const *deb = nullptr;
	vector <entry> const *cre = nullptr;
	char buf[64];

	lgr.get_entries(&deb, &cre);

	for(const auto & ite : *deb)
	{
		std::strftime(buf, sizeof(buf), "%c", &(ite.tp));
		out << "debit date:" << buf << ", " << ite.tr << std::endl;
	}

	for(const auto & ite : *cre)
	{
		std::strftime(buf, sizeof(buf), "%c", &(ite.tp));
		out << "credit date:" << buf << ", " << ite.tr << std::endl;
	}

	return out;
}

} /* namespace ibkr */
