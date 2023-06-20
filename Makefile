DVDRIP_CPPFLAGS=${CPPFLAGS} -g -O2 -Wall -Wno-multichar
DVDRIP_LDFLAGS=${LDFLAGS} -lbe -ltracker -ldvdread -ldvdcss
TARGETS=DVDRip.o DVDRipApp.o DVDRipWin.o DVDRipView.o DVDRipFilesView.o DVDRipTitlesView.o DVDRipWrapper.o

default: DVDRip

DVDRip: ${TARGETS} DVDRip.rsrc
	gcc ${TARGETS} -o DVDRip ${DVDRIP_LDFLAGS} 
	xres -o DVDRip DVDRip.rsrc

%.o: %.cpp
	gcc -c $< -o $@ ${DVDRIP_CPPFLAGS}

clean:
	rm -f DVDRip *.o

strip:
	strip DVDRip
	xres -o DVDRip DVDRip.rsrc
