LATEX = latex -t letter
PDFLATEX = pdflatex
DVIPS = dvips -P cmz -t letter -Z
MAIN = bare_conf
SOURCES = source/*
all: $(MAIN).pdf

.SUFFIXES: .tex .dvi .ps .pdf .rtf

$(MAIN).pdf: *.tex ../*.bib $(SOURCES)
	$(PDFLATEX) $(MAIN).tex
	bibtex $(MAIN).aux
	$(PDFLATEX) $(MAIN).tex
	$(PDFLATEX) $(MAIN).tex 

draft: *.tex ../*.bib $(SOURCES)
	cp bare_conf.tex bare_conf.tex.backup
	sed -in 's/conference/draft,onecolumn/g' bare_conf.tex
	-make paper -s
	mv bare_conf.pdf draft-`date +%y_%m_%d`.pdf
	mv bare_conf.tex.backup bare_conf.tex

tgz:
	tar cvfz vina-`date +%y%m%d`.tar.gz *.tex *.bib README Makefile sourc

clean:
	rm -f *.log *.dvi *.ps *.pdf *~ *.bak *.toc *.lof *.aux *.bbl *.blg *.log *.lot
