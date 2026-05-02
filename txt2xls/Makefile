TXT_FILES := $(wildcard *.txt)
XLSX_FILES := $(TXT_FILES:.txt=.xlsx)
CONVERTER := ./ibkr-txt2xls.py

.PHONY: all clean

all: $(XLSX_FILES)

%.xlsx: %.txt $(CONVERTER)
	$(CONVERTER) $<

clean:
	rm -f $(XLSX_FILES)
