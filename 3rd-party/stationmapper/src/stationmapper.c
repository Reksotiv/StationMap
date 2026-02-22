#include "stdio.h"
#include "string.h"
#include "math.h"

#ifdef _WIN32
  #include <io.h>
  #include <windows.h>
  #include <wchar.h>

  // Checking access to a file with UTF-8
  static int access_utf8(const char *path, int mode) {
      wchar_t w_path[MAX_PATH];
      MultiByteToWideChar(CP_UTF8, 0, path, -1, w_path, MAX_PATH);
      return _waccess(w_path, mode);
  }
  #ifndef F_OK
  #define F_OK   0
  #endif

  #ifndef W_OK
  #define W_OK   2
  #endif

  #ifndef R_OK
  #define R_OK   4
  #endif
#else
  #include "unistd.h"
  #define access_utf8 access
#endif

#include "../include/stationmapper.h"

#define LOADBMP_IMPLEMENTATION
#include "../include/loadbmp.h"

#define LIB_VERSION_MAJOR 1
#define LIB_VERSION_MINOR 0
#define LIB_VERSION_PATCH 2

const version_t get_library_version(void) {
    const version_t version = {.major = LIB_VERSION_MAJOR, .minor = LIB_VERSION_MINOR, .patch = LIB_VERSION_PATCH};
    return version;
}

peace_of_map_t load_map(const char* image_filename, const char* config_filename) {
    peace_of_map_t map;

    if(access_utf8(image_filename, F_OK) != 0) {
        printf("Failed to load map image file %s\n", image_filename);
        return map;
    }

    if(access_utf8(config_filename, F_OK) != 0) {
        printf("Failed to load config file %s\n", config_filename);
        return map;
    }

    unsigned int err = loadbmp_decode_file(image_filename, &map.image, &map.width, &map.height, LOADBMP_RGBA);
    if (err)
		printf("LoadBMP Load Error: %u\n", err);
    
    FILE *fp = FOPEN_UTF8(config_filename, "r");
    fscanf(fp, "%*[^\n]\n");
    fscanf(fp, "%f, %f, %f, %f\n", &map.top_left_lat, &map.top_left_lon, &map.bottom_right_lat, &map.bottom_right_lon);
    fclose(fp);

    return map;
}


int save_map(const peace_of_map_t *map, const char *output_filename) {
    return loadbmp_encode_file(output_filename, map->image, map->width, map->height, LOADBMP_RGBA);
}


void draw_pixel(unsigned char * image, int width, int x, int y, int r, int g, int b, int a)
{
	image[(width * y + x) * 4] = r;
	image[(width * y + x) * 4 + 1] = g;
	image[(width * y + x) * 4 + 2] = b;
	image[(width * y + x) * 4 + 3] = a;
}


void add_pixel(unsigned char * image, int width, int x, int y, int r, int g, int b, int a)
{
    int idx = (width * y + x) * 4;

    image[idx]     = image[idx] + (r - image[idx]) * a / 255;
    image[idx + 1] = image[idx + 1] + (g - image[idx + 1]) * a / 255;
    image[idx + 2] = image[idx + 2] + (b - image[idx + 2]) * a / 255;
}


void draw_point_by_lat_lon(peace_of_map_t * map, float lat, float lon, int r, int g, int b, int a) {
    int x = map->width * ((map->top_left_lon - lon) / (map->top_left_lon - map->bottom_right_lon));
    int y = map->height * ((map->top_left_lat - lat) / (map->top_left_lat - map->bottom_right_lat));
    if ((x < 0) || (x >= map->width) || (y < 0) || (y > map->height)) {
        printf("Incorrect lat lon: %f %f\n", lat, lon);
        return;
    }
    
    for (int i = -5; i < 5; i++) {
        for (int j = -5; j < 5; j++) {
            if ((x + i < 0) || (x + i >= map->width) || (y + j < 0) || (y + j > map->height)) {
                continue;
            }
            add_pixel(map->image, map->width, x + i, y + j, r, g, b, a);
        }
    }
}


stations_list_t load_stations(const char *stations_list_filename) {
    stations_list_t stations_list;
    stations_list.num_stations = 0;
    stations_list.stations = NULL;

    // Open file and check for errors
    FILE *fp = FOPEN_UTF8(stations_list_filename, "r");
    if (fp == NULL) {
        printf("Failed to open stations file: %s\n", stations_list_filename);
        return stations_list;
    }

    // Count entries (skip header first)
    char line[512];
    fgets(line, sizeof(line), fp);  // Skip header
    
    while (fgets(line, sizeof(line), fp) != NULL) {
        stations_list.num_stations++;
    }
    
    // Allocate memory
    stations_list.stations = malloc(stations_list.num_stations * sizeof(station_t));
    if (stations_list.stations == NULL) {
        printf("Memory allocation failed\n");
        fclose(fp);
        stations_list.num_stations = 0;
        return stations_list;
    }

    // Rewind and read stations
    rewind(fp);
    fgets(line, sizeof(line), fp);  // Skip header again
    
    for (int i = 0; i < stations_list.num_stations; i++) {
        if (fgets(line, sizeof(line), fp) == NULL) break;
        
        // Parse CSV manually to handle spaces in names
        char *token = strtok(line, ",");
        if (token) stations_list.stations[i].id = atoi(token);
        
        token = strtok(NULL, ",");
        if (token) strncpy((char*)stations_list.stations[i].name, token, 255);
        
        token = strtok(NULL, ",");
        if (token) stations_list.stations[i].lat = atof(token);
        
        token = strtok(NULL, ",\n");
        if (token) stations_list.stations[i].lon = atof(token);
    }
    
    fclose(fp);
    return stations_list;
}


float deg_to_rad(float deg) {
  return deg * (3.1415 / 180);
}


float get_distance_in_km(float lat_1, float lon_1, float lat_2, float lon_2) {
    const float R = 6371.0f;
    
    float dlat = deg_to_rad(lat_2 - lat_1);
    float dlon = deg_to_rad(lon_2 - lon_1);
    float lat1 = deg_to_rad(lat_1);
    float lat2 = deg_to_rad(lat_2);
    
    float a = sinf(dlat / 2) * sinf(dlat / 2) +
              cosf(lat1) * cosf(lat2) *
              sinf(dlon / 2) * sinf(dlon / 2);
    
    float c = 2.0f * atan2f(sqrtf(a), sqrtf(1.0f - a));
    
    return R * c;
}


station_t get_nearest_station(stations_list_t *stations, float lat, float lon) {
    int idx_min = 3000;
    float min_dist = 1.e5;
    for (int i = 0; i < stations->num_stations; i++) {
        float dist = get_distance_in_km(stations->stations[i].lat, stations->stations[i].lon, lat, lon);
        if (dist < min_dist) {
            min_dist = dist;
            idx_min = i;
        }
    }
    return stations->stations[idx_min];
}