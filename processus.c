#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <signal.h>

void sendCANMessage(int socket, struct sockaddr_can addr);
void receiveCANMessage(int socket);

int main(int argc, char **argv) 
{
    int fdSocketCAN;
    struct sockaddr_can addr;
    struct ifreq ifr;
    pid_t pid;

    // Création du socket CAN
    if ((fdSocketCAN = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0)
    {
        perror("Socket");
        return -1;
    }

    // Choix de l'interface CAN
    if (argc == 2)
        strcpy(ifr.ifr_name, argv[1]);
    else
        strcpy(ifr.ifr_name, "vcan0");

    ioctl(fdSocketCAN, SIOCGIFINDEX, &ifr);

    // Liaison du socket à l'interface CAN
    memset(&addr, 0, sizeof(addr));
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(fdSocketCAN, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("Bind");
        return -1;
    }


      // Application des filtres pour les ID CAN 0x325 et 0x662
    struct can_filter rfilter[2];
    rfilter[0].can_id = 0x325;  // ID de filtre
    rfilter[0].can_mask = 0xFFF;  // Masque de filtre
    rfilter[1].can_id = 0x662;  // ID de filtre
    rfilter[1].can_mask = 0xFFF;  // Masque de filtre

	

    // if (setsockopt(fdSocketCAN, SOL_CAN_RAW, CAN_RAW_FILTER, rfilter, sizeof(rfilter)) < 0)
    // {
    //     perror("Setsockopt");
    //     close(fdSocketCAN);
    //     return -1;
    // }

    // Application des filtres au socket CAN
    setsockopt(fdSocketCAN, SOL_CAN_RAW, CAN_RAW_FILTER, &rfilter, sizeof(rfilter));

    // Création d'un processus fils
    pid = fork();
    if (pid == 0)
    {
        // Processus fils : réception de messages CAN
        while (1)
        {
            receiveCANMessage(fdSocketCAN);
        }
    } 
    else 
    {
        // Processus père : envoi de messages CAN
        while (1) 
        {
            int choix;
            printf("Menu :\n");
            printf("1. Envoyer un message CAN\n");
            printf("2. Quitter\n");
            printf("Choix : ");
            scanf("%d", &choix);

            if (choix == 1)
            {
                sendCANMessage(fdSocketCAN, addr);
            } 
            else if (choix == 2) 
            {
                kill(pid, SIGTERM);  // Terminer le processus fils
                close(fdSocketCAN);
                break;
            }
        }
    }
    return 0;
}

void sendCANMessage(int socket, struct sockaddr_can addr)
{
    struct can_frame frame;

    // Remplissage de la trame CAN
    frame.can_id = 0x115;  // Identifiant CAN (3 derniers chiffres de DA)
    frame.can_dlc = 5;
    memset(frame.data, 0, sizeof(frame.data));
    strncpy((char *)frame.data, "salut", frame.can_dlc);  // Définir les données comme "salut"

    if (write(socket, &frame, sizeof(struct can_frame)) != sizeof(struct can_frame)) 
    {
        perror("Write");
    } 
    else 
    {
        printf("Message envoyé : ID = %03X  Data = %s \n", frame.can_id, frame.data);
    }
}

void receiveCANMessage(int socket) 
{
    struct can_frame frame;
    int nbytes;


    // Lecture des données CAN
    nbytes = read(socket, &frame, sizeof(struct can_frame));
    if (nbytes < 0) 
    {
        perror("Read");
    }
    else 
    {
        printf("\nMessage reçu : 0x%03X [%d] ", frame.can_id, frame.can_dlc);
        for (int i = 0; i < frame.can_dlc; i++) 
        {
            printf("%02X ", frame.data[i]);
        }
        printf("\n");
    }
}