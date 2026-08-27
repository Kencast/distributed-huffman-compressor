// Huffman decompressor - reconstructs original file using Huffman tree and compressed data
#include "util.h"

int main() {
    // Open and read the Huffman code table from file
    FILE* huffman_table = fopen(DESCOMPRESS_TABLE, "rb");
    if (huffman_table == NULL) {
        perror("Error opening the Huffman table file");
        return 1;
    }

    // Create root node for the Huffman tree used during decompression
    node* root = createNode('\0');
    ll greatest=0;
    
    // Rebuild the Huffman tree from the code table
    // For each of 256 byte values, read the code length and bitmask
    for (int i = 0; i < 256; i++) {
        fscanf(huffman_table, "%d %d\n", &lenTable[i], &pathTable[i]);
        if (lenTable[i]) {
            // Track the longest code for validation purposes
            if(pathTable[i] > greatest) greatest = pathTable[i];
            // Insert this byte's code into the tree structure
            insert(0, lenTable[i], pathTable[i], i, root);
        }
    }
    
    fclose(huffman_table);
    printf("Largest path %lld\n", greatest);

    // Open the compressed file for reading
    FILE* fileRead = fopen(COMPRESSED_FILE, "rb");
    if (fileRead == NULL) {
        perror("Error opening the compressed file");
        return 1;
    }

    // Open/create the output file for decompressed data
    FILE* fileWrite = fopen(PATH_FOR_DESCOMPRESS, "wb");
    if (fileWrite == NULL) {
        perror("Error opening the decompressed file");
        fclose(fileRead);
        return 1;
    }

    // Calculate file size (excluding last 2 bytes which contain padding metadata)
    fseek(fileRead, 0, SEEK_END);
    ll file_size = ftell(fileRead) - 2;
    fseek(fileRead, 0, SEEK_SET);

    ll pos;
    node* path = root;                          // Current position in Huffman tree during decompression
    uchar buffer[BUFFER_SIZE];                  // Buffer for compressed data
    uchar wbuffer[BUFFER_SIZE];                 // Buffer for decompressed output
    
    // Validate file size and tree structure
    if (file_size < 0) {
        perror("Error in the size of the file to decompress");
        fclose(fileRead);
        fclose(fileWrite);
        return 1;
    }

    if (root == NULL || (root->left == NULL && root->right == NULL)) {
        perror("Invalid Huffman tree");
        fclose(fileRead);
        fclose(fileWrite);
        return 1;
    }

    // Process main body of compressed file (excluding padding byte)
    while (file_size > 0) {
        pos = 0;
        size_t bytes_to_read = (file_size < BUFFER_SIZE) ? file_size : BUFFER_SIZE;
        size_t bytes_read = fread(buffer, 1, bytes_to_read, fileRead);

        // Decompress byte-by-byte, then bit-by-bit
        for (ll i = 0; i < (ll)bytes_read; i++) {
            uchar byteRead = buffer[i];
            
            // Process each bit in the byte (MSB to LSB)
            for (int j = 7; j >= 0; j--) {

                // Safety check: ensure we haven't lost our position in the tree
                if (path == NULL) {
                    perror("Path node is null");
                    fclose(fileRead);
                    fclose(fileWrite);
                    return 1;
                }

                // Traverse tree: bit 1 = right child, bit 0 = left child
                if (TEST(byteRead, j)) {
                    if (path->right == NULL) {
                        perror("Right child is null during traversal");
                        fclose(fileRead);
                        fclose(fileWrite);
                        return 1;
                    }
                    path = path->right;
                } else {
                    if (path->left == NULL) {
                        perror("Left child is null during traversal");
                        fclose(fileRead);
                        fclose(fileWrite);
                        return 1;
                    }
                    path = path->left;
                }

                // Reached a leaf node: output the decoded byte and reset to root
                if (path->left == NULL && path->right == NULL) {
                    wbuffer[pos] = path->sym;
                    path = root;
                    pos++;
                }
            }
        }

        // Write decoded bytes to output file
        fwrite(wbuffer, 1, pos, fileWrite);
        file_size -= bytes_read;
    }

    // Read padding metadata (last 2 bytes of compressed file)
    // padding: contains remaining bits of last partial byte
    // totalPadding: number of bits in padding byte that are valid
    uchar padding = 0, totalPadding = 0;
    fseek(fileRead, -2, SEEK_END);
    fread(&padding, sizeof(uchar), 1, fileRead); 
    fread(&totalPadding, sizeof(uchar), 1, fileRead);
    fclose(fileRead);

    // Decompress the padding bits (remaining bits in the final byte)
    pos = 0;
    for (int j = 7; j >= 0 && totalPadding; j--, totalPadding--) {
        if (path == NULL) {
            perror("Path node is null in padding");
            fclose(fileWrite);
            return 1;
        }
        
        // Traverse tree using padding bits
        if (TEST(padding, j)) {
            if (path->right == NULL) {
                perror("Right child is null during padding");
                fclose(fileWrite);
                return 1;
            }
            path = path->right;
        } else {
            if (path->left == NULL) {
                perror("Left child is null during padding");
                fclose(fileWrite);
                return 1;
            }
            path = path->left;
        }
        
        // Output decoded byte when leaf is reached
        if (path->left == NULL && path->right == NULL) {
            wbuffer[pos] = path->sym;
            pos++;
            path = root;
        }
    }
    
    // Verify consistency: after processing all bits, we should be back at root
    // (any other state indicates a corrupted file)
    if (path != root) {
        perror("Error inconsistent file");
        fclose(fileWrite);
        return 1;
    }
    
    // Write final decoded bytes if any
    if (pos) fwrite(wbuffer, 1, pos, fileWrite);

    fclose(fileWrite);
    printf("File decompressed successfully\n");
    return 0;
}
