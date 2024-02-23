spinvu.exe : main.c
	gcc -Wall -Isrc/Include -Lsrc/lib -o spinvu.exe main.c -lmingw32 -lSDL2main -lSDL2 -lSDL2_ttf -lSDL2_image -mwindows
