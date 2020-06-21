#pragma once
#include <iostream>
#include <thread>
#include <queue>
#include <vector>
#include <mutex>
#include <chrono>
#include <fstream>
#include <sstream>
#include <stdexcept>
#define PIXEL_RANGE 256
#define WINDOW_SIZE 9
#define PIXELS_TO_BE_SCANNED 2
#define TEST_FILE_PATH "./test_large.csv"

const double FILTER_WINDOW[WINDOW_SIZE] = { 0.00025177, 0.008666992, 0.078025818, 0.24130249, 0.343757629, 0.24130249, 0.078025818, 0.008666992,  0.000125885 };
static const int MODE_GENERATING_DATA = 0;
static const int MODE_TEST_DATA = 1;
int total_pixels = 0;

class InputBuffer {
private:
	std::mutex mutex_input_buffer;
	std::condition_variable condition_var_input_buffer;
	std::queue <unsigned int>  buffer_queue;
	int size_buffer_queue;
	double threshold_value;
	int number_columns;
	int start_pixel_number;
public:
	InputBuffer(double TV, int m);

	/*This method receives input pixels from DataGenerator and fills those pixels into the buffer_queue 
	till it's size is less than WINDOW_SIZE*/
	void ScannerFeed(std::vector<unsigned int> &input_pixels);

	/*This method processes the buffer_queue pixels according to the given criteria and flags the pixel
	as 0/1 according to the threshold value given by user. */
	int Filter();
};

class DataGenerator {
private:
	InputBuffer* input_buffer;
	int process_time;
	int num_columns;
public:
	DataGenerator(InputBuffer* buffer, int t,int m);

	/*This method operates in two modes according to the input given by the user- 
	1.MODE_GENERATING_DATA- In this mode, this block generates random pixels using rand() function and 
	fills the vector generated_pixels until it’s size is less than 	PIXELS_TO_BE_SCANNED. After generating 
	the required pixels, the vector is passed to 	ScannerFeed method to fill the input_buffer.
	2.MODE_TEST_DATA- In this mode, the block reads PIXELS_TO_BE_SCANNED number of 
	pixels from the FILE_PATH provided and fills the generated_pixels vector and then the 
	vector is passed to ScannerFeed method to fill input_buffer.*/

	void Run(int input_mode);
};

class FilterThreshold {
private:
	InputBuffer* input_buffer;
public:
	FilterThreshold(InputBuffer* buffer);

	/* This method calls the filter method of input_buffer object.
	TODO: For now I am printing the flag value of the pixel(0/1) to the output,
	if another process block uses this output it can be stored in another buffer queue and processed in parallel.*/

	void run();
};