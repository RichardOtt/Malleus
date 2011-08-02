### Makefile for sigex

CPP = g++
#FLAGS  = -pg -g -g3 -ggdb -fcheck-new -Wall -rdynamic -fno-inline -O\
	 -Wno-sign-compare -ftrapv -fexceptions -fnon-call-exceptions
FLAGS  = -O3 -Wall -Wno-sign-compare
LFLAGS = $(FLAGS)

OBJ= $(SRC:.cxx=.o)
DRAWOBJ= $(DRAWSRC:.cxx=.o)
METAOBJ= $(METASRC:.cxx=.o)

SRC = main.cxx MCMC.cxx PdfParent.cxx Pdf1D.cxx Pdf3D.cxx Sys.cxx Flux.cxx Bkgd.cxx ConfigFile.cxx Errors.cxx Tools.cxx RealFunction.cxx
#SRC = testBkgdCopy.cxx Errors.cxx Tools.cxx
#SRC = testFill.cxx
DRAWSRC = drawResults.cxx MCMC.cxx PdfParent.cxx Pdf1D.cxx Pdf3D.cxx Sys.cxx\
	 Flux.cxx Bkgd.cxx ConfigFile.cxx Errors.cxx Tools.cxx RealFunction.cxx
METASRC = metaConfig.cxx Errors.cxx Tools.cxx

EXE = ./rottSigEx.exe
#EXE = ./test.exe
DRAWEXE = ./drawResults.exe
METAEXE = ./metaConfig.exe

INCS = -I/usr/include/ -I$(shell root-config --incdir)

# -lCore -lRint
LIBS =  $(shell root-config --libs)
#	-L/usr/lib -lgsl -lgslcblas


all: $(EXE) $(DRAWEXE) $(METAEXE) autoFit.exe getAutoCorr.exe

main: $(EXE)

drawing: $(DRAWEXE)

metaconfig: $(METAEXE)

autocorr: getAutoCorr.exe

autofit: autoFit.exe

getAutoCorr.exe: ./getAutoCorr.C
	$(CPP) $(FLAGS) $(INCS) $(LIBS) getAutoCorr.C -o getAutoCorr.exe

autoFit.exe: ./autoFit.C
	$(CPP) $(FLAGS) $(INCS) $(LIBS) autoFit.C -o autoFit.exe

$(OBJ):  %.o: %.cxx Makefile
	$(CPP) $(FLAGS) -c $(INCS) -o $@ $<

$(EXE): $(OBJ)
	$(CPP) $(LFLAGS) $(INCS) $(LIBS) -o $@ $(OBJ)

drawResults.o:  drawResults.cxx Makefile
	$(CPP) $(FLAGS) -c $(INCS) -o $@ $<

$(DRAWEXE): $(DRAWOBJ)
	$(CPP) $(LFLAGS) $(INCS) $(LIBS) -o $@ $(DRAWOBJ)

metaConfig.o: metaConfig.cxx Makefile
	$(CPP) $(FLAGS) -c $(INCS) -o $@ $<

$(METAEXE): $(METAOBJ)
	$(CPP) $(LFLAGS) $(INCS) $(LIBS) -o $@ $(METAOBJ)

depend : $(SRC)
	makedepend -- $(INCS) -- $(SRC)

clean: 
	rm -f $(OBJ) $(DRAWOBJ) $(METAOBJ) $(EXE) $(DRAWEXE) $(METAEXE) autoFit.exe getAutoCorr.exe *~ *.bak


