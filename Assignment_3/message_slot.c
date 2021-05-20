// Declare what kind of code we want
// from the header files. Defining __KERNEL__
// and MODULE allows us to access kernel-level
// code not usually available to userspace programs.
#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE


#include <linux/kernel.h>   /* We're doing kernel work */
#include <linux/module.h>   /* Specifically, a module */
#include <linux/fs.h>       /* for register_chrdev */
#include <linux/uaccess.h>  /* for get_user and put_user */
#include <linux/string.h>   /* for memset. NOTE - not string.h!*/
#include <linux/slab.h>
#include <linux/errno.h>

MODULE_LICENSE("GPL");

//Our custom definitions of IOCTL operations
#include "message_slot.h"

// The message the device will give when asked
/** Linked List and shit
 * Data structures
 /

static bool isEmpty(LinkedList *L){
  return L->size==0;
}

static void push(LinkedList *L, char *data){
  Node *newNode = kmalloc(sizeof(Node),GFP_KERNEL);
  if (!newNode)
  {
    return NULL;
  }
  
  newNode->data=data;
  newNode->next=NULL;
  if (isEmpty(L))
  {
    L->size=1;
    L->head = L->tail = newNode;
    return;
  }
  (L->size)++;
  newNode->next = L->tail;
  L->tail=newNode;
}

static void printList(LinkedList *L){
  Node *cur;
  
  if (isEmpty(L))
  {
    printk("Empty");
    return;
  }
  cur = L->head;
  while (cur!=NULL)
  {
    printk("%s -> ",cur->data);
    cur = cur->next;
  }
  printk("NULL\n");
  
}

static LinkedList* create_list(void){
  LinkedList* l = kmalloc(sizeof(LinkedList),GFP_KERNEL);
  if (!l)
  {
    return NULL;
  }
  
  l->head = NULL;
  l->tail = NULL;
  l->size = 0;
  return l;
}

static void clear_list(LinkedList *l){
  Node* cur = l->head;
  Node* next;
  if (cur==NULL)
  {
    kfree(l);
    return;
  }
  next = cur->next;
  while(next!=NULL){
    kfree(cur);
    cur = next;
    next = cur->next;
  }
  kfree(l);
}

static LinkedList* channels[CHANNELS_SIZE];

/********************/

/**
 * array of arrays of string
 * channels[i] = channels of minor number i
 * channels[i][j] = message in channel j of minor number i
 **/
typedef struct channel
{
  unsigned int id;
  char *msg;
  int len;
} channel;

static channel* channels[CHANNELS_SIZE];
static int num_of_channels[CHANNELS_SIZE];

static unsigned int active_channel[CHANNELS_SIZE];

int search_channel(int minor, unsigned int search_id){
  int i=0;
  channel *local_channels = channels[minor];
  for (i = 0; i < num_of_channels[minor]; i++)
  {
    if (local_channels[i].id==search_id)
    {
      return i;
    }
  }
  return -1;
}

// channel* new_channel(int id, char *msg){
//   channel *new_chan = kmalloc(sizeof(channel),GFP_KERNEL);
//   if (!new_chan)
//   {
//     return NULL;
//   }
//   new_chan->id = id;
//   new_chan->msg = msg;
//   return new_chan;
// }

int add_channel(int minor, unsigned int id){
  channel *new_chan = NULL;
  channel *cur_channels = channels[minor];
  int n = num_of_channels[minor];
  int index = search_channel(minor, id);
  if (index==-1)
  {
    // new_chan = new_channel(id, NULL);
    // if (!new_chan)
    // {
    //   return ALOC_ERR;
    // }
    // num_of_channels[minor]++;
    // channels[minor] = krealloc(sizeof(channel)*num_of_channels[minor], GFP_KERNEL);
    // if (!channels[minor])
    // {
    //   return ALOC_ERR;
    // }
    
    // channels[minor][num_of_channels[minor]] = channels[minor][num_of_channels[minor]-1]; //last one is still the sentinel(id := -1)
    // channels[minor][num_of_channels[minor]-1] = new_chan;
    
    new_chan = krealloc(cur_channels, (n+1)*sizeof(channel), GFP_KERNEL);
    if (!new_chan)
    {
      printk(KERN_ERR "failed to add channel, remains unchanged");
      return ALOC_ERR;
    }
    channels[minor] = new_chan;
    num_of_channels[minor] = n+1;
    new_chan[n].id = id;
    new_chan[n].len = -1;
    new_chan[n].msg = NULL;
    return SUCCESS;
  }
  else{
    return CH_EXISTS;
  }
}

channel* get_channel(int minor, unsigned int channel_id){
  int ind = search_channel(minor, channel_id);
  if (ind==-1)
  {
    return NULL;
  }
  return &channels[minor][ind];
}

int init_channels(void){
  int i;
  for (i = 0; i < CHANNELS_SIZE; i++)
  {
    num_of_channels[i] = 0;
  }
  return SUCCESS;
}

//================== DEVICE FUNCTIONS ===========================
static int device_open( struct inode* inode,
                        struct file*  file )
{
  unsigned int minor = iminor(inode);
  printk("Invoking device_open(%p)\n", file);
  file->private_data = (void *) &minor;

