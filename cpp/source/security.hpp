/*
 * security.h
 *
 *  Created on: 27.03.2022
 *      Author: AndreasFellnhofer
 */

#ifndef SOURCE_SECURITY_H_
#define SOURCE_SECURITY_H_

#include "currency.hpp"
#include <string>


namespace ibkr
{

struct security
{
public:
	enum type {
		NONE,
		EQUITY,
		CURRENCY,
		OPTION,
		FUTURE,
		INTEREST, /* hack */ 
	};

	security() = delete;
	security(const std::string &name, const currency::price price)
		: name {name}, price {price} { type = EQUITY; }

	security(const security &rval)
		: name {rval.name}, price {rval.price}, type {rval.type} { };

	inline const currency::price &getPrice() const { return price; }
	inline const std::string &getName() const { return name; }
	inline void setName(std::string const &s) { name = s; }
	inline void setType(type const &t) { type = t; }
	inline const type &getType(void) const { return type; }
	inline bool isEquity(void) const { return type == EQUITY; }

	~security() { };
private:
	std::string name;
	const currency::price price;
	type type;
};

std::ostream &operator<<(std::ostream &, const security &);
bool normalized_option_key(std::string const &src, char *dst, size_t len);

} /* namespace ibkr */

#endif /* SOURCE_SECURITY_H_ */
