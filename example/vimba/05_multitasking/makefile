
CXX			= g++
CXXFLAGS	= -g -Wall -pthread	
INCDIR		= -I/home/facet/Vimba_6_0/ -I/home/facet/Vimba_6_0/VimbaCPP/Examples/

LDFLAGS 	= -L/home/facet/Vimba_6_0/VimbaCPP/DynamicLib/arm_64bit/
LDLIBS 		= -lVimbaCPP -lusb-1.0

vimbaserver: vimba.o main.o 
	$(CXX) $(CXXFLAGS) $(INCDIR) $(LDFLAGS) -o vimbaserver  vimba.o  main.o  $(LDLIBS)


vimba.o:			vimba.cc	vimba.h
	$(CXX) $(INCDIR) $(CXXLAGS)	-c vimba.cc


main.o:				main.cc		vimba.h		queue.h
	$(CXX) $(INCDIR)  $(CXXLAGS) -c main.cc


clean:
	rm vimba.o -f
	rm main.o -f
	rm vimbaserver  -f
