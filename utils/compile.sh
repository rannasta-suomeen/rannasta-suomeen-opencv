cd vendor/custom-opencv-lib \
&& clang++ $(pkg-config opencv4 --cflags) $(pkg-config opencv4 --libs) *.cpp -o custom-lib.o \
&& cd ../../