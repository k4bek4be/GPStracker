#include <string.h>
#include <avr/pgmspace.h>
#include "xprintf.h"
#include "math.h"
#include "main.h"
#include "gpx.h"
#include "ff.h"

#define KALMAN_Q	8.5e-6
#define KALMAN_R	4e-5
#define KALMAN_ERR_MAX	6e-4

__flash const char xml_header[] = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
		"<gpx xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns=\"http://www.topografix.com/GPX/1/1\" xsi:schemaLocation=\"http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd\" version=\"1.1\" creator=\"k4be\">\n"
		"\t<trk>\n"
		"\t\t<trkseg>\n";

FIL gpx_file;
static char buf[sizeof(xml_header)+1];

struct kalman_s {
	unsigned char initialized;
	float x_est_last;
	float P_last;
	float Q;
	float R;
	float K;
};


#define PREV_POINTS_LENGTH	4
#define AVG_COUNT	3
#define MIN_DIST_DELTA	2.0

struct {
	struct location_s data[PREV_POINTS_LENGTH];
	unsigned char start;
	unsigned char count;
} prev_points;
struct location_s last_saved;

static struct kalman_s kalman[2];
static struct {
	float lat;
	float lon;
	time_t time;
} avg_store;
static unsigned char avg_count;


float kalman_predict(struct kalman_s *k, float data);
void kalman_init(struct kalman_s *k);
float distance(struct location_s *pos1, struct location_s *pos2);

void prev_points_append(struct location_s *new){
	prev_points.data[(prev_points.start + prev_points.count)%PREV_POINTS_LENGTH] = *new;
	if(++prev_points.count > PREV_POINTS_LENGTH){
		prev_points.count--;
		prev_points.start++;
		prev_points.start %= PREV_POINTS_LENGTH;
	}
}

struct location_s *prev_points_get(unsigned char index){
	unsigned char i, addr = prev_points.start;
	for(i=0; i<index; i++){
		addr++;
		addr %= PREV_POINTS_LENGTH;
	}
	return &prev_points.data[addr];
}


unsigned char gpx_init(FIL *file) {
	unsigned int bw;
	
	kalman_init(&kalman[0]);
	kalman_init(&kalman[1]);
	prev_points.count = 0;
	avg_count = 0;	
	last_saved.lon = 0;
	last_saved.lat = 0;
	last_saved.time = 0;

	strcpy_P(buf, xml_header);
	return f_write(file, buf, strlen(buf), &bw);
}

unsigned char gpx_write(struct location_s *loc, FIL *file) {
	unsigned int bw;
	const char *time;
	
	time = get_iso_time(loc->time);
	xsprintf(buf, PSTR("\t\t\t<trkpt lat=\"%.8f\" lon=\"%.8f\">\n\t\t\t\t<time>%s</time>\n"), loc->lat, loc->lon, time);
	/* alt */
	strcat_P(buf, PSTR("\t\t\t</trkpt>\n"));
	return f_write(file, buf, strlen(buf), &bw);
}

unsigned char gpx_close(FIL *file) {
	unsigned int bw;
	strcpy_P(buf, PSTR("\t\t</trkseg>\n\t</trk>\n</gpx>\n"));
	f_write(file, buf, strlen(buf), &bw);
	return f_close(file);
}

void gpx_process_point(struct location_s *loc, FIL *file){
	float lon_est, lon_err, lat_est, lat_err;
	struct location_s *ptr;
	struct location_s nloc;
	
	lat_est = kalman_predict(&kalman[0], loc->lat);
	lon_est = kalman_predict(&kalman[1], loc->lon);
	
	lat_err = fabs(loc->lat - lat_est);
	lon_err = fabs(loc->lon - lon_est);
//	xprintf(PSTR("lat_err: %e, lon_err: %e, limit: %e\r\n"), lat_err, lon_err, (float)KALMAN_ERR_MAX);
	if(lat_err > KALMAN_ERR_MAX || lon_err > KALMAN_ERR_MAX){
		xputs_P(PSTR("KALMAN REJECT\r\n"));
		return;
	}
	loc->lat = lat_est;
	loc->lon = lon_est;
	
	prev_points_append(loc);
	if(prev_points.count == PREV_POINTS_LENGTH){
		float dist12 = distance(prev_points_get(0), prev_points_get(1));
		float dist34 = distance(prev_points_get(2), prev_points_get(3));
		float dist32 = distance(prev_points_get(2), prev_points_get(1));
		xprintf(PSTR("New distance: %fm\r\n"), dist32);
		if(dist34 > dist12 && dist32 > dist12){
			xputs_P(PSTR("DISTANCE DIFF REJECT\r\n"));
			return;
		}
		ptr = prev_points_get(PREV_POINTS_LENGTH - 2);
	} else {
		if(prev_points.count >= PREV_POINTS_LENGTH-2){
			ptr = prev_points_get(prev_points.count - 2);
			xputs_P(PSTR("NEW\r\n"));
		} else {
			return;
		}
	}

	if(distance(&last_saved, ptr) < MIN_DIST_DELTA){
		xputs_P(PSTR("Too small position change REJECT\r\n"));
		return;
	}

	xputs_P(PSTR("ACCEPT\r\n"));

	avg_store.lat += ptr->lat;
	avg_store.lon += ptr->lon;
	if(avg_count == AVG_COUNT/2)
		avg_store.time = ptr->time;
	
	if(++avg_count == AVG_COUNT){
		nloc.lat = avg_store.lat / AVG_COUNT;
		nloc.lon = avg_store.lon / AVG_COUNT;
		nloc.time = avg_store.time;
		avg_count = 0;
		avg_store.lat = 0;
		avg_store.lon = 0;
		avg_store.time = 0;
		last_saved = nloc;
		gpx_write(&nloc, file);
		return;
	}
}

void kalman_init(struct kalman_s *k){
	k->initialized = 0;
	k->P_last = 0;
	//the noise in the system
	k->Q = KALMAN_Q; // process variance
	k->R = KALMAN_R; // measurement variance
	k->K = 0;
}

float kalman_predict(struct kalman_s *k, float data){
	if(!k->initialized){
		//initial values for the kalman filter
		k->x_est_last = data;
		k->initialized = 1;
		return data;
	}
	//do a prediction
	float x_temp_est = k->x_est_last;
	float P_temp = k->P_last + k->Q;
	//calculate the Kalman gain
	k->K = P_temp * (1.0/(P_temp + k->R));
	//correct
	float x_est = x_temp_est + k->K * (data - x_temp_est);
	k->P_last = (1 - k->K) * P_temp;
	k->x_est_last = x_est;
	return x_est;
}

float distance(struct location_s *pos1, struct location_s *pos2){
	float lon_delta = fabs(pos1->lon - pos2->lon) * 111139.0;
	float lat_delta = fabs(pos1->lat - pos2->lat) * 111139.0;
//	xprintf(PSTR("lat1=%f; lat2=%f; lon1=%f; lon2=%f; lat_delta=%f; lon_delta=%f\r\n"), pos1->lat, pos2->lat, pos1->lon, pos2->lon, lon_delta, lat_delta);
	return sqrtf(lon_delta * lon_delta + lat_delta * lat_delta);
}

