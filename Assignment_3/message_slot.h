#ifndef MSGSLOT_H
#define MSGSLOT_H

#include <linux/ioctl.h>

// The major device number.
// We don't rely on dynamic registration
// any more. We want ioctls to know this
// number at compile time.
#define MAJOR_NUM 240

// Set the message of the device driver
#define IOCTL_SET_ENC _IOW(MAJOR_NUM, 0, unsigned long)
#define MSG_SLOT_CHANNEL _IOR(MAJOR_NUM, 0, unsigned int)

#define DEVICE_NAME "message_slot"
#define BUF_LEN 80
#define SUCCESS 0

#define CHANNELS_SIZE 256

// typedef struct Node {
//   char *data;
//   struct Node *next;
// } Node;

// typedef struct LinkedList
// {
//   Node *head;
//   Node *tail;
//   int size;
// } LinkedList;
#define ALOC_ERR -1
#define CH_EXISTS -2
#define CH_NOT_EXISTS -3


#endif
