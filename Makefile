fix-parser: parser.cpp
	$(CXX) -std=c++11 -Wall parser.cpp -o fix-parser
	./fix-parser
	@python ./normalize.py

plot:
	gnuplot plot.gnu

clean:
	rm -f fix-parser times-* plot.gnu