  return SUCCESS;
}

//---------------------------------------------------------------
static int device_release( struct inode* inode,
                           struct file*  file)
{
  printk("Invoking device_release(%p,%p)\n", inode, file);

  // ready for our next caller
  return SUCCESS;
}

//---------------------------------------------------------------
// a process which has already opened
// the device file attempts to read from it
static ssize_t device_read( struct file* file,
                            char __user* buffer,
                            size_t       length,
                            loff_t*      offset )
{
  int bytes_read = 0;
  unsigned int minor = *(unsigned int *)(file->private_data);
  channel *ch = get_channel(minor, active_channel[minor]);
  printk("Invoking device_read(%p,%ld)\n", file, length);
  /* required error checking */
  if (ch==NULL){
    return -EINVAL;
  }
  if (ch->len==-1)
  {
    return -EWOULDBLOCK;
  }
  if (ch->len > length)
  {
    return -ENOSPC;
  }
  /***/

  while (bytes_read<ch->len)
  {
    if (put_user(ch->msg[bytes_read++], buffer++)<0){
      return -EFAULT;
    }
  }
  //invalid argument error
  return ch->len;
}

//---------------------------------------------------------------
// a processs which has already opened
// the device file attempts to write to it
static ssize_t device_write( struct file*       file,
                             const char __user* buffer,
                             size_t             length,
                             loff_t*            offset)
{
  int i = 0;
  unsigned int minor = *(unsigned int *)(file->private_data);
  channel *ch = get_channel(minor, active_channel[minor]);
  printk("Invoking device_write(%p,%ld)\n", file, length);
  /* required error checking */
  if (ch==NULL){
    return -EINVAL;
  }
  if (length<=0 || length >128)
  {
    return -EMSGSIZE;
  }
  /***/

  ch->len = 0;
  ch->msg = krealloc(ch->msg,sizeof(char)*length, GFP_KERNEL);
  if (!ch->msg)
  {
    return -ALOC_ERR;
  }
  
  while (i<length)
  {
    if (get_user(ch->msg[i++], buffer++)<0){
      printk("in i = %d", i);
      ch->len=-1;
      return -EFAULT;
    }
    ch->len++;
  }
  
  // return the number of input characters used
  return length;
}

//----------------------------------------------------------------
static long device_ioctl( struct   file* file,
                          unsigned int   ioctl_command_id,
                          unsigned long  ioctl_param )
{
  // Switch according to the ioctl called
  if( MSG_SLOT_CHANNEL == ioctl_command_id )
  {
    // Get the parameter given to ioctl by the process
    unsigned int minor =  *(unsigned int *)(file->private_data);
    printk( "Invoking ioctl: setting active channel "
            "to %ld\n", ioctl_param );
    if (add_channel(minor, ioctl_param)==ALOC_ERR){
      return -ENOMEM;
    }
    active_channel[minor] = ioctl_param;
  }
  return SUCCESS;
}

//==================== DEVICE SETUP =============================

// This structure will hold the functions to be called
// when a process does something to the device we created
struct file_operations Fops =
{
  .owner	  = THIS_MODULE, 
  .read           = device_read,
  .write          = device_write,
  .open           = device_open,
  .unlocked_ioctl = device_ioctl,
  .release        = device_release,
};

//---------------------------------------------------------------
// Initialize the module - Register the character device
static int __init simple_init(void)
{
  int rc = -1;
  //LinkedList *l;

  // Register driver capabilities. Obtain major num
  rc = register_chrdev( MAJOR_NUM, DEVICE_NAME, &Fops );

  // Negative values signify an error
  if( rc < 0 )
  {
    printk( KERN_ALERT "%s registraion failed for  %d\n",
                       DEVICE_NAME, MAJOR_NUM );
    return rc;
  }
  // for (i = 0; i < CHANNELS_SIZE; i++)
  // {
  //   l = create_list();
  //   if (!l)
  //   {
  //     for (j = 0; j < i; j++)
  //     {
  //       clear_list(channels[j]);
  //     }
  //     printk(KERN_ERR "problem with allocating memory to linked lists for (%s, %d)", DEVICE_NAME, MAJOR_NUM);
  //     return -1;
  //   }
    
  //   channels[i] = l;
  // }

  if (init_channels()==ALOC_ERR){
    return -1;
  }
  
  printk( "Registeration is successful. ");
  printk( "If you want to talk to the device driver,\n" );
  printk( "you have to create a device file:\n" );
  printk( "mknod /dev/%s c %d 0\n", DEVICE_NAME, MAJOR_NUM );
  printk( "You can echo/cat to/from the device file.\n" );
  printk( "Dont forget to rm the device file and "
          "rmmod when you're done\n" );

  return 0;
}

//---------------------------------------------------------------
static void __exit simple_cleanup(void)
{
  // Unregister the device
  // Should always succeed
  
  int i;
  for (i = 0; i < CHANNELS_SIZE; i++)
  {
    kfree(channels[i]); 
  }
  printk("\nGoodbye, besto-friendo\n");
  unregister_chrdev(MAJOR_NUM, DEVICE_NAME);
}

//---------------------------------------------------------------
module_init(simple_init);
module_exit(simple_cleanup);

//========================= END OF FILE =========================
