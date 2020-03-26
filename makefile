

all:
	cc  drc_test.c `pkg-config cray-drc --cflags --libs` -o drc_test.exe

clean:
	rm *.exe
