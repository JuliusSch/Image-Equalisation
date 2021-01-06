#include <iostream>
#include <vector>

#include "Utils.h"
#include "CImg.h"

using namespace cimg_library;

/*
Julius Coady Schaebitz - SCH16631355, Parallel programming - CMP3752M: Assignment 1

Extension details: deadline moved from 16:00, 30/04/2020 to 16:00, 07/05/2020. Authorisation code: XXX1XXX.

Running on: 

Microsoft Visual Studio Community 2019
Version 16.5.2
VisualStudio.16.Release/16.5.2+29926.136
Microsoft .NET Framework
Version 4.8.03752

Installed Version: Community

Visual C++ 2019   00435-60000-00000-AA365
Microsoft Visual C++ 2019

ASP.NET and Web Tools 2019   16.5.236.49856
ASP.NET and Web Tools 2019

ASP.NET Web Frameworks and Tools 2019   16.5.236.49856
For additional information, visit https://www.asp.net/

Azure App Service Tools v3.0.0   16.5.236.49856
Azure App Service Tools v3.0.0

Azure Functions and Web Jobs Tools   16.5.236.49856
Azure Functions and Web Jobs Tools

C# Tools   3.5.0-beta4-20153-05+20b9af913f1b8ce0a62f72bea9e75e4aa3cf6b0e
C# components used in the IDE. Depending on your project type and settings, a different version of the compiler may be used.

Common Azure Tools   1.10
Provides common services for use by Azure Mobile Services and Microsoft Azure Tools.

Intel Debugger for Heterogeneous Compute   1.0
Provides debugging capabilities for compute shaders running on the Intel GPU.

IntelliCode Extension   1.0
IntelliCode Visual Studio Extension Detailed Info

Microsoft Azure Tools   2.9
Microsoft Azure Tools for Microsoft Visual Studio 2019 - v2.9.30207.1

Microsoft Continuous Delivery Tools for Visual Studio   0.4
Simplifying the configuration of Azure DevOps pipelines from within the Visual Studio IDE.

Microsoft JVM Debugger   1.0
Provides support for connecting the Visual Studio debugger to JDWP compatible Java Virtual Machines

Microsoft Library Manager   2.1.25+gdacdb9b7a1
Install client-side libraries easily to any web project

Microsoft MI-Based Debugger   1.0
Provides support for connecting Visual Studio to MI compatible debuggers

Microsoft Visual C++ Wizards   1.0
Microsoft Visual C++ Wizards

Microsoft Visual Studio Tools for Containers   1.1
Develop, run, validate your ASP.NET Core applications in the target environment. F5 your application directly into a container with debugging, or CTRL + F5 to edit & refresh your app without having to rebuild the container.

Microsoft Visual Studio VC Package   1.0
Microsoft Visual Studio VC Package

NuGet Package Manager   5.5.0
NuGet Package Manager in Visual Studio. For more information about NuGet, visit https://docs.nuget.org/

ProjectServicesPackage Extension   1.0
ProjectServicesPackage Visual Studio Extension Detailed Info

SQL Server Data Tools   16.0.62003.05170
Microsoft SQL Server Data Tools

Test Adapter for Boost.Test   1.0
Enables Visual Studio's testing tools with unit tests written for Boost.Test.  The use terms and Third Party Notices are available in the extension installation directory.

Test Adapter for Google Test   1.0
Enables Visual Studio's testing tools with unit tests written for Google Test.  The use terms and Third Party Notices are available in the extension installation directory.

TypeScript Tools   16.0.20225.2001
TypeScript Tools for Microsoft Visual Studio

Visual Basic Tools   3.5.0-beta4-20153-05+20b9af913f1b8ce0a62f72bea9e75e4aa3cf6b0e
Visual Basic components used in the IDE. Depending on your project type and settings, a different version of the compiler may be used.

Visual F# Tools 10.8.0.0 for F# 4.7   16.5.0-beta.20104.8+7c4de19faf36647c1ef700e655a52350840c6f03
Microsoft Visual F# Tools 10.8.0.0 for F# 4.7

Visual Studio Code Debug Adapter Host Package   1.0
Interop layer for hosting Visual Studio Code debug adapters in Visual Studio

Visual Studio Container Tools Extensions (Preview)   1.0
View, manage, and diagnose containers within Visual Studio.

Visual Studio Tools for CMake   1.0
Visual Studio Tools for CMake

Visual Studio Tools for Containers   1.0
Visual Studio Tools for Containers

Visual Studio Tools for Unity   4.5.1.0
Visual Studio Tools for Unity



Development Summary:

The program can handle images of varying sizes and bit depths, both in colour and greyscale. Additionally, the number of bins is customisable. Varying the bin size
varies the number of distinct colours that make up the final image. E.g. for 256 colour values in 64 bins, the number of colours present in the final image will be reduced by a factor of 4.
The program has a robust procedure for reading user input to determine parameters for running the equalisation. The program also records transfer and execution times for all
events in each kernel, and calculates and provides totals for these values. An optimised, local implementation for each kernel was attempted unsuccessfully.

The official OpenCL documentation was used for reference.
*/

