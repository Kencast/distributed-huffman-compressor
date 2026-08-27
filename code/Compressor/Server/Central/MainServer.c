// Central server - orchestrates distributed Huffman compression across worker nodes
#include "util.h"
#include "FileChunker/FileChunker.h"
#include "RouteFinder.h"
#include "FileCompressor/FileCompressor.h"

// Global data structures shared across all worker threads
int dto[MAX_SIZE*2] = {};                     // Stores Huffman codes: length and bitmask pairs
client_info info[MAX_CLIENT];                 // Connection info for each worker client
ll freqTable[MAX_SIZE];                       // Frequency count for each byte value (0-255)
mutex keys[MAX_SIZE];                         // Mutex locks for thread-safe frequency updates
mutex matex = PTHREAD_MUTEX_INITIALIZER;      // Main synchronization mutex

int main() {
    // Initialize socket and network variables
    int server_socket, client_socket, n, cont = 0;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    // Get number of worker servers to expect
    printf("Insert the number of client servers: ");
    scanf("%d", &n);

    // Create TCP socket for listening to incoming connections
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Error creating socket");
        exit(1);
    }

    // Configure server socket to accept connections on all interfaces
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind socket to port
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Error while binding");
        exit(1);
    }

    // Start listening for incoming connections
    if (listen(server_socket, 5) == -1) {
        perror("Error while listening");
        exit(1);
    }

    // Accept incoming connections from worker nodes
    printf("Central server listening on port: %d...\n", PORT);
    while (cont < n) {
        info[cont].socket = accept(server_socket, (struct sockaddr*)&info[cont].client_addr, &info[cont].client_len);
        if (info[cont].socket == -1) {
            perror("Error in accept");
            continue;
        }
        info[cont].index = cont;
        printf("Client connected from %s:%d!\n", 
        inet_ntoa(info[cont].client_addr.sin_addr), 
        ntohs(info[cont].client_addr.sin_port));
        cont++;
    }

    // Distribute file chunks to all worker servers for frequency calculation
    if (splitFile(createSplitDTO(FILE_PATH, info, n)) == false) {
        perror("Error splitting the file from Main");
        return 1;
    }

    // Generate Huffman codes from the aggregated frequency table
    route** routes = routeFinder(freqTable);
    
    // Pack routes into DTO array: dto[i] = length, dto[i+1] = bitmask
    for (int i = 0, j = 0; i < MAX_SIZE*2; i += 2, ++j) {
        if (routes[j] != NULL) {
            dto[i] = routes[j]->len;
            dto[i + 1] = routes[j]->msk;
        } else {
            dto[i] = 0;
            dto[i + 1] = 0;
        }
    }

    // Trigger compression on all worker nodes using the generated Huffman codes
    if (compressFile(n, info) == false) {
        perror("Error creating threads from main for compression");
        return 1;
    }
    
    // Merge compressed file chunks from all workers into single output file
    FILE* fileC = fopen(PATH_FOR_COMPRESS, "wb");
    int byte = 0;
    int bit = 7;
    uchar buffer[BUFFER_SIZE];
    uchar cbuffer[BUFFER_SIZE];

    // Process each worker's compressed output file
    for (int i = 0; i < n; i++) {
        char ruta[29];
        if (snprintf(ruta, sizeof(ruta), "%s%d", SAVED_FILE_ROUTE, i) >= sizeof(ruta)) {
            printf("Error calculating the route for %d\n", i);
            return 1;
        }
        FILE* file = fopen(ruta, "rb");
        
        size_t bytes_read;

        // Read file size (excluding last 2 bytes containing padding info)
        fseek(file, 0, SEEK_END);
        ll file_size = ftell(file) - 2;
        fseek(file, 0, SEEK_SET);        
        printf("File compress size %lld\n", file_size + 2);
        ll pos;

        // Process complete bytes from the compressed file
        while (file_size > 0) {
            pos = 0;
            size_t bytes_to_read = (file_size < BUFFER_SIZE) ? file_size : BUFFER_SIZE;
            bytes_read = fread(buffer, 1, bytes_to_read, file);

            // Bit-by-bit merging: accumulate bits from multiple files
            for (ll k = 0; k < bytes_read; k++) {
                for (int bt = 7; bt >= 0; bt--) {
                    if (TEST(buffer[k], bt)) byte |= (1 << bit);
                    bit--;
                    if (bit == -1) {
                        cbuffer[pos++] = byte;
                        byte = 0;
                        bit = 7;
                    }
                }
            }
            fwrite(cbuffer, 1, pos, fileC);
            file_size -= bytes_read;
        }

        // Read padding information (last 2 bytes contain partial byte and padding count)
        fseek(file, -2, SEEK_END);
        uchar ceros, cantCeros;
        fread(&ceros, sizeof(uchar), 1, file); 
        fread(&cantCeros, sizeof(uchar), 1, file);
        printf("Bits: %d Total: %d for file %d\n", ceros, cantCeros, i);

        // Process padding bits (remaining bits in the last byte)
        for (int j = 7; j >= 0 && cantCeros; j--, cantCeros--) {
            if (TEST(ceros, j)) byte |= (1 << bit);
            bit--;
            if (bit == -1) {
                fwrite(&byte, 1, 1, fileC);
                byte = 0;
                bit = 7;
            }
        }

        fclose(file);

        // Clean up worker's temporary compressed file
        if (remove(ruta) != 0) {
            fprintf(stderr, "Error deleting file %d\n", i);
        }
    }

    // Write final padding byte and padding count to mark end of compressed data
    buffer[0] = byte;
    buffer[1] = 7 - bit;
    printf("Final byte: %d total: %d\n", buffer[0], buffer[1]);
    fwrite(buffer, 1, 2, fileC);

    // Verify final file size
    fseek(fileC, 0, SEEK_END);
    ll file_size = ftell(fileC);
    fseek(fileC, 0, SEEK_SET); 
    fclose(fileC);
    printf("Written compressed file size %lld\n", file_size);

    printf("File successfully compressed\n");

    // Write Huffman code table to file for decompression
    FILE* fileDef = fopen(PATH_FOR_TABLE, "wb");
    if (fileDef == NULL) {
        perror("Error saving the code table");
        return 1;
    }

    // Save each code's length and bitmask
    for (int c = 0, j; c < 2 * MAX_SIZE; c += 2, j++) {
        fprintf(fileDef, "%d %d\n", dto[c], (int)dto[c + 1]);
    }

    fclose(fileDef);
    close(server_socket);
    return 0;
}
