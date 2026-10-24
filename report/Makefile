MAIN ?= main
PDFLATEX ?= pdflatex
BIBER ?= biber

TEX_SOURCES := $(shell find . -type f \( -name '*.tex' -o -name '*.sty' \))
BIB_SOURCES := $(wildcard *.bib)
FIGURE_SOURCES := $(shell find figures -type f 2>/dev/null)
AUX_FILES := $(filter-out ./$(MAIN).aux,$(shell find . -type f -name '*.aux' 2>/dev/null))

.PHONY: all pdf biber clean distclean help

all: pdf

pdf: $(MAIN).pdf

$(MAIN).pdf: $(TEX_SOURCES) $(BIB_SOURCES) $(FIGURE_SOURCES)
	$(PDFLATEX) $(MAIN).tex
	$(BIBER) $(MAIN)
	$(PDFLATEX) $(MAIN).tex
	$(PDFLATEX) $(MAIN).tex

biber:
	$(BIBER) $(MAIN)

clean:
	rm -f $(AUX_FILES) \
	      $(MAIN).aux \
	      $(MAIN).bbl \
	      $(MAIN).bcf \
	      $(MAIN).blg \
	      $(MAIN).fdb_latexmk \
	      $(MAIN).fls \
	      $(MAIN).lof \
	      $(MAIN).log \
	      $(MAIN).lot \
	      $(MAIN).out \
	      $(MAIN).run.xml \
	      $(MAIN).synctex.gz \
	      $(MAIN).toc

distclean: clean
	rm -f $(MAIN).pdf

help:
	@printf '%s\n' \
	  'Targets:' \
	  '  make            Compile PDF from TeX with bibliography' \
	  '  make pdf        Compile PDF from TeX with bibliography' \
	  '  make biber      Compile bibliography for the document' \
	  '  make clean      Remove LaTeX build artifacts' \
	  '  make distclean  Remove build artifacts and main.pdf'
