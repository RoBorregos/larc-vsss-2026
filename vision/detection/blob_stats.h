#ifndef BLOB_STATS_H
#define BLOB_STATS_H

struct BlobStats {
	unsigned int count;
	unsigned int count_low_red;
	unsigned int count_high_red;
	unsigned long long sum_s, sq_sum_s;
	unsigned int min_s, max_s;
	unsigned long long sum_v, sq_sum_v;
	unsigned int min_v, max_v;
	unsigned long long sum_h, sq_sum_h;
	unsigned int min_h, max_h;
	unsigned long long sum_h_wrap, sq_sum_h_wrap;
	unsigned int min_h_wrap, max_h_wrap;
};

#endif //BLOB_STATS_H
