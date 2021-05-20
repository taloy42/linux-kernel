#include"message_slot.h"
#include <fcntl.h>      /* open */ 
#include <unistd.h>     /* exit */
#include <sys/ioctl.h>  /* ioctl */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
    int file_desc;
    int ret_val;
    char *path;
    char buff[129];
    unsigned int channel_id;
    if (argc!=3)
    {
        perror("incorrect num of arguments\n");
        exit(1);
    }

    path = argv[1];
    channel_id = (unsigned int) atoi(argv[2]);
    
    file_desc = open(path, O_RDONLY);
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

    ret_val = read(file_desc, buff, 128);
    if (ret_val < 0)
    {
        perror("error while reading from device.\n");
        exit(1);
    }
    ret_val = write(STDOUT_FILENO, buff, ret_val);
    if (ret_val < 0)
    {
        perror("error while writing to standard output.\n");
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