
//Standard implementations using resource intensive calls to global memory:

//Histogram kernel:
kernel void hist_simple(global const uchar* input, global int* H, global const int* no_bins) {
	int id = get_global_id(0);
	float bin_size = 256/no_bins[0];
	int bin_index = input[id]/bin_size;
	atomic_inc(&H[bin_index]);
}

//Cumulative histogram kernel:
kernel void scan_add_atomic(global int* H, global int* CH) {
	int id = get_global_id(0);
	int N = get_global_size(0);
	for (int i = id+1; i < N && id < N; i++)
		atomic_add(&CH[i], H[id]);
}

//LUT scale kernel:
kernel void LUT_scale(global const int* input, global int* output, global const float* scale) {
	int id = get_global_id(0);
	output[id] = input[id] * scale[0];
}

//Image re-projection kernel:
kernel void re_project(global const uchar* input, global const int* LUT, global uchar* output, global const int* no_bins) {
	int id = get_global_id(0);
	float bin_size = 256/no_bins[0];
	int pixel = input[id];
	output[id] = bin_size*LUT[(int)(pixel/bin_size)];
}

//Optimised implementations making use of local memory:

//Histogram kernel - local memory implementation:
kernel void hist_local(global const uchar* input, global int* H, global const int* no_bins, local int* tempH) {
	int id = get_global_id(0);
	int lid = get_local_id(0);
	float bin_size = 256/no_bins[0];
	int bin_index = input[id]/bin_size;
	
	if (lid < no_bins[0]) H[lid] = 0;
	barrier(CLK_LOCAL_MEM_FENCE);

	atomic_inc(&tempH[bin_index]);

	barrier(CLK_LOCAL_MEM_FENCE);

	if (!lid) atomic_add(&H[bin_index], tempH[bin_index]);
}

//Cumulative histogram kernel - local memory implementation:
kernel void scan_add_atomic_local(global int* H, global int* CH, local int* temp) {
	int id = get_global_id(0);//
	int lid = get_local_id(0);
	int n = get_global_size(0);//
	int ln = get_local_size(0);

	temp[lid] = H[id];

	barrier(CLK_LOCAL_MEM_FENCE);

	for (int i = lid+1; i < ln && lid < n; i++) {
		temp[lid] += temp[i];

		barrier(CLK_LOCAL_MEM_FENCE);
	}

	//for (int i = id+1; i < n && id < n; i++)//
	//	atomic_add(&CH[i], H[id]);//
	//atomic_add(&CH[id])

}

//LUT scale kernel - local memory implementation:
kernel void LUT_scale_local(global const int* input, global int* output, global const float* scale, local int* temp) {

}

//Image re-projection kernel - local memory implementation:
kernel void re_project_local(global const uchar* input, global const int* LUT, global uchar* output, global const int* no_bins, local int* temp) {

}