#include <opencv2/core.hpp>
#include <stack>

using namespace cv;



cv::Mat floodFillDFS(cv::Mat& img, cv::Mat& res, int startX, int startY) {
    int width = img.cols;
    int height = img.rows;

    const uchar white = 255;
    const uchar red = 255;

    if (img.at<uchar>(startY, startX) == white)
        return res;

    std::stack<cv::Point> stack;
    stack.push(cv::Point(startX, startY));

    while (!stack.empty()) {
        cv::Point p = stack.top();
        stack.pop();

        int x = p.x;
        int y = p.y;

        if (x < 0 || x >= width || y < 0 || y >= height)
            continue;

        if (img.at<uchar>(y, x) == white)
            continue;

        img.at<uchar>(y, x) = white;
        res.at<uchar>(y, x) = red;

        stack.push(cv::Point(x + 1, y));
        stack.push(cv::Point(x - 1, y));
        stack.push(cv::Point(x, y + 1));
        stack.push(cv::Point(x, y - 1));
    }

    return res;
}