/* See LICENSE file for license and copyright details */
#ifndef DRUNKCAN_PROTOCOL_H
#define DRUNKCAN_PROTOCOL_H
enum protocol {
  CANOPEN = 1,
};

struct protocol_conf {
  enum protocol protocol;
  void *protocol_state;
  unsigned int frame_size;
  int (*get_id)(int);
  int (*validate_can)(struct can_frame fr, ...);
  int (*validate_sock)(void *data, size_t size, ...);
  int (*process_can)(struct can_frame fr, char *buf, ...);
  int (*cleanup)(void *state);
};
#endif
