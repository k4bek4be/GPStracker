#pragma once

#include "ff.h"

extern FIL gpx_file;

unsigned char gpx_init(FIL *file);
unsigned char gpx_write(struct location_s *loc, FIL *file);
unsigned char gpx_close(FIL *file);
void gpx_process_point(struct location_s *loc, FIL *file);
void gpx_save_single_point(struct location_s *loc);
void add_distance(float dist);

