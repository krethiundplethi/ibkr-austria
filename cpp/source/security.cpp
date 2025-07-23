/*
 * security.cpp
 *
 *  Created on: 27.03.2022
 *      Author: AndreasFellnhofer
 */

#include "security.hpp"
#include <map>


namespace ibkr
{

std::ostream &operator<<(std::ostream &o, const security &s)
{
	o << s.getName() << ": " << s.getPrice();
	return o;
}

/*QQQ220819P312.5*/
bool normalized_option_key(std::string const &src_in, char *dst, size_t len)
{
	//return std::format("{}{}{:02}{:02}{}{}.{}", symbol, yy, m, d, p_or_c, val, frac);
	std::string src{src_in};
	char symbol[7] = {0};
	char mon[4] = {0};
	int yy;
	int m;
	int d;
	char p_or_c;
	unsigned int val;
	unsigned int frac;
	int ret;
	bool success = false;

    static const std::map<std::string, int> months
    {
        { "JAN", 1 },
        { "FEB", 2 },
        { "MAR", 3 },
        { "APR", 4 },
        { "MAY", 5 },
        { "JUN", 6 },
        { "JUL", 7 },
        { "AUG", 8 },
        { "SEP", 9 },
        { "OCT", 10 },
        { "NOV", 11 },
        { "DEC", 12 }
    };

	std::size_t foundpc = src.find_last_of("C");
	if (foundpc == std::string::npos) foundpc = src.find_last_of("P");

	if (foundpc == (src.length() - 1)) /*assume QQQ 19AUG22 312.5 P*/
	{
		char *p = (char *) strchr(src.c_str(), ' ');
		if (p)
		{
			*p = '\0';
			strncpy(symbol, src.c_str(), 7);
			ret = sscanf(p + 1, "%2u%3s%2u %u.%u %c", &d, (char*) &mon, &yy, &val, &frac, &p_or_c);
			if (ret == 4) /* no dot, no fraction... try again */
			{
				frac = 0;
				ret = sscanf(p + 1, "%2u%3s%2u %u %c", &d, (char *) &mon, &yy, &val, &p_or_c);
				ret++; /* pretend its six... for the if afterwards */
			}
			const auto iter(months.find(mon));
			if ((ret == 6) && (iter != std::cend(months)))
			{
				m = iter->second;
				success = true;
			}
		}
	}
	else if (foundpc != std::string::npos) /*QQQ   220819P00312500*/
	{
		ret = sscanf(src.c_str(), "%6s%2u%2u%2u%c%05u%3u", symbol, &yy, &m, &d, &p_or_c, &val, &frac);
		if (ret == 7)
		{
			if (!(frac % 10)) frac /= 10; /* remove trailing zeros 0.500 --> 0.5 */
			if (!(frac % 10)) frac /= 10;
			char *p = strchr(symbol, ' ');
			if (p) *p = '\0'; /* strip blank and all after */
			success = true;
		}
	}


	if (success) snprintf(dst, len, "%s%2u%02u%02u%c%u.%u", symbol, yy, m, d, p_or_c, val, frac);
	else  strncpy(dst, src.c_str(), len);

	return success;
}

} /* namespace ibkr */
