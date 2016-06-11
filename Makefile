markdowns:= $(addsuffix .html, $(basename $(wildcard *.md)))

.PHONY : all
all : $(markdowns)

%.html : %.md
	markdown $< > $@
