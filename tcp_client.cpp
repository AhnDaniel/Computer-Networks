/*
 * tcp_client.cpp
 *
 *  Created on: Apr 15, 2017
 *      Author: Daniel
 */


#include <sdkddkver.h>
#include <conio.h>
#include <stdio.h>

#include <WinSock2.h>
#include <Windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <ctype.h>

#define SCK_VERSION2 0x0202
#define _WINSOCK_DEPRECATED_NO_WARNINGS

using namespace std;

// function prototypes
void recvFileInfo(SOCKET, int&, int&);
void bytesRecvd(int&, int, int);
int menu();

int main(){
	// *****************************************LOOKED UP HOW TO OPEN A TCP SOCKET
	// Winsock Startup
	WSAData wsaData;		WORD DllVersion = MAKEWORD(2,1);
	if(WSAStartup(DllVersion, &wsaData) != 0){
		MessageBoxA(NULL, "Winsock startup failed", "Error", MB_OK | MB_ICONERROR);
		exit(1);
	}

	SOCKADDR_IN addr;			// address to be binded to our connection socket
	int sizeofaddr = sizeof(addr);	// need sizeofaddr for the connect function
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");	// address = local host (this pc)
	addr.sin_port = htons(10000); // port = 1234, same as server
	addr.sin_family = AF_INET; 	//IPv4 socket

	SOCKET Connection = socket(AF_INET, SOCK_STREAM, 0);	// set connection socket
	if(connect(Connection, (SOCKADDR*)&addr, sizeofaddr) != 0){	// if we are unable to connect...
		MessageBoxA(NULL, "Failed to Connect", "Error", MB_OK | MB_ICONERROR);
		return 0;	// failed to connect
	}

	cout << "Connected!\n";
	char Message[256];
	recv(Connection, Message, sizeof(Message), 0);
	cout << Message << endl;

	// EVERYTHING AFTER THIS IS MY OWN CODE@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

	// define variables
	int userInput = 2;
	int ack[1];

	string filename;
	bool fileExists;
	char fileReq[256];
	int fileExistence[1];
	ofstream outFile;

	int fileSize = 0;
	int bufferSize = 0;
	int bytesWritten;
	int remainingBytes = 0;
	int numBytesExpected;
	int packNum;
	int len;

	do{
		do{
			// initialize important variables
			bytesWritten = 0;
			fileExists = false;
			len = 0;

			cout << "What file would you like to request from the server via TCP? ";
			cin >> filename;

			// send the file name over to the server to see if file exists

			strcpy(fileReq, filename.c_str());
			send(Connection, fileReq, sizeof(fileReq), 0);

			// receive response of validity of file from the server
			recv(Connection, (char*)&fileExistence, sizeof(fileExistence), 0);

			// receive ack if file does NOT exist
			if(fileExistence[0] == -1){
				cout << "The file you requested from the server does not exist.\n"
						"Request another file.\n\n";
			}
			else{
				cout << "File exists in the server. Incoming file.\n\n";
				fileExists = true;
				// open output file to write to
				outFile.open(filename, ios::binary | ios::ate);
			}
		}while(fileExists == false);

		// check if we can open a file to write to.
		if(!outFile.is_open()){
			cout << "Unable to open \"" << filename << "\" for writing.\n";
			exit(-1);
		}

		// receive important file info:
		recvFileInfo(Connection, fileSize, bufferSize);

		char buffer[bufferSize];
		packNum = 1;

		// start receiving packets
		numBytesExpected = 0;
		while(bytesWritten < fileSize){					// once BytesWritten == fileSize, the client knows that all of the file has been sent
			remainingBytes = fileSize - bytesWritten;

			len = recv(Connection, buffer, bufferSize, 0);				// receive message

			numBytesExpected = len;
			cout << "\n\nExpecting byte/sequence number: " << bytesWritten + 1 << endl;

			// error checking
			if(remainingBytes >= bufferSize){
				if(numBytesExpected != bufferSize){
					cout << "Error. Message was not received properly. Asking server to resend packet.\n";
					ack[0] = 1;
					send(Connection, (char*)&ack, sizeof(int), 0);
				}
				else{
					cout << "Packet successfully received. Sending ACK.";
					outFile.write(buffer, bufferSize);
					bytesWritten += len;
					ack[0] = bytesWritten + 1;
					send(Connection, (char*)&ack, sizeof(int), 0);
				}
			}
			else{
				if(numBytesExpected != remainingBytes){
					cout << "Error. Message was not received properly. Asking server to resend packet.\n";
					ack[0] = 1;
					send(Connection, (char*)&ack, sizeof(int), 0);
				}
				else{
					cout << "Packet successfully received. Sending ACK.";
					outFile.write(buffer, remainingBytes);
					bytesWritten += len;
					ack[0] = bytesWritten + 1;
					send(Connection, (char*)&ack, sizeof(int), 0);
				}
			}

			bytesRecvd(packNum, len, bytesWritten);
		}

		outFile.close();

		cout << "File has successfully been received.\n\n";

		// does the user want to request another file or close the socket?
		userInput = menu();

		// if user wishes to request another file:
		if(userInput == 1){			// user wishes to request another file
			ack[0] = 0;
			send(Connection, (char*)&ack, sizeof(int), 0);
		}

	}while(userInput == 1);

	// this part of the program only executes if the client wishes to
	// close the socket between the client and server
	cout << "\nNow attempting to close the socket connection...\n\n";

	ack[0] = 1;			// sending/receiving ACK's
		send(Connection, (char*)&ack, sizeof(int), 0);
		recv(Connection, (char*)&ack, sizeof(int), 0);
		send(Connection, (char*)&ack, sizeof(int), 0);

	cout << "Socket connection closed between client and server.";
	return 0;

}

// receive important file info from server
void recvFileInfo(SOCKET s, int &fileSize, int &bufferSize){
	// START RECEIVING THE IMPORTANT MESSAGES FROM THE SERVER
	// Message 1: receive size of file from server
	recv(s, (char*)&fileSize, sizeof(int), 0);
	cout << "\nSize of file: " << fileSize << endl;

	// Message 2: bufferSize
	recv(s, (char*)&bufferSize, sizeof(int), 0);
	cout << "Size of the buffer: " << bufferSize << endl;
}

// displays the received packet number, bytes received, and bytes written
void bytesRecvd(int &packNum, int amountOnBuffer, int bytesWritten){
	cout << "\nReceived packet number: " << packNum << endl
		 << "Bytes received: " << amountOnBuffer << endl
		 << "Bytes Written: " << bytesWritten << endl;

	packNum += 1;
}

// menu function to see if user/client wants to request another file or close the socket
int menu(){
	string input;	// user input
	double userInput = 0;

	bool correctInput = false;
	cin.ignore();

	// input error checking
	/*
	 * The conditions for proper user input are:
	 * 1. It MUST only be a one-character input
	 * 2. The input MUST be either 1 or 2
	 * 3. The input CANNOT be in double form, such as 1.0 or 2.0, because it is only expecting a one-character input
	 */
	while(correctInput == false){
		cout << "\nWould you like to request another file or close the connection?\n"
				"1. Request another file\n"
				"2. Close connection socket\n\n";

		getline(cin, input);

		stringstream myStream(input);
		if((myStream >> userInput) && (userInput == 1 || userInput == 2))
			correctInput = true;
		else
			cout << "\n\nInvalid option. Please enter \"1\" or \"2\".\n";
	}

	return static_cast<int>(userInput);
}