void print_help() {
	std::cerr << "Application usage:" << std::endl;

	std::cerr << "  -p : select platform " << std::endl;
	std::cerr << "  -d : select device" << std::endl;
	std::cerr << "  -l : list all platforms and devices" << std::endl;
	std::cerr << "  -f : input image file (default: test.ppm)" << std::endl;
	std::cerr << "  -h : print this message" << std::endl;
}

//Returns the execution time of a given event in nanoseconds
int duration(cl::Event event) {
	return (event.getProfilingInfo<CL_PROFILING_COMMAND_END>() - event.getProfilingInfo<CL_PROFILING_COMMAND_START>());
}

//Histogram
static vector<unsigned int> histogram(cl::Context context, cl::Program program, cl::CommandQueue queue, CImg<unsigned char> image_input, char implem, const int no_bins, int* times) {
	vector<unsigned int> hist_output(no_bins, 0);

	//Buffers
	cl::Buffer image_input_buffer(context, CL_MEM_READ_ONLY, image_input.size());
	cl::Buffer hist_output_buffer(context, CL_MEM_READ_WRITE, hist_output.size() * sizeof(int));
	cl::Buffer no_bins_input(context, CL_MEM_READ_ONLY, sizeof(int));

	//Set up profiling for queue
	cl::Event event_w1;
	cl::Event event_w2;
	cl::Event event_f;
	cl::Event event_r;
	cl::Event event_hist;

	//Copy image data to device memory
	queue.enqueueWriteBuffer(image_input_buffer, CL_TRUE, 0, image_input.size(), &image_input.data()[0], NULL, &event_w1);
	queue.enqueueWriteBuffer(no_bins_input, CL_TRUE, 0, sizeof(int), &no_bins, NULL, &event_w2);

	//Initialise the output to zeroes
	queue.enqueueFillBuffer(hist_output_buffer, 0, 0, no_bins * sizeof(int), NULL, &event_f);
	
	//Set up and execute the kernel
	cl::Kernel kernel_hist;

	//Execute serial implementation using global memory or optimised implementation using local memory
	if (implem == 'l') {
		kernel_hist = cl::Kernel(program, "hist_local");
		kernel_hist.setArg(3, cl::Local(hist_output.size() * sizeof(int)));
	}
	else if (implem == 'g') kernel_hist = cl::Kernel(program, "hist_simple");
	else std::cerr << "error: invalid assignment for variable 'implem'" << std::endl;

	kernel_hist.setArg(0, image_input_buffer);
	kernel_hist.setArg(1, hist_output_buffer);
	kernel_hist.setArg(2, no_bins_input);

	queue.enqueueNDRangeKernel(kernel_hist, cl::NullRange, cl::NDRange(image_input.size()), cl::NDRange(no_bins), NULL, &event_hist);

	//Copy the result from device to host
	queue.enqueueReadBuffer(hist_output_buffer, CL_TRUE, 0, hist_output.size() * sizeof(int), &hist_output[0], NULL, &event_r);

	//std::cout << hist_output << std::endl;
	//std::cout << GetFullProfilingInfo(event_hist, ProfilingResolution::PROF_US) << std::endl;

	//Record transfer and execution times for kernel
	int transfer_time = duration(event_w1) + duration(event_w2) + duration(event_f) + duration(event_r);
	int exec_time = duration(event_hist);
	std::cout << "Histogram                      |  " << transfer_time << "                   |  " << exec_time << "                   |  " << transfer_time + exec_time << std::endl;
	times[0] += transfer_time;
	times[1] += exec_time;
	times[2] += (transfer_time + exec_time);
	return hist_output;
}

