all: paper
paper: *.tex *.bib
	pdflatex bare_conf.tex
	bibtex bare_conf.aux
	pdflatex bare_conf.tex
	pdflatex bare_conf.tex 

