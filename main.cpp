#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <iostream>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <stasm.h>
#include <stasm_lib.h>
#include "appmisc.h"

using namespace stasm;
using cv::Mat;
using cv::Rect;
using std::cout;
using std::endl;

static char *data_dir;
static char *path_in;
static char *path_out;
static int rotate;

static int is_image(char *path)
{
    std::string path_str(path);
    std::string allowed_list[] =
        { "jpg", "JPG", "jpeg", "gif", "png", "tiff", "bmp" };
    int idx = path_str.rfind('.');

    if (idx != std::string::npos) {
        std::string extension = path_str.substr(idx + 1);
        int len = sizeof(allowed_list) / sizeof(allowed_list[0]);

        for (int i = 0; i < len; i++) {
            if (extension == allowed_list[i])
                return 1;
        }
    }
    
    return 0;
}

/* assume input is a single channel image */
cv::Rect get_wrap_rect(cv::Mat img) {
	int x_min, y_min, x_max, y_max;
	uint8_t *data = (uint8_t *) img.data;

	x_min = img.cols;
	y_min = img.rows;
	x_max = 0;
	y_max = 0;

	for (int x = 0; x < img.cols; x++) {
		for (int y = 0; y < img.rows; y++) {
			if (data[y * img.cols + x]) {
				if (x < x_min)
					x_min = x;
				if (y < y_min)
					y_min = y;
				if (x > x_max)
					x_max = x;
				if (y > y_max)
					y_max = y;
			}
		}
	}

	Rect rect(x_min, y_min, x_max - x_min, y_max - y_min);
	return rect;
}

void set_alpha_to_zero(cv::Mat& im, cv::Mat& mask)
{
    uint8_t *mask_data = (uint8_t *) mask.data;
    for (int i = 0; i < im.rows; i++) {
        for (int j = 0; j < im.cols; j++) {
            cv::Vec4b& v = im.at<cv::Vec4b>(i, j);
            if (!*mask_data++)
                v[3] = 0;
        }
    }
}

/*
 * Gamma correction algorithm:
 *
 *      1. Convert ROI to grayscale image
 *      2. Calculate mean:
 *              - below 100: set optimalMean 200
 *              - above 200: set optimalMean 127
 *              - otherwise: return
 *      3. Calculate gamma based on optimalMean
 *      4. Calculate LUT for all pixel vals [0, 255]
 *      5. Apply the LUT on image ROI
 *
 *      Reminder:
 *
 *          out = in.^ (1/gamma)
 *          gamma < 1:  output image will be darker
 *          gamma > 1:  output will be brighter
 *
 */
void gamma_correct(cv::Mat& im, cv::Rect& rect)
{
    cv::Mat imROI(im(rect)), imGray;
    cvtColor(imROI, imGray, CV_RGB2GRAY);
    cv::Scalar imMean = cv::mean(imGray);
    unsigned imMeanInt = imMean(0);

    if (imMeanInt > 100 && imMeanInt < 200)
        return;

    unsigned optimalMean = 127;

    if (imMeanInt <= 100)
        optimalMean = 200;

    float imGamma = log2(imMeanInt) / log2(optimalMean);

    //cv::imshow("before", im(rect));

    // Build look-up table
    uchar lut[256];
    for (int i = 0; i < 256; i++)
        lut[i] = cv::saturate_cast<uchar>(pow((float)(i / 255.0), imGamma) * 255.0f);

    if (imMeanInt < 100 || imMeanInt > 200) {
        // cout << "Perform gamma (" << 1 / imGamma << ") correction " << endl;
        for (int i = rect.y; i < rect.y + rect.height; i++) {
            for (int j = rect.x; j < rect.x + rect.width; j++) {
                cv::Vec4b& v = im.at<cv::Vec4b>(i, j);
                v[0] = lut[v[0]];
                v[1] = lut[v[1]];
                v[2] = lut[v[2]];
            }
        }
    }

    //cv::imshow("after", im(rect));
    //cv::waitKey();
}

enum rotate_flags {
    ROTATE_NONE,
    ROTATE_CLOCKWISE,
    ROTATE_COUNTER_CLOCKWISE,
    ROTATE_UPSIDE_DOWN
};

static cv::Mat rotate_image(cv::Mat image, int flag)
{
    cv::Mat image_rot;

    switch (flag) {
    case ROTATE_NONE:
                    image_rot = image;
                    break;
    case ROTATE_CLOCKWISE:
                    image_rot = image.t();
                    flip(image_rot, image_rot, 1 /* flipMode */);
                    break;
    case ROTATE_COUNTER_CLOCKWISE:
                    image_rot = image.t();
                    flip(image_rot, image_rot, 0 /* flipMode */);
                    break;
    case ROTATE_UPSIDE_DOWN:
                    image_rot = image.clone();
                    flip(image_rot, image_rot, 0 /* flipMode */);
                    break;
    default:
            image_rot = image;
            std::cout << "Unsupported rotate option." << std::endl;
    }

    return image_rot;
}

