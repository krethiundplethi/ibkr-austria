/*
 * depot.cpp
 *
 *  Created on: 27.03.2022
 *      Author: AndreasFellnhofer
 */

#include "ledger.hpp"
#include <time.h>

namespace ibkr {

ledger::~ledger()
{
}


void ledger::add_entry(entry::type type, const timepoint &tp, const tranche &tr)
{
	if (type == entry::type::CREDIT)
	{
		credit_entries.push_back(entry(type, tp, tr));
	}
}


std::ostream &operator<<(std::ostream &os, const ledger &l)
{
	vector <entry> const *deb = NULL;
	vector <entry> const *cre = NULL;
	char buf[26]; // asctime_s requires buffer of at least 26 chars

	l.get_entries(&deb, &cre);

	for(vector<entry>::const_iterator it = deb->begin(); it != deb->end(); ++it)
	{
		asctime_s(buf, sizeof(buf), &(it->tp));
		buf[strcspn(buf, "\n")] = '\0';
		os << "debit date:" << buf << ", " << it->tr << std::endl;
	}

	for(vector<entry>::const_iterator it = cre->begin(); it != cre->end(); ++it)
	{
		asctime_s(buf, sizeof(buf), &(it->tp));
		buf[strcspn(buf, "\n")] = '\0';
		os << "credit date:" << buf << ", " << it->tr << std::endl;
	}

	return os;
}

} /* namespace ibkr */
