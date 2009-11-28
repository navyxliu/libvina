all: paper
paper: *.tex ../*.bib
	pdflatex bare_conf.tex
	bibtex bare_conf.aux
	pdflatex bare_conf.tex
	pdflatex bare_conf.tex 
draft: *.tex ../*.bib
	cp bare_conf.tex bare_conf.tex.backup
	sed -in 's/conference/draft,onecolumn/g' bare_conf.tex
	-make paper -s
	mv bare_conf.pdf draft-`date +%y_%m_%d`.pdf
	mv bare_conf.tex.backup bare_conf.tex
