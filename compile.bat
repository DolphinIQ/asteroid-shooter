@REM Compilation only:
@REM gcc -Wall -g3 -I ./src/raylib/include main.c -L ./src/raylib/lib -lraylib -lgdi32 -lwinmm -o dist/app

@REM Compile & Run:
@REM gcc -Wall -g3 -I ./src/raylib/include main.c -L ./src/raylib/lib -lraylib -lgdi32 -lwinmm -o dist/app | ./dist/app


gcc -Wall -g3 -I ./src/raylib/include main.c -L ./src/raylib/lib^
 -lraylib -lgdi32 -lwinmm -o dist/app

@REM gcc -Wall -g3 -I ./src/raylib/include main.c -L ./src/raylib/lib -lraylib -lgdi32 -lwinmm -o dist/app

start "" dist/app.exe
