
#include <iostream>
#include "opencv2/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"
 
using namespace std;
using namespace cv;

const double CLEAN_CUT_THRESHOLD = 1.0;

cv::RNG rng(cv::getTickCount());
 
cv::Mat adjustGamma(const cv::Mat& image, double gamma = 1.0) {
    CV_Assert(gamma > 0);
    double invGamma = 1.0 / gamma;

    uchar table[256];
    for (int i = 0; i < 256; ++i) {
        table[i] = cv::saturate_cast<uchar>(std::pow(i / 255.0, invGamma) * 255.0);
    }

    cv::Mat lut(1, 256, CV_8U, table);

    cv::Mat result;
    cv::LUT(image, lut, result);
    return result;
}


struct MorphResult {
    cv::Mat morphClean;
    cv::Mat morphDirty;
    cv::Mat debugDirty;
    cv::Mat debugClean;
};

MorphResult applyMorphology(const cv::Mat& imgGamma) {
    MorphResult result;

    cv::Mat gray;
    cv::cvtColor(imgGamma, gray, cv::COLOR_RGB2GRAY);

    cv::Mat otsuClean;
    cv::threshold(gray, otsuClean, 250, 253, cv::THRESH_BINARY_INV);

    cv::Mat otsuDirty;
    cv::threshold(gray, otsuDirty, 254, 254, cv::THRESH_BINARY_INV);

    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(2, 2));
    cv::morphologyEx(otsuClean, result.morphClean, cv::MORPH_CLOSE, kernel, cv::Point(-1, -1), 3);

    kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(7, 7));
    cv::morphologyEx(otsuDirty, result.morphDirty, cv::MORPH_CLOSE, kernel, cv::Point(-1, -1), 2);

    cv::cvtColor(result.morphDirty.clone(), result.debugDirty, cv::COLOR_GRAY2RGB);
    cv::cvtColor(result.morphDirty.clone(), result.debugClean, cv::COLOR_GRAY2RGB);

    return result;
}


cv::Mat fixDirtyContours(const cv::Mat& morphDirty, cv::Mat& debugCanvas) {
    cv::Mat morphFixed = cv::Mat::ones(morphDirty.size(), CV_8U);

    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;
    cv::findContours(morphDirty, contours, hierarchy, cv::RETR_CCOMP, cv::CHAIN_APPROX_SIMPLE);

    for (size_t i = 0; i < contours.size(); ++i) {
        if (hierarchy[i][3] > -1) {
            cv::drawContours(debugCanvas, contours, static_cast<int>(i), cv::Scalar(255, 0, 0), cv::FILLED);
            cv::drawContours(morphFixed, contours, static_cast<int>(i), 255, cv::FILLED);
        }
    }

    return morphFixed;
}


std::vector<cv::Point> extractKeyContourPoints(const cv::Mat& cannyImg, cv::Mat& debugCanvas) {
    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;
    cv::findContours(cannyImg, contours, hierarchy, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);

    std::vector<cv::Point> keyPoints;
    std::vector<cv::Point> tmp;

    for (const auto& cnt : contours) {
        for (size_t i = 0; i + 1 < cnt.size(); ++i) {
            const auto& p1 = cnt[i];
            const auto& p2 = cnt[i + 1];

            cv::Point pt = tmp.empty() ? p2 : tmp.back();

            double d1 = cv::norm(p2 - p1);
            double d2 = cv::norm(pt - p1);

            if (d1 >= 5) {
                cv::line(debugCanvas, p1, p2, cv::Scalar(0, 0, 255), 1);
                cv::circle(debugCanvas, p1, 1, cv::Scalar(0, 0, 255));
                cv::circle(debugCanvas, p2, 1, cv::Scalar(0, 0, 255));
                keyPoints.push_back(p1);
            } else if (d1 <= 2) {
                tmp.push_back(p1);
                if (d2 > 5) {
                    tmp.clear();
                }

                if (tmp.size() >= 6) {
                    const auto& s = tmp[0];
                    const auto& m = tmp[2];
                    const auto& e = tmp.back();

                    auto slope = [](const cv::Point& a, const cv::Point& b) -> double {
                        if (b.x - a.x == 0) return 999.0;
                        return (double)(b.y - a.y) / (b.x - a.x);
                    };

                    double k_sm = slope(s, m);
                    double k_me = slope(m, e);
                    double k_se = slope(s, e);

                    double score = std::abs(std::abs(k_sm) - std::abs(k_me));
                    double _score = std::abs(std::abs(k_sm) - std::abs(k_se));
                    double thresh = 0.7;
                    bool straight = (score < thresh) && (_score < thresh);

                    if (straight) {
                        cv::line(debugCanvas, s, e, cv::Scalar(255, 255, 0), 1);
                        keyPoints.push_back(p1);
                    }

                    tmp.clear();
                }
            } else {
                cv::circle(debugCanvas, p1, 1, cv::Scalar(255, 0, 0));
                keyPoints.push_back(p1);
            }
        }
    }

    return keyPoints;
}

struct ResolveOptions {
    bool verbose;
    bool debug;
};

struct ResolveResult {
    cv::Mat result;
    cv::Point center;
};


