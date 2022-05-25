
CXX			= g++
CXXFLAGS	= -g -Wall -pthread	
INCDIR		= -I/home/facet/Vimba_6_0/ -I/home/facet/Vimba_6_0/VimbaCPP/Examples/

LDFLAGS 	= -L/home/facet/Vimba_6_0/VimbaCPP/DynamicLib/arm_64bit/
LDLIBS 		= -lVimbaCPP -lusb-1.0


vimbaserver: main.o camera_thread.o  vimba.o  state_machine.o tools.o  pugixml.o
	$(CXX) $(CXXFLAGS) $(INCDIR) $(LDFLAGS) -o vimbaserver  main.o vimba.o  camera_thread.o state_machine.o tools.o pugixml.o $(LDLIBS)

vimba.o:			vimba.cc	vimba.h
	$(CXX) $(INCDIR) $(CXXLAGS)	-c vimba.cc

state_machine.o:	state_machine.cc
	$(CXX) $(INCDIR)  $(CXXLAGS) -c state_machine.cc

camera_thread.o:	camera_thread.cc		vimba.h		queue.h		server.h	camserver.h
	$(CXX) $(INCDIR)  $(CXXLAGS) -c camera_thread.cc

tools.o:				tools.cc		tools.h
	$(CXX) $(INCDIR)  $(CXXLAGS) -c tools.cc

main.o:				main.cc		vimba.h		queue.h		server.h	ctr_server.h
	$(CXX) $(INCDIR)  $(CXXLAGS) -c main.cc

pugixml.o:			pugixml.cpp
	$(CXX) $(INCDIR) $(CXXLAGS) -c pugixml.cpp

clean:
	rm usb.o -f
	rm vimba.o -f
	rm vimba_capture.o -f
	rm camserver.o -f
	rm main.o -f
	rm state_machine.o -f
	rm vimbaserver  -f
	rm camera_thread.o -f
	rm pugixml.o -f
