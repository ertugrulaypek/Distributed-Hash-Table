compile:
	@echo "Compiling server..."
	g++ HashTable.cpp Server.cpp -pthread -lrt -o server
	@echo "Compiling client..."
	g++ Client.cpp -pthread -lrt -o client
	@echo "Compilation successful!"
	