/// @brief Resolve optimal mask for product image
/// @param src image
/// @return See `ResolveResult`
ResolveResult resolveMask(const cv::Mat& src, ResolveOptions options) {
    cv::Mat imgGamma = adjustGamma(src, 0.9);
    cv::Mat img3 = adjustGamma(src, 0.01);

    cv::Mat debugCanvas = cv::Mat::zeros(imgGamma.size(), CV_8UC3);
    cv::Mat debugCanvasSanity = cv::Mat::zeros(imgGamma.size(), CV_8UC3);

    MorphResult morph = applyMorphology(imgGamma);
    cv::Mat morphDirtyFixed = fixDirtyContours(morph.morphDirty, morph.debugDirty);
    cv::Mat morphDirtyCombined = morph.morphDirty | morphDirtyFixed;
    cv::threshold(morphDirtyCombined, morphDirtyCombined, 10, 255, cv::THRESH_BINARY);

    cv::Mat morphCleanFixed = fixDirtyContours(morph.morphClean, morph.debugClean);
    cv::Mat morphCleanCombined = morph.morphClean | morphCleanFixed;
    cv::threshold(morphCleanCombined, morphCleanCombined, 10, 255, cv::THRESH_BINARY);

    cv::Mat cannyClean, cannyDirty;
    cv::Canny(morphCleanCombined, cannyClean, 0, 255);
    cv::Canny(morphDirtyCombined, cannyDirty, 0, 255);
    cv::Mat finalCanny = cannyDirty.clone(); // Default final

    std::vector<std::vector<cv::Point>> sanityContours;
    std::vector<cv::Vec4i> sanityHierarchy;

    cv::findContours(morphCleanCombined, sanityContours, sanityHierarchy, cv::RETR_CCOMP, cv::CHAIN_APPROX_SIMPLE);

    if (options.debug) {
        cv::imwrite("output/morph-dirty-combined.png", morphDirtyCombined);
        cv::imwrite("output/morph-clean-combined.png", morphCleanCombined);
        cv::imwrite("output/morph-clean-fix.png", morphCleanFixed);
        cv::imwrite("output/morph-dirty-fix.png", morphDirtyFixed);
        cv::imwrite("output/morph-dirty-debug.png", morph.debugDirty);
        cv::imwrite("output/morph-clean-debug.png", morph.debugClean);
    }


    int true_width = morphCleanCombined.cols;
    int true_height = morphCleanCombined.rows;

    std::vector<std::vector<cv::Point>> cleanContours;
    int i = 0;
    for (const auto& cnt : sanityContours) {
        double area = cv::contourArea(cnt);
        cv::Rect box = cv::boundingRect(cnt);
        
        bool invalid = box.height - true_height == 0 || box.width - true_width == 0;

        if (area > CLEAN_CUT_THRESHOLD && !invalid) {
            cleanContours.push_back(cnt);
            cv::drawContours(debugCanvasSanity, std::vector<std::vector<cv::Point>>{cnt}, 0, cv::Scalar(0, 255, 0), 1);
        } else {
            cv::drawContours(debugCanvasSanity, std::vector<std::vector<cv::Point>>{cnt}, 0, cv::Scalar(0, 0, 255), 1);
        }
    }
    

    if (cleanContours.size() == 1) {

        if (options.debug) {
            std::cout << "> auto-resolved contours" << endl;
            cv::imwrite("output/debug-canvas-sanity.png", debugCanvasSanity);
        }

        cv::Rect bbox = cv::boundingRect(cleanContours[0]);
        cv::Point center(bbox.x + bbox.width / 2, bbox.y + bbox.height / 2);


        cv::Mat kernel;
		cv::Mat finalCanny;

        kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(7, 7));
        cv::morphologyEx(cannyClean, finalCanny, cv::MORPH_CLOSE, kernel);

        kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(2, 2));
        cv::morphologyEx(finalCanny, finalCanny, cv::MORPH_CLOSE, kernel);

        kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
        cv::morphologyEx(finalCanny, finalCanny, cv::MORPH_DILATE, kernel);

        return { finalCanny, center };
    }

    if (options.debug) {
        std::cout << "> failed to auto-resolve; switching to hull-based edge-detection" << endl;
        std::cout << "> failed by: " << cleanContours.size() << endl;
    }

	std::vector<cv::Point> simplified = extractKeyContourPoints(cannyClean, debugCanvas);

    if (simplified.empty()) {
        if (options.debug) {
            cv::imwrite("output/debug-canvas-sanity.png", debugCanvasSanity);
            cv::imwrite("output/debug-canvas.png", debugCanvas);
            cv::imwrite("output/canny-clean.png", cannyClean);
            cv::imwrite("output/canny-dirty.png", cannyDirty);
        }
        return { finalCanny, cv::Point(-1, -1) };
    }

    std::vector<std::vector<cv::Point>> simplifiedContours = { simplified };
    cv::Mat hullInput(simplified.size(), 1, CV_32SC2, simplified.data());
    std::vector<cv::Point> hull;
    cv::convexHull(hullInput, hull);

    cv::Rect bbox = cv::boundingRect(hull);
    cv::Point center(bbox.x + bbox.width / 2, bbox.y + bbox.height / 2);

    cv::drawContours(finalCanny, std::vector<std::vector<cv::Point>>{hull}, 0, cv::Scalar(255, 255, 255));
    cv::drawContours(debugCanvas, std::vector<std::vector<cv::Point>>{hull}, 0, cv::Scalar(255, 255, 255));
    cv::circle(debugCanvas, center, 5, cv::Scalar(255, 0, 255));
    cv::rectangle(debugCanvas, bbox, cv::Scalar(255, 0, 255));

    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(6, 6));
    cv::morphologyEx(finalCanny, finalCanny, cv::MORPH_CLOSE, kernel);

    if (options.debug) {
        cv::imwrite("output/debug-canvas-sanity.png", debugCanvasSanity);
        cv::imwrite("output/debug-canvas.png", debugCanvas);
        cv::imwrite("output/canny-clean.png", cannyClean);
        cv::imwrite("output/canny-dirty.png", cannyDirty);
    }

    return { finalCanny, center };
}