//Cumulative histogram
static vector<unsigned int> cumulative_histogram(cl::Context context, cl::Program program, cl::CommandQueue queue, vector<unsigned int> hist_output_buffer, char implem, const int no_bins, int* times) {
	vector<unsigned int> cum_hist_out_buffer(no_bins, 0);

	//Buffers
	cl::Buffer hist_input(context, CL_MEM_READ_ONLY, hist_output_buffer.size() * sizeof(int));
	cl::Buffer cum_hist_output(context, CL_MEM_READ_WRITE, cum_hist_out_buffer.size() * sizeof(int));

	//Profiling
	cl::Event event_w;
	cl::Event event_f;
	cl::Event event_r;
	cl::Event event_CH;

	//Init buffers
	queue.enqueueWriteBuffer(hist_input, CL_TRUE, 0, hist_output_buffer.size() * sizeof(int), &hist_output_buffer[0], NULL, &event_w);
	queue.enqueueFillBuffer(cum_hist_output, 0, 0, cum_hist_out_buffer.size() * sizeof(int), NULL, &event_f);

	//Set up and execute kernel
	cl::Kernel kernel_CH;
	if (implem == 'l') {
		kernel_CH = cl::Kernel(program, "scan_add_atomic_local");
		kernel_CH.setArg(2, cl::Local(8 * sizeof(int)));
	}
	else if (implem == 'g') kernel_CH = cl::Kernel(program, "scan_add_atomic");
	else std::cerr << "error: invalid assignment for variable 'implem'" << std::endl;
	kernel_CH.setArg(0, hist_input);
	kernel_CH.setArg(1, cum_hist_output);

	queue.enqueueNDRangeKernel(kernel_CH, cl::NullRange, cl::NDRange(hist_output_buffer.size() * sizeof(int)), cl::NDRange(no_bins), NULL, &event_CH);
	queue.enqueueReadBuffer(cum_hist_output, CL_TRUE, 0, cum_hist_out_buffer.size() * sizeof(int), &cum_hist_out_buffer[0], NULL, &event_r);

	//std::cout << cum_hist_out_buffer << std::endl;

	//Sum up and print transfer and execution times of kernel
	int transfer_time = duration(event_w) + duration(event_f) + duration(event_r);
	int exec_time = duration(event_CH);
	std::cout << "Cumulative histogram           |  " << transfer_time << "                     |  " << exec_time << "                   |  " << transfer_time + exec_time << std::endl;
	times[0] += transfer_time;
	times[1] += exec_time;
	times[2] += (transfer_time + exec_time);
	return cum_hist_out_buffer;
}

