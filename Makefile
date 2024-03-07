all:
	g++ tinytetris-commented.cpp -o tinytetris-commented -lncurses

clean:
	rm -f tinytetris tinytetris-commented
