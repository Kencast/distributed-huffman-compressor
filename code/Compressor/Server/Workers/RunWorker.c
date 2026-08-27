// Worker node - compresses assigned file chunk using Huffman codes from central server
#include "ByteCounter.h"
#include "FileCompressor/FileCompressor.h"

int main() {
    // Create socket for TCP connection to central server
    int server_socket;
    struct sockaddr_in server_addr;

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Error creating socket");
        exit(1);
    }

    // Resolve central server hostname/IP address
    struct hostent *server = gethostbyname(IP);
    if (server == NULL) {
        fprintf(stderr, "Error: cannot resolve hostname\n");
        return 1;
    }

    // Configure connection to central server
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(NGROK_PORT);
    memcpy(&server_addr.sin_addr.s_addr, server->h_addr_list[0], server->h_length);
    
    // Connect to the central server
    if (connect(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Error connecting to central server");
        exit(1);
    }

    // Step 1: Receive file chunk from central server
    printf("Connected. Receiving file...\n");
    if (receive_file(server_socket) == false) {
        perror("Error receiving the file");
    }

    // Step 2: Count byte frequencies in the assigned file chunk
    if (calc_frequency(server_socket) == false) {
        perror("Error calculating frequencies");
    }
    
    // Step 3: Receive Huffman code table from central server
    // codes array format: codes[2*i] = code length, codes[2*i+1] = bitmask
    int codes[2 * MAX_SIZE];

    if (recv(server_socket, codes, 2 * MAX_SIZE * sizeof(int), 0) == -1) {
        perror("Error receiving the code tables");
        return 1;
    }
    
    // Step 4: Compress the file chunk using received Huffman codes
    if (compress(codes, server_socket) == false) {
        perror("Error compressing the file");
        return 1;
    }

    // Step 5: Receive acknowledgment from central server (optional completion signal)
    int ack;
    printf("Going to receive the ack\n");

    int p = recv(server_socket, &ack, sizeof(ack), 0);
    printf("The received ack was: %d\n", p);
    
    // Close connection and cleanup
    close(server_socket);
    return 0;
}
