# @configure-input@

analysisdir ?= ../../analysis
tooldir := $(root)/src/fromplsa
idtooldir := $(root)/src/line-ids

# where the original data, processed by plsa analysis can be found
datadir ?= ..

# where the results of the plsa analysis can be found
resultsdir ?= ../best

MATLAB = matlab -nodisplay
XSLTPROC = xsltproc --path $(tooldir)/toxml --path . --path $(analysisdir) --path $(root)/src
time := /usr/bin/time

xml: results.mat summary.html aspects.xml
	$(time) $(MATLAB) -r "path('$(tooldir)', path); makeBaseFiles('$<', 'all'); quit"
	$(MAKE) aspectspredictiveruns aspectspredictivefeatures aspectsprobablefeatures

runsinfo.mat: $(datadir)/runsinfo.mat
	cp $< $@

results.mat: $(resultsdir)/results.mat
	$(time) $(MATLAB) -r "path('$(tooldir)', path); postProcess('$<', '$@'); quit ()"

summary.xml:
	$(time) $(MATLAB) -r "path('$(tooldir)/toxml', path); summarize('$@', '$(analysisdir)/src'); quit()"

aspects.xml: results.mat
	$(time) $(MATLAB) -r "path('$(tooldir)/toxml', path); summarizeAspects('$<', '$@'); quit()"

features.xml: runsinfo.mat
	$(time) $(MATLAB) -r "path('$(tooldir)/toxml', path); summarizeFeatures('$<', '$@'); quit()"

runs.xml: runsinfo.mat
	$(time) $(MATLAB) -r "path('$(tooldir)/toxml', path); summarizeRuns('$<', '$(analysisdir)/$<', '$@'); quit()"

predictive_features.xml: results.mat
	$(time) $(MATLAB) -r "path('$(tooldir)', path); path('$(tooldir)/toxml', path); mostPredictiveFeatures('$<', '$@'); quit()"

probable_features.xml: results.mat
	$(time) $(MATLAB) -r "path('$(tooldir)/toxml', path); mostProbableFeatures('$<', '$@'); quit()"

predictive_runs.xml: results.mat runsinfo.mat
	$(time) $(MATLAB) -r "path('$(tooldir)', path); path('$(tooldir)/toxml', path); mostPredictiveRuns('results.mat', 'runsinfo.mat','$@'); quit()"

summary.html: runs.xml aspects.xml features.xml summary.xml
	$(XSLTPROC) -o $@ --novalid summary.xsl summary.xml

aspect_%_probable_features.html : aspect_%.base probable_features.xml
	$(XSLTPROC) -o $@ --stringparam index `cat $<` probable_features.xsl probable_features.xml

aspect_%_predictive_features.html : aspect_%.base predictive_features.xml
	$(XSLTPROC) -o $@ --stringparam index `cat $<` predictive_features.xsl predictive_features.xml

aspect_%_predictive_runs.html : aspect_%.base predictive_runs.xml
	$(XSLTPROC) -o $@ --stringparam index `cat $<` predictive_runs.xsl predictive_runs.xml

aspectsprobablefeatures: $(addsuffix _probable_features.html, $(basename $(wildcard aspect*.base)))
.PHONY: aspectsprobablefeatures

aspectspredictivefeatures: $(addsuffix _predictive_features.html, $(basename $(wildcard aspect*.base)))
.PHONY: aspectspredictivefeatures

aspectspredictiveruns: $(addsuffix _predictive_runs.html, $(basename $(wildcard aspect*.base)))
.PHONY: aspectspredictiveruns

.PHONY : clean
clean:
	-rm *.html *.png *.xml *.mat *.base *.txt
	-rm -Rf *_src