//Look-up table scaling
static vector<unsigned int> LUT_scaling(cl::Context context, cl::Program program, cl::CommandQueue queue, vector<unsigned int> cum_hist_out_buffer, const float *scale_val, char implem, const int no_bins, int* times) {
	vector<unsigned int> LUT_output_data(no_bins, 0);
	
	//Buffers
	cl::Buffer LUT_input(context, CL_MEM_READ_ONLY, cum_hist_out_buffer.size() * sizeof(int));
	cl::Buffer LUT_output(context, CL_MEM_READ_WRITE, LUT_output_data.size() * sizeof(int));
	cl::Buffer scale(context, CL_MEM_READ_ONLY, sizeof(float));

	//Profiling
	cl::Event event_w1;
	cl::Event event_f;
	cl::Event event_w2;
	cl::Event event_r;
	cl::Event event_LUT;

	//Init input buffer with correct values and output with zeroes
	queue.enqueueWriteBuffer(LUT_input, CL_TRUE, 0, LUT_output_data.size() * sizeof(int), &cum_hist_out_buffer[0], NULL, &event_w1);
	queue.enqueueFillBuffer(LUT_output, 0, 0, LUT_output_data.size() * sizeof(int), NULL, &event_f);
	queue.enqueueWriteBuffer(scale, CL_TRUE, 0, sizeof(int), &scale_val[0], NULL, &event_w2);

	//Set up and execute kernel
	cl::Kernel kernel_LUT;
	if (implem == 'l') {
		kernel_LUT = cl::Kernel(program, "LUT_scale_local");
		kernel_LUT.setArg(3, cl::Local(8 * sizeof(int)));
	}
	else if (implem == 'g') kernel_LUT = cl::Kernel(program, "LUT_scale");
	else std::cerr << "error: invalid assignment for variable 'implem'" << std::endl;
	kernel_LUT.setArg(0, LUT_input);
	kernel_LUT.setArg(1, LUT_output);
	kernel_LUT.setArg(2, scale);

	queue.enqueueNDRangeKernel(kernel_LUT, cl::NullRange, cl::NDRange(cum_hist_out_buffer.size() * sizeof(int)), cl::NDRange(no_bins), NULL, &event_LUT);
	queue.enqueueReadBuffer(LUT_output, CL_TRUE, 0, LUT_output_data.size() * sizeof(int), &LUT_output_data[0], NULL, &event_r);

	//std::cout << LUT_output_data << std::endl;
	
	int transfer_time = duration(event_w1) + duration(event_f) + duration(event_w2) + duration(event_r);
	int exec_time = duration(event_LUT);
	std::cout << "Look-up table                  |  " << transfer_time << "                     |  " << exec_time << "                     |  " << transfer_time + exec_time << std::endl;
	times[0] += transfer_time;
	times[1] += exec_time;
	times[2] += (transfer_time + exec_time);
	return LUT_output_data;
}

