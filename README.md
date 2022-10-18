# MinecraftSLP
The program uses the SLP protocol to retrieve information about the status of a server:
Example of status: {"previewsChat":false,"enforcesSecureChat":false,"description":{"text":"A Minecraft Server"},"players":{"max":20,"online":0},"version":{"name":"Paper 1.19.2","protocol":760}}

To make the program: make MinecraftSLP or gcc MinecraftSLP -o MinecraftSLP
Usage : ./MinecraftSLP ip port
Example : ./MinecraftSLP 127.0.0.1 25565

Everything is based on this page on wiki.vg : https://wiki.vg/Server_List_Ping#Current_.281.7.2B.29
