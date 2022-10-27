/* See LICENSE file for license and copyright details */
#ifndef DRUNKCAN_PROTOCOL_H
#define DRUNKCAN_PROTOCOL_H
enum protocol {
  CANOPEN = 1,
};

struct protocol_conf {
  enum protocol protocol;
  void *state;
  unsigned int frame_size;
  int (*get_id)(int);
  int (*validate_can)(struct can_frame fr, void *state);
  int (*validate_sock)(void *data, size_t size, void *state);
  int (*process_can)(struct can_frame fr, void *buf, void *state);
  int (*process_sock)(struct can_frame *fr, void *buf, size_t size, int id,
		      void *state);
  int (*cleanup)(void *state);
};
#endif
