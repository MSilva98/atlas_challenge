/*  
 * Atlas Challenge 
 * Client side - player.cpp
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
#include <thread>
#include <algorithm>

#define PORT 8080
#define SERVER_IP "127.0.0.1"
#define TRUE 1
#define FALSE 0
#define CARD_SIZE 10

class Player{
	private:
		int sock, value_read, inGame=FALSE;
		struct sockaddr_in serv_addr;
		char buffer[1024] = {0};
		char *msg = "Game";
		std::vector<int> card;

		// Create TCP socket file descriptor
		void createSocket(){
			if((sock = socket(AF_INET, SOCK_STREAM, 0)) == 0){
				perror("Socker creation failed");
				exit(EXIT_FAILURE);
			}

			// Define type of socket created
			serv_addr.sin_family = AF_INET;
			serv_addr.sin_port = htons(PORT);

			// Convert address from text to binary
			if(inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr)<=0)
				printf("Invalid address/Address not supported\n");

			printf("Socket Created\n");
		}

		// Connect player to server
		void connectSocket(){
			if (connect(sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr))<0){
				perror("Connection Failed");
				exit(EXIT_FAILURE);
			}
			printf("Socket Connected\n");
		}

		// Receive card and acknowledge reception
		void readCard(){
			printf("Receiving card... \n");
			for(int i = 0; i < CARD_SIZE; i++){
				read(sock, buffer, 1024);
				printf("Number %s\n", buffer);
				card.push_back(atoi(buffer));
			}
			msg = "Received";
			send(sock, msg, 1024, 0);
			inGame = TRUE;
			printf("Received card\n");
			game();
		}

		// Play game until one player wins
		void game(){
			int v;
			char msg_tmp[1024];
			while(inGame){
				read(sock, buffer, 1024);
				v = atoi(buffer);
				// Find and remove number from card
				if(std::find(card.begin(), card.end(), v) != card.end()){
					card.erase(std::remove(card.begin(), card.end(), v), card.end());
					if(card.size() > 0){
						sprintf(msg_tmp, "has %d. Remain %d numbers.", v, card.size());
						printf("Player %d: Number %d is in card\n", sock, v);
					}
					else{
						sprintf(msg_tmp, "Won");
						inGame = FALSE;
						printf("Player %d: I won!\n", sock);
					}
				}
				else {
					sprintf(msg_tmp, "does not have %d. Remain %d numbers.",v , card.size());
					printf("Player %d: Number %d is not in card\n", sock, v);
				}
				send(sock, msg_tmp, 1024, 0);
			}
			terminate();
		}

		void terminate(){
			close(sock);
			printf("Player %d: Goodbye!\n", sock);
			exit(EXIT_SUCCESS);
		}

	public:
		Player(){
			createSocket();
			connectSocket();
		}
		
		// Send "Game" message to ask for card
		void startGame(){
			value_read = read(sock, buffer, 1024);
			if(strcmp(buffer, "Connected") == 0){
				send(sock, msg, 1024, 0);
			}
			printf("Sent \"Game\" message\n");
			readCard();
		}	
};


int main(int argc, char const *argv[])
{
	// Player p1, p2, p3, p4, p5, p6, p7, p8;
	int num_players = 10;
	
	std::vector<std::thread> threads;
	for(int i = 0; i < num_players; i++){
		threads.push_back(std::thread(&Player::startGame, Player()));
	}

	for(auto& thread: threads)
		thread.join();
	
	return 0;
}