static int get_face_landmarks(cv::Mat_<unsigned char>& imgGray,
                              float *landmarks, int& rotateFlag)
{
    cv::Mat_<unsigned char> imgRot;
    int foundFace;

    do {
        imgRot = rotate_image(imgGray, rotateFlag);
        if (!stasm_search_single(&foundFace,
                                 landmarks,
                                 (const char *) imgRot.data,
                                 imgRot.cols,
                                 imgRot.rows,
                                 path_in,
                                 data_dir ? data_dir : "data")) {
            printf("Error in stasm_search_single: %s\n", stasm_lasterr());
            return -1;
        }

        //std::cout << "Rotate " << rotate_flag << std::endl;

        if (foundFace)
            break;

        rotateFlag++;
    } while (rotate && rotateFlag <= ROTATE_UPSIDE_DOWN);

    if (!foundFace) {
        cout << "No face found in " << path_in << endl;
        return -1;
    }

    return 0;
}

/* Generate a mask and output face */
static int process_landmarks(Mat& imIn, float *landmarks,
                             int rFlag, Mat& imOut)
{
    Mat mask, imTmp;
    Rect boundRect;

    CImage cimg(imIn);
    cimg = rotate_image(cimg, rFlag);
    mask = Mat::zeros(cimg.size(), CV_8UC3);
    CImage cimg_tmp(mask);

    // Draw bounding face shape:
    // border:         white
    // other parts:    black
    Shape shape(LandmarksAsShape(landmarks));
    DrawShape(cimg_tmp, shape, 0xffffff);

    // Create face shape mask:
    // face shape:     white
    // other parts:    black
    cv::cvtColor(cimg_tmp, mask, cv::COLOR_BGR2GRAY);
    cv::floodFill(mask, cv::Point(0, 0),
                  cv::Scalar(255.0, 255.0, 255.0));
    cv::threshold(mask, mask, 254, 255, CV_THRESH_BINARY_INV);

    // Create bounding rectangle for image mask
    boundRect = get_wrap_rect(mask);

    // Apply mask on original image
    cimg.copyTo(imTmp, mask);
    cv::cvtColor(imTmp, imTmp, cv::COLOR_BGR2BGRA, 4);

    // Set alpha channel for all pixels to 0
    set_alpha_to_zero(imTmp, mask);

    // Gamma correct output image
    gamma_correct(imTmp, boundRect);

    // TODO: implement Bilateral Filter

    imOut = imTmp(boundRect);

    return 0;
}

static void show_usage(char *program) {
	cout << "usage: " << program << endl;
	cout << "   [-d data directory]  - optional (default: data)" << endl;
	cout << "   [-i input_file]      - mandatory" << endl;
	cout << "   [-o output_file]     - optional" << endl;
	cout << "   [-r ] - optional (assume that input image might be rotated)" << endl;
}

static int parseOpts(int argc, char **argv)
{
    const char *const opts= "ri:o:d:";
    int op;

    if (argc < 3 || argc > 8) {
        cout << "Invalid args" << endl;
        show_usage(argv[0]);
        return -1;
    }

    while ((op = getopt(argc, argv, opts)) != -1) {
        switch (op) {
            case 'i':
                path_in = optarg;
                break;
            case 'o':
                path_out = optarg;
                break;
            case 'd':
                data_dir = optarg;
                break;
            case 'r':
                rotate = 1;
                break;
            default:
                show_usage(argv[0]);
                return -1;
        }
    }

    if (!path_in || !is_image(path_in)) {
        cout << "No input image specified" << endl;
        return -1;
    }

    return 0;
}

int main(int argc, char **argv)
{
    int ret;

    ret = parseOpts(argc, argv);
    if (ret < 0)
        exit(0);

    cv::Mat imIn(cv::imread(path_in));
    cv::Mat_<unsigned char> imGray;
    cvtColor(imIn, imGray, CV_RGB2GRAY);

    if (!imGray.data) {
        cout << "Cannot load " << path_in << endl;
        exit(1);
    }

    int rFlag = ROTATE_NONE;
    float landmarks[2 * stasm_NLANDMARKS]; // x,y coords (note the 2)

    ret = get_face_landmarks(imGray, landmarks, rFlag);
    if (ret < 0)
        exit(0);

    Mat imOut;
    process_landmarks(imIn, landmarks, rFlag, imOut);

    if (path_out) {
        cv::imwrite(path_out, imOut);
    } else {
        cv::imshow("mask preview", imOut);
        cv::waitKey();
    }

    return 0;
}
