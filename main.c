/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <kevinmottashed@gmail.com> wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return. 
 * -Kevin Mottashed
 * ----------------------------------------------------------------------------
 */

#include "manticore.h"
#include "clock.h"
#include "gpio.h"

#include <stdbool.h>
#include <string.h>
#include <assert.h>

// 128 bytes of stack should be enough for these dummy tasks.
#define NUM_TASKS (8)
#define STACK_SIZE (128)

// This collection of tasks are used to test the system.
// I hope to eventually have a proper test framework but this will
// have to do for now.
extern __task void * Task_Busy_Yield(void * arg);
static __task void * task_mutex_try_lock(void * arg);
static __task void * task_led3(void * arg);
static __task void * task_led4(void * arg);
static __task void * task_never(void * arg);
static __task void * task_mutex_lock(void * arg);
static __task void * task_mutex_lock2(void * arg);
static __task void * task_led_server(void * arg);

// ARM requires 8 byte alignment for the stack.
#pragma data_alignment = 8
static uint8_t stacks[NUM_TASKS][STACK_SIZE];

static mutex_handle_t mutex;
static channel_handle_t ledChannel;

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

__task void * task_mutex_try_lock(void * arg)
{
  // This task locks and unlocks the mutex.
  while (true)
  {
    for (int i = 0x10000; i < 0x20000; ++i)
    {
      if (mutex_trylock(mutex))
      {
        mutex_unlock(mutex);
      }
      else
      {
        mutex_lock(mutex);
        mutex_unlock(mutex);
      }
    }
  }
}

__task void * task_led3(void * arg)
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
    channel_send(ledChannel, &cmd, sizeof(cmd), &reply, &replyLen);
    assert(replyLen == sizeof(reply));
    assert(reply.ok);
    task_delay(x);
    on = !on;
  }
}

__task void * task_led4(void * arg)
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
    channel_send(ledChannel, &cmd, sizeof(cmd), &reply, &replyLen);
    assert(replyLen == sizeof(reply));
    assert(reply.ok);
    task_delay(x);
    on = !on;
  }
}

__task void * task_never(void * arg)
{
  // This task should never be scheduled due to its low priority.
  assert(false);
  return NULL;
}

__task void * task_mutex_lock(void * arg)
{
  // This task locks and unlocks a mutex.
  while (true)
  {
    mutex_lock(mutex);
    task_delay(50);
    mutex_unlock(mutex);
    task_delay(50);
  }
}

__task void * task_mutex_lock2(void * arg)
{
  // This task locks and unlocks a mutex.
  while (true)
  {
    mutex_lock(mutex);
    task_delay(200);
    
    // task_mutex_lock() should be blocked on us
    assert(task_get_priority(NULL) == 12);
    
    mutex_unlock(mutex);
    
    // We should be back to our original priority.
    assert(task_get_priority(NULL) == 10);
    task_delay(200);
  }
}

__task void * task_led_server(void * arg)
{
  // This task receives messages on a channel to turn LEDs on and off.
  while (true)
  {
    led_cmd_t cmd;
    led_cmd_reply_t reply;
    
    size_t len = channel_recv(ledChannel, &cmd, sizeof(cmd));
    assert(len == sizeof(cmd));
    
    if (cmd.led == 3 && cmd.state == true)
    {
      // We should have inherited the priority of the sender.
      assert(task_get_priority(NULL) == 15);
      gpio_led3_on();
      reply.ok = true;
    }
    else if (cmd.led == 3 && cmd.state == false)
    {
      assert(task_get_priority(NULL) == 15);
      gpio_led3_off();
      reply.ok = true;
    }
    else if (cmd.led == 4 && cmd.state == true)
    {
      assert(task_get_priority(NULL) == 20);
      gpio_led4_on();
      reply.ok = true;
    }
    else if (cmd.led == 4 && cmd.state == false)
    {
      assert(task_get_priority(NULL) == 20);
      gpio_led4_off();
      reply.ok = true;
    }
    else
    {
      reply.ok = false;
    }
    channel_reply(ledChannel, &reply, sizeof(reply));
    
    // We should be back to our original priority.
    assert(task_get_priority(NULL) == 10);
  }
}

int main()
{
  // Initialize the hardware.
  clock_init();
  gpio_init();

  // Initialize the kernel
  manticore_init();
  
  // Create some synchronization mechanisms.
  mutex = mutex_create();
  ledChannel = channel_create();
  
  //
  // Create all the tasks.
  //
  
  //           Entry                Argument   Stack      Stack Size  Priority
  task_create(&Task_Busy_Yield,     NULL,      stacks[0], STACK_SIZE, 10);
  task_create(&task_mutex_try_lock, NULL,      stacks[1], STACK_SIZE, 10);
  task_create(&task_led3,           (void*)33, stacks[2], STACK_SIZE, 15);
  task_create(&task_led4,           (void*)56, stacks[3], STACK_SIZE, 20);
  task_create(&task_never,          NULL,      stacks[4], STACK_SIZE, 5);
  task_create(&task_mutex_lock,     NULL,      stacks[5], STACK_SIZE, 12);
  task_create(&task_mutex_lock2,    NULL,      stacks[6], STACK_SIZE, 10);
  task_create(&task_led_server,     NULL,      stacks[7], STACK_SIZE, 10);
  
  // Start the kernel.
  manticore_main();
}
