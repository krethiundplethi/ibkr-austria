#ifndef EXCHANGE_RATE_PARSER_H
#define EXCHANGE_RATE_PARSER_H

#include <string>
#include <unordered_map>
#include <vector>
#include <ctime>

class RateParser {
public:
    bool open(const std::string& filename);
    double get(const std::string& currency, const std::tm &tm) const;

private:
    void parse(std::istream& stream);

    std::unordered_map<std::string, std::unordered_map<std::string, double>> data;
    std::vector<std::string> currencies;
};

#endif // EXCHANGE_RATE_PARSER_H
