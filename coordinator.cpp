/*  
 * Atlas Challenge 
 * Server side - coordinator.cpp
 * Marco Silva
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <vector>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros
#include <bits/stdc++.h>

#define PORT 8080
#define TRUE 1
#define FALSE 0
#define MAX_CLIENTS 30
#define TOP_VAL 50
#define BOTTOM_VAL 1
#define CARD_SIZE 10

class Coordinator{
	private:
        int server_socket, cli_socket, opt=TRUE, client_sockets[MAX_CLIENTS]={}, 
			sd, max_sd, activity, i, addrlen = sizeof(addr), value_read, rnd_num, inGame=FALSE;
		struct sockaddr_in addr;
        char buffer[1024] = {0};
		fd_set readfds;
		char *msg = "Connected";
		int cardsReceived=0, players=0;
		std::vector<int> possible_nums; // Vector with all numbers from BOTTOM_VAL to TOP_VAL 

		// Create TCP socket file descriptor
		void createSocket(){
			if((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0){
				perror("Socker creation");
				exit(EXIT_FAILURE);
			}
			
			// Set socket to allow multiple connections
			if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))){
				perror("setsockopt");
				exit(EXIT_FAILURE);
			}
			
			// Define type of socket created
			addr.sin_family = AF_INET;
			addr.sin_addr.s_addr = INADDR_ANY;
			addr.sin_port = htons(PORT);
			printf("Socket Created\n");
		}

		// Bind socket to address and port
		void bindSocket(){
			if (bind(server_socket, (struct sockaddr *) &addr, sizeof(addr))<0){
				perror("Socket bind");
				exit(EXIT_FAILURE);
			}
			printf("Socket bind\n");
		}

		// Specify maximum pending connections on server socket
		void serverListen(int max){
			printf("Listener on port %d\n", PORT);
			if (listen(server_socket, max) < 0){
				perror("Too many pending connections");
				exit(EXIT_FAILURE);
			}

			// Ready to accept incoming connections
			puts("Waiting for connections...");
		}

		// Accept incoming connections
		void acceptConnections(){
			while (TRUE){
				// Clear socket set
				FD_ZERO(&readfds);

				// Add server socket to set
				FD_SET(server_socket, &readfds);
				max_sd = server_socket;

				// Add child sockets to set 
				for(i = 0; i < MAX_CLIENTS; i++){
					sd = client_sockets[i];

					if(sd > 0)	// Valid socket descriptor, add to set
						FD_SET(sd, &readfds);

					if(sd > max_sd)
						max_sd = sd;
				}

				// Wait indefinitely for activity in one of the sockets
				activity = select(max_sd+1, &readfds, NULL, NULL, NULL);
				if(activity <0 && errno!=EINTR)
					perror("Select failed");

				// IF something happens in server socket
				if(FD_ISSET(server_socket, &readfds)){
					if(!inGame){
						if((cli_socket = accept(server_socket, (struct sockaddr *)&addr, (socklen_t *)&addrlen))<0){
							perror("Accept connection failed");
							exit(EXIT_FAILURE);
						}
						printf("New player connected, socket fd: %d, ip: %s, port: %d\n", cli_socket, inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));

						if(send(cli_socket, msg, 1024, 0) != 1024)
							perror("Failed to send message");
						
						// Increase number of active players
						players++;

						for(i = 0; i < MAX_CLIENTS; i++){
							if(client_sockets[i] == 0){
								client_sockets[i] = cli_socket;
								printf("Client socket added to array\n");
								break;
							}
						}
					}
					else
						perror("Game with this coordinator already on progress\n");
				}

				// ELSE some operation on some other socket
				for(i = 0; i < MAX_CLIENTS; i++){
					sd = client_sockets[i];
					if(FD_ISSET(sd, &readfds)){
						
						// Read incoming message and check if it was for closing
						if((value_read = read(sd, buffer, 1024)) == 0){
							disconnectPlayer(sd);
						}

						// Player sends message
						else{
							// Only start sending numbers after all players received their card
							if(cardsReceived == players && strcmp(buffer, "Won") != 0){
								inGame = TRUE;
								// Keep extracting balls until one player wins
								sendNumber();
							}

							// If received message "Game" start new game
							if(strcmp(buffer, "Game") == 0){
								printf("Start game");
								startGame();
							}
							// One player won the game, end it
							else if(strcmp(buffer, "Won") == 0){
								printf("Player in socket %d won!\n", sd);
								endGame();
							}
							// Player received his card
							else if(strcmp(buffer, "Received") == 0){
								cardsReceived++;
								if(cardsReceived == players){
									inGame = TRUE;
									sendNumber();
								}
							}
							else{
								printf("Player in socket %d %s\n", sd, buffer);
							}
						}

					}
				}
			}
			
		}

		void disconnectPlayer(int sd){
			// Get details of disconnected socket
			getpeername(sd, (struct sockaddr*)&addr, (socklen_t*)&addrlen);
			printf("Player disconnected, ip: %s, port: %d\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
			players--;
			// Close socket and free array space
			close(sd);
			client_sockets[i] = 0;
		}

		// Start game with current players until one wins
		void startGame(){
			// Send random card to all players
			std::vector<int> tmp = randomCard();
			for(i=0;i<MAX_CLIENTS;i++){
				sd = client_sockets[i];
				if(sd != server_socket && sd != 0){		// Iterate through all connected sockets except server
					for(int v: tmp){
						send(sd, std::to_string(v).c_str(), 1024, 0);	// Send each value from the card
						printf("Sent number: %s\n", std::to_string(v).c_str());
					}
				}
			}
		}

		void endGame(){
			// Send End message to all players
			msg = "END";
			for(i=0;i<MAX_CLIENTS;i++){
				sd = client_sockets[i];
				if(sd != server_socket && sd != 0){	
					send(sd, msg, 1024, 0);	// Inform all players that one won
				}
			}
		}

		void sendNumber(){
			// Send random number to all players
			i = rand()%possible_nums.size();
			rnd_num = possible_nums.at(i);
			possible_nums.erase(possible_nums.begin()+i);
			for(i=0;i<MAX_CLIENTS;i++){
				sd = client_sockets[i];
				if(sd != server_socket && sd != 0){ // Iterate through all connected sockets except server
					send(sd, std::to_string(rnd_num).c_str(), 1024, 0);	// Send extracted value
				}
			}
			printf("Number %d\n", rnd_num);
			sleep(1);	// Wait 1 second before sending another number
		}

		// Helper functions
		std::vector<int> randomCard(){
			// Shuffle vector with all possible numbers
			std::random_shuffle(possible_nums.begin(), possible_nums.end());
			// Return the first ones until CARD_SIZE
			return {possible_nums.begin(), possible_nums.begin()+CARD_SIZE};
		}

	public:
		Coordinator(){
			printf("Launch coordinator\n");
			// Initialize vector
			for(i=BOTTOM_VAL; i<TOP_VAL; i++)
				possible_nums.push_back(i);
			createSocket();
			bindSocket();
			serverListen(5);
			acceptConnections();
		}

		void reset(){
			// Reset possible numbers
			for(i=BOTTOM_VAL; i<TOP_VAL; i++)
				possible_nums.push_back(i);
			
			// Disconnect all players
			for(i=0;i<MAX_CLIENTS;i++)
				disconnectPlayer(client_sockets[i]);
			
			inGame=FALSE;
			buffer[1024] = {0};
			msg = "Connected";
		 	cardsReceived=0;
		}
};

int main(int argc, char const *argv[])
{
	Coordinator();
	return 0;
}
