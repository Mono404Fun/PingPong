cls
g++ -o app main.cpp rsc/app_res.o -Os -g -s -lwinmm -lgdi32 -mwindows -std=c++23
app.exe