//Image projection
static vector<unsigned char> re_projection(cl::Context context, cl::Program program, cl::CommandQueue queue, CImg<unsigned char> image_input, vector<unsigned int> LUT_output_data, char implem, const int no_bins, int* times) {
	vector<unsigned char> RP_output_data(image_input.size(), 0);

	//Buffers
	cl::Buffer dev_image_input(context, CL_MEM_READ_ONLY, image_input.size());
	cl::Buffer LUT_data(context, CL_MEM_READ_ONLY, LUT_output_data.size() * sizeof(int));
	cl::Buffer RP_output(context, CL_MEM_READ_WRITE, RP_output_data.size());
	cl::Buffer no_bins_input(context, CL_MEM_READ_ONLY, sizeof(int));

	//Profiling
	cl::Event event_w1;
	cl::Event event_w2;
	cl::Event event_w3;
	cl::Event event_f;
	cl::Event event_r;
	cl::Event event_RP;

	//Init buffers
	queue.enqueueWriteBuffer(dev_image_input, CL_TRUE, 0, image_input.size(), &image_input.data()[0], NULL, &event_w1);
	queue.enqueueWriteBuffer(LUT_data, CL_TRUE, 0, LUT_output_data.size() * sizeof(int), &LUT_output_data[0], NULL, &event_w2);
	queue.enqueueWriteBuffer(no_bins_input, CL_TRUE, 0, sizeof(int), &no_bins, NULL, &event_w3);
	queue.enqueueFillBuffer(RP_output, 0, 0, image_input.size(), NULL, &event_f);

	//kernel
	cl::Kernel kernel_RP;
	if (implem == 'l') {
		kernel_RP = cl::Kernel(program, "re_project_local");
		kernel_RP.setArg(4, cl::Local(8 * sizeof(int)));
	}
	else if (implem == 'g') kernel_RP = cl::Kernel(program, "re_project");
	else std::cerr << "error: invalid assignment for variable 'implem'" << std::endl;
	kernel_RP.setArg(0, dev_image_input);
	kernel_RP.setArg(1, LUT_data);
	kernel_RP.setArg(2, RP_output);
	kernel_RP.setArg(3, no_bins_input);

	queue.enqueueNDRangeKernel(kernel_RP, cl::NullRange, cl::NDRange(image_input.size()), cl::NDRange(no_bins), NULL, &event_RP);
	queue.enqueueReadBuffer(RP_output, CL_TRUE, 0, RP_output_data.size(), &RP_output_data[0], NULL, &event_r);
	
	int transfer_time = duration(event_w1) + duration(event_w2) + duration(event_w3) + duration(event_f) + duration(event_r);
	int exec_time = duration(event_RP);
	std::cout << "Re-projection                  |  " << transfer_time << "                   |  " << exec_time << "                    |  " << transfer_time + exec_time << std::endl;
	times[0] += transfer_time;
	times[1] += exec_time;
	times[2] += (transfer_time + exec_time);
	return RP_output_data;
}

