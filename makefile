project2_src:
	gcc project2_src.cpp -lpthread -lstdc++ -o project2.out
run:
	gcc project2_src.cpp -lpthread -lstdc++ -o project2.out && ./project2.out -p 0.7 -s 10 -t 0