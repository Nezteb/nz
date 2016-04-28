all: nzProx nzServ

nzProx:
	g++ nzProx.cpp -o nzProx.out

nzServ:
	g++ nzServ.cpp -o nzServ.out

clean:
	rm -f *.out *.log