int main(int argc, char **argv) {
	//Part 1 - handle command line options such as device selection, verbosity, etc.
	int platform_id = 0;
	int device_id = 0;
	string image_filename = "test.pgm";
	//string image_filename = "test_large.pgm";

	for (int i = 1; i < argc; i++) {
		if ((strcmp(argv[i], "-p") == 0) && (i < (argc - 1))) { platform_id = atoi(argv[++i]); }
		else if ((strcmp(argv[i], "-d") == 0) && (i < (argc - 1))) { device_id = atoi(argv[++i]); }
		else if (strcmp(argv[i], "-l") == 0) { std::cout << ListPlatformsDevices() << std::endl; }
		else if ((strcmp(argv[i], "-f") == 0) && (i < (argc - 1))) { image_filename = argv[++i]; }
		else if (strcmp(argv[i], "-h") == 0) { print_help(); return 0; }
	}

	cimg::exception_mode(0);

	//detect any potential exceptions
	try {
		bool image_found = false;
		bool bins_assigned = false;
		bool imp_set = false;
		int no_bins;
		char implem;
		CImg<unsigned char> image_input;

		//Read and check user input
		while (!image_found) {	//loop until valid input is received
			std::cout << "Enter image filename - 'test.pgm'/'test_large.pgm': or enter 'd' for default:" << std::endl;
			string temp;
			std::cin >> temp;
			if (temp == "d") image_filename = "test.pgm";
			try {
				if (!(temp == "d")) image_filename = temp;
				image_input = CImg<unsigned char>(image_filename.c_str());
				image_found = true;
			}
			catch (CImgException& err) {
				std::cout << "error: invalid image filename." << std::endl;
				image_filename = "test.pgm";
			}
		}
		while (!bins_assigned) {
			std::cout << "Enter desired number of bins - this should divide the total pixel count and be less than 256 - or enter 'd' for default:" << std::endl;
			string temp2;
			std::cin >> temp2;
			//Default to 256 bins
			if (temp2 == "d") no_bins = 256;
			try {
				if (!(temp2 == "d")) no_bins = std::stoi(temp2);
				if ((image_input.size() % no_bins == 0 && no_bins <= 256) || no_bins == -1) bins_assigned = true;
				else std::cout << "error: number of bins must be divisor of number of pixels and less than 256." << std::endl;
			}
			catch (std::invalid_argument) {
				std::cout << "error: enter integer value." << std::endl;
			}
		}
		while (!imp_set) {
			std::cout << "Enter desired implementation, 'l' for local and 'g' for global - or enter 'd' for default:" << std::endl;
			std::cin >> implem;
			if (implem == 'd') implem = 'g';
			if (implem == 'l' || implem == 'g') imp_set = true;
			else std::cout << "error: input 'd', 'l' or 'g'." << std::endl;

		}
		CImgDisplay disp_input(image_input,"input");
		const float LUT_scale_val = (float)(no_bins-1)/(float)image_input.size();

		//Host operations

		//Select computing devices
		cl::Context context = GetContext(platform_id, device_id);

		//display the selected device
		std::cout << "Running on " << GetPlatformName(platform_id) << ", " << GetDeviceName(platform_id, device_id) << std::endl;

		//create a queue to which we will push commands for the device
		cl::CommandQueue queue(context, CL_QUEUE_PROFILING_ENABLE);

		//Load & build the device code
		cl::Program::Sources sources;

		AddSources(sources, "kernels/my_kernels.cl");

		cl::Program program(context, sources);

		//build and debug the kernel code
		try { 
			program.build();
		}
		catch (const cl::Error& err) {
			std::cout << "Build Status: " << program.getBuildInfo<CL_PROGRAM_BUILD_STATUS>(context.getInfo<CL_CONTEXT_DEVICES>()[0]) << std::endl;
			std::cout << "Build Options:\t" << program.getBuildInfo<CL_PROGRAM_BUILD_OPTIONS>(context.getInfo<CL_CONTEXT_DEVICES>()[0]) << std::endl;
			std::cout << "Build Log:\t " << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(context.getInfo<CL_CONTEXT_DEVICES>()[0]) << std::endl;
			throw err;
		}
		std::cout << std::endl;
		std::cout << "Operation                      |  Transfer time (ns)       |  Execution time (ns)      |  Total time (ns)" << std::endl;
		std::cout << "---------------------------------------------------------------------------------------------------------" << std::endl;
		int total_event_times[3] = {};

		//Operation 1: Histogram
		vector<unsigned int> hist_output_buffer = histogram(context, program, queue, image_input, implem, no_bins, total_event_times);
		
		//Operation 2: Cumulative histogram
		vector<unsigned int> cum_hist_out_buffer = cumulative_histogram(context, program, queue, hist_output_buffer, implem, no_bins, total_event_times);
		
		//Operation 3: LUT scaling
		vector<unsigned int> LUT_output_data = LUT_scaling(context, program, queue, cum_hist_out_buffer, &LUT_scale_val, implem, no_bins, total_event_times);

		//Operation 4: Re-projection
		vector<unsigned char> RP_output_data = re_projection(context, program, queue, image_input, LUT_output_data, implem, no_bins, total_event_times);

		//Print total execution times
		std::cout << "---------------------------------------------------------------------------------------------------------" << std::endl;
		std::cout << "Totals                         |  " << total_event_times[0] << "                   |  " << total_event_times[1] << "                   |  " << total_event_times[2] << std::endl;

		CImg<unsigned char> output_image(RP_output_data.data(), image_input.width(), image_input.height(), image_input.depth(), image_input.spectrum());
		CImgDisplay disp_output(output_image,"output");

 		while (!disp_input.is_closed() && !disp_output.is_closed()
			&& !disp_input.is_keyESC() && !disp_output.is_keyESC()) {
		    disp_input.wait(1);
		    disp_output.wait(1);
	    }		

	}
	catch (const cl::Error& err) {
		std::cerr << "ERROR: " << err.what() << ", " << getErrorString(err.err()) << std::endl;
	}
	catch (CImgException& err) {
		std::cerr << "ERROR: " << err.what() << std::endl;
	}

	return 0;
}