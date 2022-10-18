#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <time.h>

uint64_t pack_varint(uint32_t number);
uint64_t unpack_varint(int *sock, int *valread);

uint8_t bytes_used(uint32_t number);
void insert_bytes_in_data(uint64_t dataByte, uint8_t **data, uint32_t *ptr);
void insert_string_in_data(char **ip, uint32_t serverAddressLength, uint8_t **data, uint32_t *ptr);

uint8_t main(int argc, char **argv){
    if (argc != 3){
        printf("Usage : ./MinecraftSLP ip port");
        return 0;
    }

    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);

    //Handshake request:
    //Varint: request length in bytes   (example : 1 + 1 + 9 + 2 + 1 = 14)
    //Varint: packetID                  (example : 0)  
    //Varint: protocol Version          (example : pack_varint(760))       
    //Varint: Server address length     (example : 9)
    //String: Server address            (example : "127.0.0.1")
    //uint16_t: Server Port             (example : 25565)
    //Varint: next state                (example : 0)

    uint32_t protocolVersionVarint = pack_varint(760);
    uint8_t packetID = 0;
    uint32_t serverAddressLength = pack_varint(strlen(ip));
    uint8_t nextState = pack_varint(1);
    uint32_t requestLength = bytes_used(packetID)
                            + bytes_used(protocolVersionVarint)
                            + bytes_used(serverAddressLength) 
                            + serverAddressLength
                            + 2 //Port uses 2 bytes 
                            + 1; //nextState is either 0 or 1
    uint32_t totalRequestLength = pack_varint(requestLength + bytes_used(requestLength));
    uint8_t *data = malloc(totalRequestLength);
    uint32_t ptr = 0;

    insert_bytes_in_data(requestLength, &data, &ptr);
    insert_bytes_in_data(packetID, &data, &ptr);
    insert_bytes_in_data(protocolVersionVarint, &data, &ptr);
    insert_bytes_in_data(serverAddressLength, &data, &ptr);
    insert_string_in_data(&ip, serverAddressLength, &data, &ptr);
    insert_bytes_in_data(port, &data, &ptr);
    insert_bytes_in_data(nextState, &data, &ptr);
    
    //The data packet is constructed, it is ready to be sent
    uint8_t statusRequestPacket[2] = {1, 0};
    //statusRequestPacket is constructed



    //Initializing socket
    int sock = 0, valread, client_fd;
    struct sockaddr_in serv_addr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }
 
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
 
    // Convert IPv4 and IPv6 addresses from text to binary
    if (inet_pton(AF_INET, ip, &serv_addr.sin_addr)
        <= 0) {
        printf(
            "\nInvalid address/ Address not supported \n");
        return -1;
    }
 
    if ((client_fd
         = connect(sock, (struct sockaddr*)&serv_addr,
                   sizeof(serv_addr)))
        < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }

    //Sending the data
    send(sock, data, totalRequestLength, 0); //Sends the data to the server
    send(sock, statusRequestPacket, 2, 0);   //Sends the status request packet 

    // The server will now send information, we need to read it.
    unpack_varint(&sock, &valread); //Total packet length, not needed
    uint8_t tmp;
    valread = read(sock, &tmp, 1);  //PacketID, not needed
    uint64_t stringLength = unpack_varint(&sock, &valread);
    
    char *buffer = malloc(stringLength);
    valread = read(sock, buffer, stringLength);
    printf("%s\n", buffer);

    // closing the connected socket
    close(client_fd);

    free(data);
    free(buffer);
}

uint64_t pack_varint(uint32_t number){
    /* Function to transform integers into variable size integer. Maximum size in effective bytes is 5.
    More details : https://wiki.vg/Protocol#VarInt_and_VarLong */

    uint64_t varint = 0;
    while (1){
        varint <<= 8;

        uint8_t tmp = number & 0x7F; //0b01111111
        number >>= 7;

        varint += tmp;
        if (number != 0){
            varint |= 0x80; //0b10000000
        }else break;
    }
    return varint;
}

uint64_t unpack_varint(int *sock, int *valread){
    uint64_t unpackedVarint = 0;
    uint8_t tmp = 0x80;
    uint8_t i = 0;
    
    while(tmp & 0x80){ //While the varint indicates that the next byte is part of the varint
        *valread = read(*sock, &tmp, 1);
        unpackedVarint |= (tmp & 0x7F) << (7 * i);
        i++;
    }
    return unpackedVarint;
}

uint8_t bytes_used(uint32_t number){ 
    /* function to find how many bytes aren't 0 in a function 
    (example 0x0000abd8 uses only the last two bytes) */

    if (number == 0){
        return 1;   //If number = 0 it still uses 1 byte
    }
    for (uint8_t i = 4 ; i > 0 ; i--){
        if (number & 0xFF000000){
            return i;
        }else{
            number <<= 8;
        }
    }
}

void insert_bytes_in_data(uint64_t dataByte, uint8_t **data, uint32_t *ptr){
    /* Inserts bytes one at a time in the data variable */
    for (int64_t i = bytes_used(dataByte) - 1 ; i >= 0 ; i--){

        *(*data + *ptr) = (dataByte >> (i * 8)) & (0xFF); //Selects the right byte to put in data
        (*ptr)++;
    }
}

void insert_string_in_data(char **ip, uint32_t serverAddressLength, uint8_t **data, uint32_t *ptr){
    /* Inserts a string into the data variable */
    for (uint32_t i = 0 ; i < serverAddressLength ; i++){
        *(*data + *ptr) = *(*ip + i);
        (*ptr)++;
    }
}