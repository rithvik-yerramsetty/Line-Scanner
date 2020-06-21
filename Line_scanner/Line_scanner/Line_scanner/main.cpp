#include "utils.h"

InputBuffer::InputBuffer(double TV, int m) {
	size_buffer_queue = 0;
	threshold_value = TV;
	number_columns = m;
	start_pixel_number = 0;
}

void InputBuffer::ScannerFeed(std::vector<unsigned int> &input_pixels) {
	while (true) {
		std::unique_lock<std::mutex> locker(mutex_input_buffer);
		condition_var_input_buffer.wait(locker, [this]() {return size_buffer_queue < WINDOW_SIZE;});
		int input_pixels_size = input_pixels.size();
		int pixel_number = 0;
		while (pixel_number < input_pixels_size) {
			buffer_queue.push(input_pixels[pixel_number]);
			pixel_number++;
		}
		while (pixel_number) {
			input_pixels.pop_back();
			pixel_number--;
		}
		size_buffer_queue += input_pixels_size;
		locker.unlock();
		condition_var_input_buffer.notify_all();
		return;
	}
}

int InputBuffer::Filter() {
	while (true) {
		std::unique_lock<std::mutex> locker(mutex_input_buffer);
		condition_var_input_buffer.wait(locker, [this]() {return size_buffer_queue >= WINDOW_SIZE;});
		static int cnt = 0;
		cnt++;
		double filtered_value = 0;
		int temp_size = 0;
		filtered_value += (double)buffer_queue.front() * FILTER_WINDOW[temp_size];
		std::queue<unsigned int> q;
		buffer_queue.pop();
		temp_size++;
		while (temp_size < WINDOW_SIZE) {
			if (temp_size == (WINDOW_SIZE / 2))
				std::cout <<"Pixel- "<< buffer_queue.front() << "\n";
			filtered_value += buffer_queue.front() * FILTER_WINDOW[temp_size];
			buffer_queue.push(buffer_queue.front());
			buffer_queue.pop();
			temp_size++;
		}
		while (temp_size < size_buffer_queue) {
			buffer_queue.push(buffer_queue.front());
			buffer_queue.pop();
			temp_size++;
		}
		start_pixel_number++;
		if (start_pixel_number + WINDOW_SIZE > number_columns) {
			while (!buffer_queue.empty())
				buffer_queue.pop();
			size_buffer_queue = 1;
			start_pixel_number = 0;
		}
		size_buffer_queue--;
		locker.unlock();
		condition_var_input_buffer.notify_all();
		std::cout <<"Filtered Value: "<< filtered_value << "\n";
		return(filtered_value >= threshold_value);
	}
}

DataGenerator::DataGenerator(InputBuffer* buffer, int t,int m) {
	input_buffer = buffer;
	process_time = t;
	num_columns = m;
}

void DataGenerator::Run(int input_mode) {
	int columns_generated = 0;
	switch (input_mode) {
	case MODE_GENERATING_DATA:
		while (true) {
			std::vector<unsigned int> generated_pixels;
			int number_pixels_generated = 0;
			srand(unsigned(time(NULL)));
			total_pixels++;
			while (columns_generated<num_columns && number_pixels_generated < PIXELS_TO_BE_SCANNED) {
				generated_pixels.push_back(unsigned(((rand() + rand()) * total_pixels)) % PIXEL_RANGE);
				total_pixels++;
				number_pixels_generated++;
				columns_generated++;
			}
			if (columns_generated == num_columns)
				columns_generated = 0;
			input_buffer->ScannerFeed(generated_pixels);
			std::this_thread::sleep_for(std::chrono::nanoseconds(process_time));
		}
		break;

	case MODE_TEST_DATA:
		while (true) {
			std::ifstream data_input_file(TEST_FILE_PATH);
			if (!data_input_file.is_open()) 
				throw std::runtime_error("Could not open file");
			std::string line;
			while (std::getline(data_input_file, line)) {
				std::vector<unsigned int> read_pixels;
				int columns_scanned = 0;
				std::stringstream ss(line);
				std::string pixel;
				int pixel_count = 0;
				while (std::getline(ss, pixel, ',')) {
					unsigned int pixel_value;
					pixel_value = unsigned int(stoi(pixel));
					read_pixels.push_back(pixel_value);
					pixel_count++;
					columns_scanned++;
					if (pixel_count == PIXELS_TO_BE_SCANNED || columns_scanned == num_columns) {
						input_buffer->ScannerFeed(read_pixels);
						std::this_thread::sleep_for(std::chrono::nanoseconds(process_time));
						if (columns_scanned == num_columns)
							break;
						pixel_count = 0;
					}
				}
			}
			data_input_file.close();
			break;
		}
		break;

	default:
		std::cout << "Invalid Input Mode!!! \n";
		break;
	}
}

FilterThreshold::FilterThreshold(InputBuffer* buffer) {
	input_buffer = buffer;
}

void FilterThreshold::run() {
	while (true) {
		int x = input_buffer->Filter();
		std::cout <<"Flag: "<< x << "\n\n";
	}
}

int main() {
	double threshold_value;
	int number_columns;
	int process_time;
	int input_mode;
	std::cout << "Number of columns: \n";
	std::cin >> number_columns;
	if (number_columns < WINDOW_SIZE) {
		std::cout << "number of columns should be greater than WINDOW SIZE!!! \n";
		return 0;
	}
	std::cout << "Threshold value: \n";
	std::cin >> threshold_value;
	std::cout << "Process time(in nanoseconds): \n";
	std::cin >> process_time;
	std::cout << "Input mode: \n" << MODE_GENERATING_DATA << " - Data generating mode\n" << MODE_TEST_DATA << " - Test Mode\n";
	std::cin >> input_mode;
	InputBuffer input_buffer(threshold_value, number_columns);
	DataGenerator data_generator(&input_buffer, process_time, number_columns);
	FilterThreshold filter(&input_buffer);

	std::thread t1(&DataGenerator::Run, &data_generator, input_mode);
	std::thread t2(&FilterThreshold::run, &filter);

	t1.join();
	t2.join();

	return 0;
}