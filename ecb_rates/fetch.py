#!/usr/bin/env python3

import requests
import pandas as pd
from collections import defaultdict
import argparse
from datetime import datetime


def validate_date(date_str):
    try:
        date = datetime.strptime(date_str, "%Y-%m-%d")
        if not (1 <= date.month <= 12 and 1 <= date.day <= 31):
            raise ValueError
        return date_str
    except ValueError:
        raise argparse.ArgumentTypeError(f"Invalid date format or value: '{date_str}'. Expected YYYY-MM-DD.")


def fetch_ecb_rates(currencies, start_date, end_date):
    symbol_str = "+".join(currencies)
    url = (f"https://data-api.ecb.europa.eu/service/data/EXR/D.{symbol_str}.EUR.SP00.A"
           f"?startPeriod={start_date}&endPeriod={end_date}&format=jsondata")

    r = requests.get(url)
    r.raise_for_status()
    data = r.json()

    series = data['dataSets'][0]['series']
    structure = data['structure']
    obs_dates = [v['id'] for v in structure['dimensions']['observation'][0]['values']]

    # Identify index of 'CURRENCY' in the series key
    series_dims = structure['dimensions']['series']
    currency_dim_index = next(i for i, d in enumerate(series_dims) if d['id'] == 'CURRENCY')

    # Build index-to-currency map
    currency_values = series_dims[currency_dim_index]['values']
    currency_map = {str(i): val['id'] for i, val in enumerate(currency_values)}

    rates = defaultdict(dict)
    for series_key, content in series.items():
        parts = series_key.split(":")
        currency_idx = parts[currency_dim_index]
        currency = currency_map[currency_idx]
        for obs_idx, val in content['observations'].items():
            date = obs_dates[int(obs_idx)]
            rates[date][currency] = val[0]

    df = pd.DataFrame.from_dict(rates, orient='index').sort_index()
    return df


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Fetch ECB exchange rates for given currencies and date range.")
    parser.add_argument("currencies", nargs='+', help="List of 3-letter currency codes (e.g. USD AUD GBP)")
    parser.add_argument("--start", type=validate_date, default="2023-01-01", help="Start date (YYYY-MM-DD)")
    parser.add_argument("--end", type=validate_date, default="2023-12-31", help="End date (YYYY-MM-DD)")
    parser.add_argument("--output", default="ecb_rates.csv", help="Output CSV file name")
    parser.add_argument("--transpose", action="store_true", help="Transpose output so rows are currencies and columns are dates")
    args = parser.parse_args()

    df = fetch_ecb_rates(args.currencies, args.start, args.end)
    if args.transpose:
        df = df.transpose()
        df.to_csv(args.output, index_label="currency")
    else:
        df.to_csv(args.output, index_label="date")
    print(f"Saved as {args.output}")

