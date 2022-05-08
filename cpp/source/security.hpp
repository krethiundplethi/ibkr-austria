/*
 * security.h
 *
 *  Created on: 27.03.2022
 *      Author: AndreasFellnhofer
 */

#ifndef SOURCE_SECURITY_H_
#define SOURCE_SECURITY_H_

#include "currency.hpp"

namespace ibkr
{

struct security
{
public:
	enum type {

	};

	security() = delete;
	security(const char *name, const currency::price price)
		: name {name}, price {price} {}

	security(const security &rval)
		: name {rval.name}, price {rval.price} { };

	inline const currency::price &getPrice() const { return price; }
	inline const char * getName() const { return name; }


	~security() { };
private:
	const char *name;
	const currency::price price;
};

std::ostream &operator<<(std::ostream &, const security &);

} /* namespace ibkr */

#endif /* SOURCE_SECURITY_H_ */
