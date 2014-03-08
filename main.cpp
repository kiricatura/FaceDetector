#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <iostream>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <stasm.h>
#include <stasm_lib.h>
#include "appmisc.h"

using namespace stasm;

static void show_usage(char *program) {
	std::cout << "usage: " << program << std::endl;
	std::cout << "   [-i input_file]  - mandatory" << std::endl;
	std::cout << "   [-o output_file] - optional" << std::endl;
}

int main(int argc, char **argv)
{
    char *path_in = NULL;
    char *path_out = NULL;
	const char *const opts= "i:o:";
	int op;

    if (argc != 3 && argc != 5) {
        std::cout << "Invalid args" << std::endl;
        show_usage(argv[0]);
        exit(0);
    }

	while ((op = getopt(argc, argv, opts)) != -1) {
		switch (op) {
		case 'i':
			path_in = optarg;
			break;
		case 'o':
			path_out = optarg;
			break;
		default:
			show_usage(argv[0]);
			exit(0);
		}
	}

    if (!path_in) {
        std::cout << "No input image specified" << std::endl;
        exit(0);
    }

    //cv::Mat_<unsigned char> img(cv::imread(path, CV_LOAD_IMAGE_GRAYSCALE));
    cv::Mat img(cv::imread(path_in, 0));
    CImage cimg(cv::imread(path_in));

    if (!img.data) {
        printf("Cannot load %s\n", path_in);
        exit(1);
    }

    int foundface;
    float landmarks[2 * stasm_NLANDMARKS]; // x,y coords (note the 2)

    if (!stasm_search_single(&foundface, landmarks,
                             (const char*)img.data, img.cols, img.rows, path_in, "data")) {
        printf("Error in stasm_search_single: %s\n", stasm_lasterr());
        exit(1);
    }

    cv::Mat img_mask;

    if (!foundface) {
         printf("No face found in %s\n", path_in);
    } else {
        // draw the landmarks on the image as white dots (image is monochrome)

        Shape shape(LandmarksAsShape(landmarks));
        DrawShape(cimg, shape, 0xffffff);

        cv::cvtColor(cimg, img_mask, cv::COLOR_BGR2GRAY);
        cv::threshold(img_mask, img_mask, 254, 255, CV_THRESH_BINARY);
        cv::floodFill(img_mask, cv::Point(0, 0), cv::Scalar(255.0, 255.0, 255.0));

        cv::medianBlur(img_mask, img_mask, 9);
        cv::threshold(img_mask, img_mask, 254, 255, CV_THRESH_BINARY_INV);

        //stasm_force_points_into_image(landmarks, img.cols, img.rows);
        //for (int i = 0; i < stasm_NLANDMARKS; i++)
        //    img(cvRound(landmarks[i*2+1]), cvRound(landmarks[i*2])) = 255;
    }

    cv::Mat img_out;
    cimg.copyTo(img_out, img_mask);

    if (path_out) {
        //cv::imwrite(path_out, cimg);
        cv::imwrite(path_out, img_out);
    } else {
        //cv::imshow("mask preview", cimg);
        cv::imshow("mask preview", img_out);
        cv::waitKey();
    }

    return 0;
}
