texs := $(wildcard *.tex *.sty *.bib)
shots := first-time
epss := $(shots:=.eps)
pngs := $(shots:=.png)

all: master.pdf master.ps

master.pdf: %.pdf: %.dvi $(texs) $(pngs)
	pdflatex $*
	thumbpdf $*
	pdflatex $*

master.ps: %.ps: %.dvi $(shots:=.eps)
	dvips -o$@ -Pgeneric $*

master.dvi: %.dvi: $(texs) $(epss)
	latex $*
	bibtex $*
	latex $*
	latex $*

$(epss): %.eps: %.png
	convert $< $@

clean:
	rm -f *.aux *.bbl *.blg *.dvi *.eps *.log *.out *.pdf *.ps *.tpt
