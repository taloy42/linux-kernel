#include"message_slot.h"
#include <fcntl.h>      /* open */ 
#include <unistd.h>     /* exit */
#include <sys/ioctl.h>  /* ioctl */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

int main(int argc, char **argv)
{
    int file_desc;
    int ret_val;
    char *path, *msg;
    unsigned int channel_id;
    if (argc!=4)
    {
        perror("incorrect num of arguments\n");
        exit(1);
    }

    path = argv[1];
    msg = argv[3];
    channel_id = (unsigned int) atoi(argv[2]);
    
    file_desc = open(path, O_WRONLY);
    if (file_desc < 0)
    {
        perror("cannot open message slot\n");
        exit(1);
    }
    
    ret_val = ioctl(file_desc, MSG_SLOT_CHANNEL, channel_id);
    if (ret_val < 0)
    {
        perror("error while changing channel ID\n");
        exit(1);
    }

    ret_val = write(file_desc, msg, strlen(msg));
    if (ret_val < 0)
    {
        perror("error while writing to device.\n");
        exit(1);
    }

    ret_val = close(file_desc);
    if (ret_val < 0)
    {
        perror("error while closing device.\n");
        exit(1);
    }

    exit(0);
}