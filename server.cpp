//==============================================================================
// You can change the username and password for the VIP user by editing the
// VIP_USER and VIP_PASS below
// -----------------------------------------------------------------------------
// Group:
// Dylan Cross - 15219491
// Nate Fort   - 15195444
// Tom Sloman  - 15078758
// -----------------------------------------------------------------------------
// Exit reasons
// 0 - No errors, successful run
// 1 - Unable to instantiate WSADATA
// 2 - getaddrinfo failure during Start-up
// 3 - Failed to initialize socket
// 4 - Failed to bind socket
// 5 - getnameinfo failed during client connection
// 6 - getaddrinfo failure in EPRT command
// -----------------------------------------------------------------------------
// NOTE: Data transfers assume ASCII encoding and won't work as intended with other ways of encoding
//==============================================================================
#define USE_IPV6 true
#define BUFFER_SIZE 500
#define _WIN32_WINNT 0x0A00 // win 10? allows use of getaddrinfo which requires 0x501 or up : https://msdn.microsoft.com/en-us/library/6sehtctf.aspx
#define WSVERS MAKEWORD(2,2) // Use the MAKEWORD(lowbyte, highbyte) macro declared in Windef.h
#define VIP_USER "nhreyes"
#define VIP_PASS "334"

#include <winsock2.h>
#include <ws2tcpip.h> // required by getaddrinfo()
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <direct.h> // used for _chdir, _rmdir, _mkdir _getcwd
#include <string.h>

WSADATA wsadata; //Create a WSADATA object called wsadata.

/*
 * Found here:
 * https://blog.kowalczyk.info/article/8h/Check-if-file-exists-on-Windows.html
 * Return TRUE if file 'fileName' exists
 */
