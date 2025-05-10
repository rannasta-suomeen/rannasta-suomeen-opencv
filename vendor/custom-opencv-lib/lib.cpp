#include <iostream>
#include <opencv2/opencv.hpp>
#include "opencv2/imgcodecs.hpp"
#include "resolve_mask.hpp"
#include "flood_fill.hpp"



int run_image_tool(ResolveOptions options) {
    cv::Mat src;
    src = cv::imread(options.input_path);
    if (src.empty()) {
        std::cerr << "> image not found!\n";
        return -1;
    }
    
    int height = src.rows;
    int width = src.cols;
    double aspectRatio = static_cast<double>(height) / width;
    
    int targetHeight = 700;
    int targetWidth = static_cast<int>(targetHeight / aspectRatio);
    

    cv::Mat resized;
    cv::resize(src, resized, cv::Size(targetWidth, targetHeight));

    cv::Mat padded;
    cv::copyMakeBorder(resized, padded, 120, 120, 120, 120, cv::BORDER_CONSTANT, cv::Scalar(255, 255, 255));

    auto [canny, mid] = resolveMask(padded, options);
    
    
    if (options.debug) {
        cv::imwrite("output/canny.png", canny);
        std::cout << "> midpoint: (" << mid.x << ", " << mid.y << ")\n";
    }

    cv::Mat c2 = canny.clone();
    cv::Mat c4 = cv::Mat::zeros(c2.size(), CV_8UC1);
    cv::Mat result = cv::Mat::zeros(c2.size(), CV_8UC1);

    int cx = mid.x;
    int cy = mid.y;

    c4 = floodFillDFS(c2, c4, cx, cy);
    c4.at<uchar>(cy, cx) = 200;

    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;
    cv::findContours(c4, contours, hierarchy, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);

    if (contours.empty()) {
        std::cerr << "No contours found after flood fill.\n";
        return -1;
    }

    auto bigContour = *std::max_element(contours.begin(), contours.end(),
        [](const std::vector<cv::Point>& a, const std::vector<cv::Point>& b) {
            return cv::contourArea(a) < cv::contourArea(b);
        });

    cv::drawContours(result, std::vector<std::vector<cv::Point>>{bigContour}, 0, cv::Scalar(255), cv::FILLED);

    cv::Mat imgBgra;
    cv::cvtColor(padded, imgBgra, cv::COLOR_BGR2BGRA);

    cv::Mat mask = cv::Mat::zeros(imgBgra.size(), CV_8UC4);
    cv::drawContours(mask, std::vector<std::vector<cv::Point>>{bigContour}, 0, cv::Scalar(255, 255, 255, 255), 4);

    cv::Mat imask;
    cv::bitwise_not(mask, imask);

    cv::Mat imgMasked;
    imgBgra.copyTo(imgMasked, result);

    cv::Mat blurred;
    cv::GaussianBlur(imgMasked, blurred, cv::Size(31, 31), 1.0);

    cv::Mat output;
    output = cv::Mat::zeros(imgMasked.size(), imgMasked.type());

    for (int y = 0; y < output.rows; ++y) {
        for (int x = 0; x < output.cols; ++x) {
            if (mask.at<cv::Vec4b>(y, x) == cv::Vec4b(255, 255, 255, 255)) {
                output.at<cv::Vec4b>(y, x) = imask.at<cv::Vec4b>(y, x);
            } else {
                output.at<cv::Vec4b>(y, x) = blurred.at<cv::Vec4b>(y, x);
            }
        }
    }

    if (options.debug) {
        std::cout << "> writing debug to ./output" << std::endl;
        cv::imwrite("output/result3.png", result);
        cv::imwrite("output/result_mask.png", imgMasked);
        cv::imwrite("output/output.png", output);
    }
    cv::imwrite(options.output_path, output);

    return 0;
}
