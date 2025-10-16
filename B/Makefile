
all:
	g++ -std=c++17 -I include/ -I src/include/  -pthread src/server.cpp   -o bin/server
	g++ -std=c++17 -I include/ -I src/include/ -pthread src/client.cpp -o bin/client
	g++ -std=c++17 -I include/ -I src/include/ -pthread src/lb.cpp -o bin/lb

clean:
	rm -f bin/server bin/client bin/simple_client




