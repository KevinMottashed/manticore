/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <kevinmottashed@gmail.com> wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return. 
 * -Kevin Mottashed
 * ----------------------------------------------------------------------------
 */

#include "system.h"
#include "task.h"
#include "kernel.h"
#include "syscall.h"
#include "clock.h"
#include "mutex.h"
#include "gpio.h"

#include <stdbool.h>
#include <string.h>
#include <assert.h>

// 128 bytes of stack should be enough for these dummy tasks.
#define NUM_TASKS (7)
#define STACK_SIZE (128)

// This collection of tasks are used to test the system.
// I hope to eventually have a proper test framework but this will
// have to do for now.
extern __task void Task_Busy_Yield(void * arg);
static __task void task_mutex_try_lock(void * arg);
static __task void task_led3(void * arg);
static __task void task_led4(void * arg);
static __task void task_never(void * arg);
static __task void task_mutex_lock(void * arg);
static __task void task_led_server(void * arg);

// ARM requires 8 byte alignment for the stack.
#pragma data_alignment = 8
static uint8_t stacks[NUM_TASKS][STACK_SIZE];

static mutex_t mutex;
static channel_t ledChannel;

// The message format used to communicate with the LED task.
typedef struct led_cmd_s
{
  int led;
  bool state;
} led_cmd_t;

typedef struct led_cmd_reply_s
{
  bool ok;
} led_cmd_reply_t;

__task void task_mutex_try_lock(void * arg)
{
  // This task locks and unlocks the mutex.
  while (true)
  {
    for (int i = 0x10000; i < 0x20000; ++i)
    {
      if (mutex_trylock(&mutex))
      {
        mutex_unlock(&mutex);
      }
      else
      {
        mutex_lock(&mutex);
        mutex_unlock(&mutex);
      }
    }
  }
}

__task void task_led3(void * arg)
{
  // This task sends messages to the LED server task to toggle LED3.
  unsigned int x = (unsigned int)arg;
  led_cmd_t cmd;
  cmd.led = 3;
  led_cmd_reply_t reply; 
  bool on = true;
  while (true)
  {
    size_t replyLen = sizeof(reply);
    cmd.state = on;
    channel_send(&ledChannel, &cmd, sizeof(cmd), &reply, &replyLen);
    assert(replyLen == sizeof(reply));
    assert(reply.ok);
    sleep(x);
    on = !on;
  }
}

__task void task_led4(void * arg)
{
  // This task sends messages to the LED server task to toggle LED4.
  unsigned int x = (unsigned int)arg;
  led_cmd_t cmd;
  cmd.led = 4;
  led_cmd_reply_t reply; 
  bool on = true;
  while (true)
  {
    size_t replyLen = sizeof(reply);
    cmd.state = on;
    channel_send(&ledChannel, &cmd, sizeof(cmd), &reply, &replyLen);
    assert(replyLen == sizeof(reply));
    assert(reply.ok);
    sleep(x);
    on = !on;
  }
}

__task void task_never(void * arg)
{
  // This task should never be scheduled due to its low priority.
  assert(false);
}

__task void task_mutex_lock(void * arg)
{
  // This task locks and unlocks a mutex.
  while (true)
  {
    mutex_lock(&mutex);
    delay(50);
    mutex_unlock(&mutex);
    delay(50);
  }
}

__task void task_led_server(void * arg)
{
  // This task receives messages on a channel to turn LEDs on and off.
  while (true)
  {
    led_cmd_t cmd;
    led_cmd_reply_t reply;
    channel_recv(&ledChannel, &cmd, sizeof(cmd));
    if (cmd.led == 3 && cmd.state == true)
    {
      gpio_led3_on();
      reply.ok = true;
    }
    else if (cmd.led == 3 && cmd.state == false)
    {
      gpio_led3_off();
      reply.ok = true;
    }
    else if (cmd.led == 4 && cmd.state == true)
    {
      gpio_led4_on();
      reply.ok = true;
    }
    else if (cmd.led == 4 && cmd.state == false)
    {
      gpio_led4_off();
      reply.ok = true;
    }    
    else
    {
      reply.ok = false;
    }
    channel_reply(&ledChannel, &reply, sizeof(reply));
  }
}

int main()
{
  // Initialize the hardware.
  clock_init();
  gpio_init();

  // Initialize the kernel
  kernel_init();
  
  // Initialize some synchronization mechanisms.
  mutex_init(&mutex);
  channel_init(&ledChannel);
  
  //
  // Create all the tasks.
  //
  
  //                 Entry                 Argument   Stack      Stack Size  Priority
  kernel_create_task(&Task_Busy_Yield,     NULL,      stacks[0], STACK_SIZE, 10);
  kernel_create_task(&task_mutex_try_lock, NULL,      stacks[1], STACK_SIZE, 10);
  kernel_create_task(&task_led3,           (void*)3,  stacks[2], STACK_SIZE, 15);
  kernel_create_task(&task_led4,           (void*)5,  stacks[3], STACK_SIZE, 20);
  kernel_create_task(&task_never,          NULL,      stacks[4], STACK_SIZE, 5);
  kernel_create_task(&task_mutex_lock,     NULL,      stacks[5], STACK_SIZE, 10);
  kernel_create_task(&task_led_server,     NULL,      stacks[6], STACK_SIZE, 10);
  
  // Start the kernel.
  kernel_main();
}
