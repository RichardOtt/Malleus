### Makefile for sigex

CPP = g++
#FLAGS  = -pg -g -g3 -ggdb -fcheck-new -Wall -rdynamic -fno-inline -O\
	 -Wno-sign-compare -ftrapv -fexceptions -fnon-call-exceptions
FLAGS  = -O3 -Wall -Wno-sign-compare
LFLAGS = $(FLAGS)

OBJ= $(SRC:src/%.cxx=obj/%.o)
DRAWOBJ= $(DRAWSRC:src/%.cxx=obj/%.o)
METAOBJ= $(METASRC:src/%.cxx=obj/%.o)
GENERATEMAPOBJ= $(GENERATEMAPSRC:src/%.cxx=obj/%.o)

SRC = main.cxx MCMC.cxx PdfParent.cxx Pdf1D.cxx Pdf3D.cxx Sys.cxx Flux.cxx\
	 Bkgd.cxx ConfigFile.cxx Errors.cxx Tools.cxx RealFunction.cxx\
	 Decider.cxx metaReader.cxx
DRAWSRC = drawResults.cxx MCMC.cxx PdfParent.cxx Pdf1D.cxx Pdf3D.cxx Sys.cxx\
	 Flux.cxx Bkgd.cxx ConfigFile.cxx Errors.cxx\
	 Tools.cxx RealFunction.cxx Decider.cxx
METASRC = metaConfig.cxx metaReader.cxx Errors.cxx Tools.cxx
GENERATEMAPSRC = generateMap.cxx Errors.cxx Tools.cxx

SRC := $(addprefix src/,$(SRC))
DRAWSRC := $(addprefix src/,$(DRAWSRC))
METASRC := $(addprefix src/,$(METASRC))
GENERATEMAPSRC := $(addprefix src/,$(GENERATEMAPSRC))

EXE = bin/Malleus
#EXE = ./test.exe
DRAWEXE = bin/drawResults.exe
METAEXE = bin/metaConfig.exe
GENERATEMAPEXE = bin/generateMap.exe
AUTOCORREXE = bin/getAutoCorr.exe
AUTOFITEXE = bin/autoFit.exe

INCS = -I/usr/include/ -I$(shell root-config --incdir)

# -lCore -lRint
LIBS =  $(shell root-config --libs)
#	-L/usr/lib -lgsl -lgslcblas


all: subdir $(EXE) $(DRAWEXE) $(METAEXE) $(AUTOCORREXE) $(AUTOFITEXE)

.PHONY : subdir
subdir: 
	-mkdir -p bin
	-mkdir -p obj

main: $(EXE)

drawing: $(DRAWEXE)

metaconfig: $(METAEXE)

generatemap: $(GENERATEMAPEXE)

autocorr: $(AUTOCORREXE)

autofit: $(AUTOFITEXE)

$(AUTOCORREXE): src/getAutoCorr.C
	$(CPP) $(FLAGS) $(INCS) $(LIBS) src/getAutoCorr.C -o $(AUTOCORREXE)

$(AUTOFITEXE): src/autoFit.C
	$(CPP) $(FLAGS) $(INCS) $(LIBS) src/autoFit.C -o $(AUTOFITEXE)


$(OBJ):  obj/%.o: src/%.cxx Makefile
	$(CPP) $(FLAGS) -c $(INCS) -o $@ $<

$(EXE): $(OBJ) $(GENERATEMAPEXE)
	$(CPP) $(LFLAGS) $(INCS) $(LIBS) -o $@ $(OBJ)

obj/drawResults.o:  src/drawResults.cxx Makefile
	$(CPP) $(FLAGS) -c $(INCS) -o $@ $<

$(DRAWEXE): $(DRAWOBJ)
	$(CPP) $(LFLAGS) $(INCS) $(LIBS) -o $@ $(DRAWOBJ)

obj/metaConfig.o: src/metaConfig.cxx src/metaReader.cxx src/metaReader.h\
                  Makefile
	$(CPP) $(FLAGS) -c $(INCS) -o $@ $<

#metaReader.o: metaReader.cxx metaReader.h Makefile
#	$(CPP) $(FLAGS) -c $(INCS) -o $@ $<

obj/generateMap.o: src/generateMap.cxx Makefile
	$(CPP) $(FLAGS) -c $(INCS) -o $@ $<

src/Decider.cxx: $(GENERATEMAPEXE) src/FunctionDefs.h
	bin/generateMap.exe src/FunctionDefs.h > src/Decider.cxx

$(METAEXE): $(METAOBJ)
	$(CPP) $(LFLAGS) $(INCS) $(LIBS) -o $@ $(METAOBJ)

$(GENERATEMAPEXE): $(GENERATEMAPOBJ)
	$(CPP) $(LFLAGS) $(INCS) $(LIBS) -o $@ $(GENERATEMAPOBJ)

depend : $(SRC)
	makedepend -- $(INCS) -- $(SRC)

.PHONY : clean
clean: 
	rm -f $(OBJ) $(DRAWOBJ) $(METAOBJ) $(GENERATEMAPOBJ) $(EXE) $(DRAWEXE)\
 $(METAEXE) $(GENERATEMAPEXE) $(AUTOFITEXE) $(AUTOCORREXE) src/Decider.cxx\
 *~ *.bak