bool FileExists(const TCHAR *fileName)
{
    DWORD fileAttr;
    fileAttr = GetFileAttributes(fileName);
    if (0xFFFFFFFF == fileAttr) return false;
    return true;
}
//******************************************************************************
// MAIN
//******************************************************************************
int main(int argc, char *argv[]) {
//******************************************************************************
// INITIALIZATION
//******************************************************************************
	// Attempt to instantiate with the winsock library
	int err = WSAStartup(WSVERS, &wsadata);
	if (err != 0) {
		 WSACleanup();
	 	// Tell the user that we could not find a usable WinsockDLL
		 printf("WSAStartup failed with error: %d\n", err);
		 exit(1);
	}
	printf("\n========================================================================\n");
	printf("       159.334 FTP Server          ");
	printf("\n========================================================================\n");
	// Global variable declaration
	struct addrinfo hints, *result = NULL;
	struct sockaddr_storage localaddr, remoteaddr; // IPV6-compatible
	struct sockaddr_in local_data_addr4; // IPV4 only (only used in Port)
	char clientHost[NI_MAXHOST];
	char clientService[NI_MAXSERV];
	char port[6];
	SOCKET s,ns, ns_data, s_data_act;
	char send_buffer[BUFFER_SIZE], receive_buffer[BUFFER_SIZE];
	int active = 0;
	int n, bytes, addrlen;

	ns_data = INVALID_SOCKET;
	memset(&hints, 0, sizeof(struct addrinfo));
	memset(&localaddr, 0, sizeof(localaddr)); //clean up the structure
	memset(&remoteaddr, 0, sizeof(remoteaddr)); //clean up the structure

	if (USE_IPV6) { hints.ai_family = AF_INET6; }
	else { hints.ai_family = AF_INET; }
	if (argc == 2) { strcpy(port, argv[1]); }
	else { strcpy(port, "1234\0"); } // NOTE default port if not provided when running the program

	if (getaddrinfo(NULL, port, &hints, &result) != 0) { // getaddrinfo returns 0 if it worked
		printf("getaddrinfo failed");
		exit(2);
	}
	//****************************************************************************
	//SOCKET
	//****************************************************************************
	s = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (s < 0) {
		printf("Socket initialization failed\n");
		exit(3);
	}
	//****************************************************************************
	// BIND
	//****************************************************************************
	if (bind(s, result->ai_addr, result->ai_addrlen) != 0) {
		printf("Bind failed!\n");
		exit(4);
	}
	freeaddrinfo(result);
	//****************************************************************************
	//LISTEN
	//****************************************************************************
	listen(s, 5);
	//****************************************************************************
	//INFINITE LOOP
	//****************************************************************************
	//============================================================================
	bool root_level = true; // used to determine if a user can go up in directories (cd ..)
  while (1) { // Start of MAIN LOOP
		// If server not at root directory, change directory back to root
		if (root_level == false) {
			printf("Switching to root\n");
			_chdir("..");
			root_level = true;
		}
		char user[50];
		char pass[50];
    char filename[BUFFER_SIZE];
		bool vip = false; // determines access level
		memset(clientHost, 0, sizeof(clientHost));
 	  memset(clientService, 0, sizeof(clientService));
 	  addrlen = sizeof(remoteaddr);
		printf("\n------------------------------------------------------------------------\n");
	 	printf("SERVER is waiting for an incoming connection request...");
	 	printf("\n------------------------------------------------------------------------\n");
	 	//**************************************************************************
	 	//NEW SOCKET newsocket = accept  //CONTROL CONNECTION
	 	//**************************************************************************
	 	ns = accept(s, (struct sockaddr *)(&remoteaddr), &addrlen);
	 	if (ns < 0 ) break;
		printf("\n========================================================================\n");
		if (getnameinfo((struct sockaddr *)&remoteaddr, addrlen, clientHost, sizeof(clientHost), clientService, sizeof(clientService), NI_NUMERICHOST) != 0) {
			printf("\nError detected: getnameinfo() failed \n");
			exit(5);
		} else { printf("\nConnected to <<<CLIENT>>> with IP address: %s, at Port: %s\n", clientHost, clientService); }
 	  printf("\n========================================================================\n");
	 	//**************************************************************************
	 	//Respond with welcome message
	 	//**************************************************************************
 	  sprintf(send_buffer,"220 FTP Server ready. \r\n");
 	  bytes = send(ns, send_buffer, strlen(send_buffer), 0);
 	 	//==========================================================================
 	 	//COMMUNICATION LOOP per CLIENT
 	 	//==========================================================================
	  while (1) {
	 		n = 0;
		 	//PROCESS message received from CLIENT
		 	while (1) {
			  //**********************************************************************
			  //RECEIVE
			  //**********************************************************************
		 		bytes = recv(ns, &receive_buffer[n], 1, 0);//receive byte by byte...
			  //**********************************************************************
			  //PROCESS REQUEST
			  //**********************************************************************
		 		if ((bytes < 0) || (bytes == 0)) break;
		 		if (receive_buffer[n] == '\n') { /*end on a LF*/
		 			receive_buffer[n] = '\0';
		 			break;
		 		}
		 		if (receive_buffer[n] != '\r') n++; /*Trim CRs*/
		 	}
			// If bytes was 0 or less then an error occurred
		 	if ((bytes < 0) || (bytes == 0)) break;
		 	printf("<< DEBUG INFO. >>: the message from the CLIENT reads: '%s\\r\\n' \n", receive_buffer);
			// USER Command
			// 331 Registered username, now enter password
		 	if (strncmp(receive_buffer,"USER",4)==0)  {
				if (receive_buffer[5] == '\n' || receive_buffer[5] == '\r') {
					sprintf(user, "Public User\n");
					printf("User connected as %s", user);
					sprintf(send_buffer,"331 Welcome public user, enter any password: \r\n");
				} else {
					strncpy(user, &receive_buffer[5], 54);
					printf("User logging in as: %s", user);
					sprintf(send_buffer,"331 Welcome %s, enter your password \r\n", user);
				}
		 		bytes = send(ns, send_buffer, strlen(send_buffer), 0);
		 		if (bytes < 0) break;
		 	}
			// PASS Command
			// 230 Successful login
		 	if (strncmp(receive_buffer,"PASS",4)==0)  {
				if (strcmp(user, "Public User") == 0) {
					sprintf(send_buffer,"230 Public login sucessful \r\n");
				} else {
					sprintf(pass, &receive_buffer[5], 54);
					if (strncmp(user, VIP_USER, 7) == 0 && strncmp(pass, VIP_PASS, 3) == 0) {
						// Authenticated as VIP
						vip = true;
						sprintf(send_buffer,"230 successfully Authenticated as (VIP) %s \r\n", user);
					} else {
						vip = false;
						sprintf(send_buffer,"230 %s login sucessful \r\n", user);
					}
				}
		 		printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
		 		bytes = send(ns, send_buffer, strlen(send_buffer), 0);
		 		if (bytes < 0) break;
		 	}
			// SYST Command
			// 215 Windows type
		 	if (strncmp(receive_buffer,"SYST",4)==0)  {
		 		printf("Information about the system \n");
		 		sprintf(send_buffer,"215 Windows Type: WIN32\r\n");
		 		printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
		 		bytes = send(ns, send_buffer, strlen(send_buffer), 0);
		 		if (bytes < 0) break;
		 	}
			// OPTS Command
			// WIN 10 OPTS = UTF8 ON
			// 202 not implemented
		 	if (strncmp(receive_buffer,"OPTS",4)==0)  {
		 		printf("unrecognised command \n");
		 		sprintf(send_buffer,"202 Not Implemented (OPTS)\r\n");
		 		printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
		 		bytes = send(ns, send_buffer, strlen(send_buffer), 0);
		 		if (bytes < 0) break;
		 	}
			// QUIT Command
			// 221 Connection closed
		 	if (strncmp(receive_buffer,"QUIT",4)==0)  {
		 		printf("Quit \n");
		 		sprintf(send_buffer,"221 Connection close by client\r\n");
		 		printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
		 		bytes = send(ns, send_buffer, strlen(send_buffer), 0);
		 		if (bytes < 0) break;
		 		closesocket(ns);
		 	}
			// PORT (IPV4) Command
			// IPV4: PORT 127.0.0.1:50149\r\n
			// 501 Syntax errors
			// 425 Can't start connection
			// 200 Connection successful
		 	if(strncmp(receive_buffer, "PORT", 4) == 0) {
		 		s_data_act = socket(AF_INET, SOCK_STREAM, 0);
		 		//local variables
		 		int act_port[2];
		 		int act_ip[4], port_dec;
		 		char ip_decimal[40];
		 		active = 1;//flag for active connection
		 		int scannedItems = sscanf(receive_buffer, "PORT %d,%d,%d,%d,%d,%d",
		 			 &act_ip[0],&act_ip[1],&act_ip[2],&act_ip[3],
		 				&act_port[0],&act_port[1]);
		 		if(scannedItems < 6) {
		 			sprintf(send_buffer,"501 Syntax error in arguments \r\n");
		 		 	printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
		 		 	bytes = send(ns, send_buffer, strlen(send_buffer), 0);
		 			 //if (bytes < 0) break;
		 			break;
		 		}
				sprintf(ip_decimal, "%d.%d.%d.%d", act_ip[0], act_ip[1], act_ip[2],act_ip[3]);
				local_data_addr4.sin_family = AF_INET;
				local_data_addr4.sin_addr.s_addr = inet_addr(ip_decimal);  //ipv4 only
				port_dec = act_port[0];
				port_dec = port_dec << 8;
				port_dec = port_dec + act_port[1];
				local_data_addr4.sin_port = htons(port_dec); //ipv4 only
				printf("\n========================================================================\n");
				printf("    Active FTP mode, the client is listening...    \n");
		 		printf("    CLIENT's IP is %s\n", ip_decimal);
		 		printf("    CLIENT's Port is %d\n",port_dec);
		 		printf("========================================================================\n");
		 		if (connect(s_data_act, (struct sockaddr *)&local_data_addr4, (int) sizeof(struct sockaddr)) != 0) {
		 			sprintf(send_buffer, "425 Something is wrong, can't start active connection... \r\n");
		 			bytes = send(ns, send_buffer, strlen(send_buffer), 0);
		 			printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
		 			closesocket(s_data_act);
		 		}
		 		else {
		 			sprintf(send_buffer, "200 PORT Command successful\r\n");
		 			bytes = send(ns, send_buffer, strlen(send_buffer), 0);
		 			printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
		 			printf("Connected to client\n");
		 		}
		 	}
			// EPRT (IPV6)
			// IPV6: EPRT |2|::1|50149|\r\n
			// 425 Connection error
			// 200 Connection successful
			if  (strncmp(receive_buffer, "EPRT", 4) == 0) {
				s_data_act = socket(AF_INET6, SOCK_STREAM, 0);
		 		//local variables
				char address[129], port[6];
				memset(address, '\0', sizeof(address));
				memset(port, '\0', sizeof(port));
				active = 1; // flag for active connection
				int i = 8; // set i to skip this bit of the receive_buffer "EPRT |2|"

				while (true) {
					if (receive_buffer[i] == '|') { break; }
					i++;
				}
				strncpy(address, &receive_buffer[8], i - 8);
				address[i-7] = '\0'; i++;
				strncpy(port, &receive_buffer[i], 6);
				i = 0;
				while (true) {
					if (port[i] == '|') { port[i] = '\0'; break; }
					i++;
				}
				printf("========================================================================\n");
				printf("    Active FTP mode, the client is listening...\n");
				printf("    CLIENT's IP is %s\n",address);
				printf("    CLIENT's Port is %s\n", port);
				printf("========================================================================\n");

				hints.ai_family = AF_INET6;
				struct addrinfo *results;
				if (getaddrinfo(address, port,  &hints, &results) != 0) {
					printf("Failed to find address");
					exit(6);
				}
		 		if (connect(s_data_act, results->ai_addr, results->ai_addrlen) != 0) {
		 			sprintf(send_buffer, "425 Something is wrong, can't start active connection... \r\n");
					printf("%d\n", WSAGetLastError()); // Prints the error that happened in connecting
		 			bytes = send(ns, send_buffer, strlen(send_buffer), 0);
		 			printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
		 			closesocket(s_data_act);
		 		}
		 		else {
		 			sprintf(send_buffer, "200 EPRT Command successful\r\n");
		 			bytes = send(ns, send_buffer, strlen(send_buffer), 0);
		 			printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
		 			printf("Connected to client\n");
		 		}
				freeaddrinfo(results);
			}
			// LIST Command
		 	// technically, LIST is different than NLST,but we make them the same here
			// 226 File transfer successful
		 	if ((strncmp(receive_buffer, "LIST", 4) == 0) || (strncmp(receive_buffer, "NLST", 4) == 0))   {
		 		//system("ls > tmp.txt");//change that to 'dir', so windows can understand
		 		system("dir > tmp.txt");
		 		FILE *fin=fopen("tmp.txt","r");//open tmp.txt file
		 		//sprintf(send_buffer,"125 Transfering... \r\n");
		 		sprintf(send_buffer,"150 Opening ASCII mode data connection... \r\n");
		 		printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
		 		bytes = send(ns, send_buffer, strlen(send_buffer), 0);
		 		char temp_buffer[BUFFER_SIZE];
		 		while (!feof(fin)){
		 			fgets(temp_buffer,490,fin);
		 			sprintf(send_buffer,"%s",temp_buffer);
		 			if (active==0) send(ns_data, send_buffer, strlen(send_buffer), 0);
		 			else send(s_data_act, send_buffer, strlen(send_buffer), 0);
		 		}
		 		fclose(fin);
		 		sprintf(send_buffer,"226 File transfer complete. \r\n");
		 		printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
		 		bytes = send(ns, send_buffer, strlen(send_buffer), 0);
		 		if (active == 0) closesocket(ns_data);
		 		else closesocket(s_data_act);
		 		system("del tmp.txt");
		 	}
			// RETR Command
			// 226 if the file was successfully transferred
			// 550 for file-does-not-exist, permission-denied, etc.
			if (strncmp(receive_buffer, "RETR", 4) == 0) {
				memset(filename, '\0', sizeof(filename));
				strncpy(filename, &receive_buffer[5], 490);
				printf("Get: %s\n", filename);
				FILE *fin = fopen(filename,"r"); // Open file
				if (fin == NULL) {
					sprintf(send_buffer,"550 File '%s' not found\r\n", filename);
			 		printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
			 		bytes = send(ns, send_buffer, strlen(send_buffer), 0);
				} else { // Send file
			 		sprintf(send_buffer,"150 Opening ASCII mode data connection... \r\n");
			 		printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
			 		bytes = send(ns, send_buffer, strlen(send_buffer), 0);
			 		char temp_buffer[BUFFER_SIZE];
			 		while (!feof(fin)){
			 			fgets(temp_buffer,490,fin);
			 			sprintf(send_buffer,"%s",temp_buffer);
			 			if (active == 0) send(ns_data, send_buffer, strlen(send_buffer), 0);
			 			else send(s_data_act, send_buffer, strlen(send_buffer), 0);
			 		}
					sprintf(send_buffer,"226 File transfer complete. \r\n");
			 		printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
			 		bytes = send(ns, send_buffer, strlen(send_buffer), 0);
				}
		 		fclose(fin); // Close file
		 		if (active == 0) closesocket(ns_data);
		 		else closesocket(s_data_act);
			}
			// STOR Command
			// 226 file successfully transferred
			// 550 file-does-not-exist
			if (strncmp(receive_buffer,"STOR",4) == 0) {
				memset(filename, '\0', sizeof(filename));
				strncpy(filename, &receive_buffer[5], 490);
				// Returns true if file exists
				if (FileExists(filename)) {
					sprintf(send_buffer, "550 file already exists!\r\n");
					printf("<< DEBUG INFO. >>: REPLY sent to client %s\n", send_buffer);
					bytes = send(ns, send_buffer, strlen(send_buffer), 0);
				} else {
					FILE *fin = fopen(filename,"w");
					sprintf(send_buffer, "150 recieve ascii\r\n");
					printf("<< DEBUG INFO. >>: REPLY sent to client %s\n", send_buffer);
					bytes = send(ns, send_buffer, strlen(send_buffer), 0);
					char temp_buffer[BUFFER_SIZE]; // Could be bigger?
					n = 0;
					while (true) {
						if (active == 0) bytes = recv(ns, &temp_buffer[n], 1, 0);
						else bytes = recv(s_data_act, &temp_buffer[n], 1, 0);
						// If bytes == 0 it means its empty/done transferring
						if ((bytes < 0) || (bytes == 0)) break;
						if (temp_buffer[n] != '\r') n++; // Trim CRs
					}
					temp_buffer[n] = '\0';
					if (n != 0) {
						fprintf(fin, "%s", temp_buffer);
						sprintf(send_buffer, "226 File transfer successful\r\n");
						bytes = send(ns, send_buffer, strlen(send_buffer), 0);
						printf("<< DEBUG INFO. >>: REPLY sent to client %s\n", send_buffer);
					} else {
						sprintf(send_buffer, "550 File transfer unsuccessful\r\n");
						bytes = send(ns, send_buffer, strlen(send_buffer), 0);
						printf("<< DEBUG INFO. >>: REPLY sent to client %s\n", send_buffer);
					}
					fclose(fin);
				}
			}
      // Delete file
			// Only enabled for VIP
      // Doesn't check if the file is needed to function so be careful what you delete
			// 250 Successfully deleted file
			// 550 Could not find file, action not allowed
			if (strncmp(receive_buffer, "DELE", 4) == 0) {
				if (!vip) {
					sprintf(send_buffer, "550 need to be authenticated as VIP to delete files\r\n");
					printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
					bytes = send(ns, send_buffer, strlen(send_buffer), 0);
				} else {
					memset(filename, '\0', sizeof(filename));
					strncpy(filename, &receive_buffer[5], 490);
          if (remove(filename) == 0) {
            sprintf(send_buffer, "250 successfully deleted %s\r\n", filename);
  					printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
  					bytes = send(ns, send_buffer, strlen(send_buffer), 0);
          } else {
            sprintf(send_buffer, "550 could not find file %s\r\n", filename);
  					printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
  					bytes = send(ns, send_buffer, strlen(send_buffer), 0);
          }
				}
			}
      // Rename file
      // Requires 2 commands:
      //  - RNFR (Rename) From
      //  - RNTO (Rename) To
			// Only enabled for VIP
      // Doesn't check if the file is needed to function so be careful what you rename
			// 250 Successfully renamed file
      // 350 need filename to rename to
			// 550 Rename unsuccessful/filename(s) already exist
			if (strncmp(receive_buffer, "RNFR", 4) == 0) {
				if (!vip) {
					sprintf(send_buffer, "550 need to be authenticated as VIP to delete files\r\n");
					printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
					bytes = send(ns, send_buffer, strlen(send_buffer), 0);
				} else {
          memset(filename, '\0', sizeof(filename));
					strncpy(filename, &receive_buffer[5], 490);
          if (FileExists(filename)) {
            sprintf(send_buffer, "350 rename %s to:\r\n", filename);
  					printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
  					bytes = send(ns, send_buffer, strlen(send_buffer), 0);
          } else {
            sprintf(send_buffer, "550 could not find file %s\r\n", filename);
  					printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
  					bytes = send(ns, send_buffer, strlen(send_buffer), 0);
          }
				}
			}
      if (strncmp(receive_buffer, "RNTO", 4) == 0) {
				if (!vip) {
					sprintf(send_buffer, "550 need to be authenticated as VIP to delete files\r\n");
					printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
					bytes = send(ns, send_buffer, strlen(send_buffer), 0);
				} else {
          char fileTo[BUFFER_SIZE];
          memset(fileTo, '\0', sizeof(fileTo));
					strncpy(fileTo, &receive_buffer[5], 490);
          if (!FileExists(fileTo)) {
            if (rename(filename, fileTo) == 0) {
              sprintf(send_buffer, "250 renamed %s to %s\r\n", filename, fileTo);
    					printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
    					bytes = send(ns, send_buffer, strlen(send_buffer), 0);
            } else {
              sprintf(send_buffer, "550 failed to rename %s to %s\r\n", filename, fileTo);
              printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
              bytes = send(ns, send_buffer, strlen(send_buffer), 0);
            }
          } else {
            sprintf(send_buffer, "550 %s already exists, cancelling rename\r\n", fileTo);
  					printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
  					bytes = send(ns, send_buffer, strlen(send_buffer), 0);
          }
				}
			}
			// CWD Command
			// 250 = Requested file action okay, completed.
			// 550 = No permission to enter folder (must be logged in as vip to access non public folder)
			// 550 = Requested action not taken. File unavailable (e.g., file not found, no access).
			// Assumes a folder structure of
			// root
			//     -- vip_folder (closed to public)
			//     -- public_folder
			if (strncmp(receive_buffer, "CWD", 3) == 0) {
				char directory[BUFFER_SIZE];
				strncpy(directory, &receive_buffer[4], 490);
				char command[BUFFER_SIZE];
				sprintf(command, "cd %s", directory);
				if (system(command) == 0) {
					// Trying to go up a directory while at root level
					if (strcmp(directory, "..") == 0 && root_level) {
						sprintf(send_buffer, "550 You are at root level of server\r\n");
						bytes = send(ns, send_buffer, strlen(send_buffer), 0);
						printf("<< DEBUG INFO. >>: REPLY sent to client %s\r\n", send_buffer);
					}
					// Go up a directory when not at root directory
					else if (strcmp(directory, "..") == 0 && !root_level) {
						_chdir(directory);
						sprintf(send_buffer, "250 Switched to root\r\n");
						bytes = send(ns, send_buffer, strlen(send_buffer), 0);
						printf("<< DEBUG INFO. >>: REPLY sent to client %s\r\n", send_buffer);
					}
					// If they are not trying to access public_folder and they are not authenticated as VIP, deny
					else if (strcmp(directory, "public_folder") != 0 && !vip) {
						sprintf(send_buffer, "550 No permission to enter %s\r\n", directory);
						bytes = send(ns, send_buffer, strlen(send_buffer), 0);
						printf("<< DEBUG INFO. >>: REPLY sent to client %s\r\n", send_buffer);
					}
					else {
						_chdir(directory);
						root_level = false;
						sprintf(send_buffer, "250 Changed directory to %s\r\n", directory);
						bytes = send(ns, send_buffer, strlen(send_buffer), 0);
						printf("<< DEBUG INFO. >>: REPLY sent to client %s\r\n", send_buffer);
					}
				} else {
					sprintf(send_buffer, "550 Directory Not Found\r\n");
					bytes = send(ns, send_buffer, strlen(send_buffer), 0);
					printf("<< DEBUG INFO. >>: REPLY sent to client %s\r\n", send_buffer);
				}
			}
			// Make Directory command
			// Only enabled for VIP
			// 250 Successfully created directory
			// 550 Action not allowed, or directory already exists
			if (strncmp(receive_buffer, "XMKD", 4) == 0) {
				if (!vip) {
					sprintf(send_buffer, "550 need to be authenticated to make directories\r\n");
					printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
					bytes = send(ns, send_buffer, strlen(send_buffer), 0);
				} else {
					char directory[BUFFER_SIZE];
					strncpy(directory, &receive_buffer[5], 490);
					if (_mkdir(directory) == 0) {
						sprintf(send_buffer, "250 %s created\r\n", directory);
						printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
						bytes = send(ns, send_buffer, strlen(send_buffer), 0);
					} else {
						sprintf(send_buffer, "550 %s already exists\r\n", directory);
						printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
						bytes = send(ns, send_buffer, strlen(send_buffer), 0);
					}
				}
			}
			// Remove Directory command
			// Only enabled for VIP
			// 250 Successfully deleted directory
			// 550 Action not allowed, or directory doesn't exist
			if (strncmp(receive_buffer, "XRMD", 4) == 0) {
				if (!vip) {
					sprintf(send_buffer, "550 need to be authenticated to delete directories\r\n");
					printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
					bytes = send(ns, send_buffer, strlen(send_buffer), 0);
				} else {
					char directory[BUFFER_SIZE];
					strncpy(directory, &receive_buffer[5], 490);
					// Check that user isn't trying to delete essential directories
					if (strncmp(directory, "vip_folder", 10) == 0 || strncmp(directory, "public_folder", 13) == 0) {
						sprintf(send_buffer, "550 this folder is required by the server\r\n");
						printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
						bytes = send(ns, send_buffer, strlen(send_buffer), 0);
					} else if (_rmdir(directory) == 0) {
						sprintf(send_buffer, "250 %s removed\r\n", directory);
						printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
						bytes = send(ns, send_buffer, strlen(send_buffer), 0);
					} else {
						sprintf(send_buffer, "550 %s doesn't exist\r\n", directory);
						printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
						bytes = send(ns, send_buffer, strlen(send_buffer), 0);
					}
				}
			}
      // Print Working Directory (current directory)
			// 250 Successfully deleted directory
			// 550 Action not allowed, or directory doesn't exist
			if (strncmp(receive_buffer, "XPWD", 4) == 0) {
				memset(filename, '\0', sizeof(filename));
        _getcwd(filename, sizeof(filename));
        sprintf(send_buffer, "250 current dirtory: %s\r\n", filename);
        printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
        bytes = send(ns, send_buffer, strlen(send_buffer), 0);
			}
      // Help command
			// 214 Available commands
			if (strncmp(receive_buffer, "HELP", 4) == 0) {
				// Send back implemented commands that the user can input.
				// Not sure what calls SYST on win 10 ftp client
        char commands[100] = "\nuser\t\tSYST\t\tdir\t\tls\t\tget\nput\t\tdelete\t\trename\t\tcd\t\tremotehelp\nmkdir\t\trmdir\t\tpwd\0";
				sprintf(send_buffer, "214 Available server commands: \n%s\r\n", commands);
				printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
				bytes = send(ns, send_buffer, strlen(send_buffer), 0);
			}
			//========================================================================
			//End of COMMUNICATION LOOP per CLIENT
			//========================================================================
		}
	 	//**************************************************************************
	 	//CLOSE SOCKET
	 	//**************************************************************************
	 	closesocket(ns);
	 	//==========================================================================
	} //End of MAIN LOOP
	 	//==========================================================================
	closesocket(s);
	printf("\nSERVER SHUTTING DOWN...\n");
	exit(0);
}
