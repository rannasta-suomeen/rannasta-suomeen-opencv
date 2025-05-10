#ifndef RESOLVE_MASK_H
#define RESOLVE_MASK_H

#include <opencv2/core.hpp> 

struct ResolveOptions {
    bool verbose {false};
    bool debug {false};
    const char* input_path;
    const char* output_path;
};

struct ResolveResult {
    cv::Mat result;
    cv::Point center;
};

ResolveResult resolveMask(const cv::Mat&, ResolveOptions);

#endif // RESOLVE_MASK_H