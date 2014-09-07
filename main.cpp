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

static int is_image(char *path)
{
    std::string allowed_list[] = { "jpg", "jpeg", "gif", "png", "tiff", "bmp" };
    std::string path_str(path);
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

	cv::Rect rect(x_min, y_min, x_max - x_min, y_max - y_min);
	return rect;
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

static void show_usage(char *program) {
	std::cout << "usage: " << program << std::endl;
	std::cout << "   [-i input_file]  - mandatory" << std::endl;
	std::cout << "   [-o output_file] - optional" << std::endl;
	std::cout << "   [-r ] - optional (assume that input image might be rotated)" << std::endl;
}

int main(int argc, char **argv)
{
    char *path_in = NULL;
    char *path_out = NULL;
    int rotate = 0;
	const char *const opts= "ri:o:";
	int op;

    if (argc < 3 || argc > 6) {
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
        case 'r':
            rotate = 1;
            break;
		default:
			show_usage(argv[0]);
			exit(0);
		}
	}

    if (!path_in || !is_image(path_in)) {
        std::cout << "No input image specified" << std::endl;
        exit(0);
    }

    cv::Mat img_in(cv::imread(path_in));
    cv::Mat_<unsigned char> img_gray, img_rot;
    CImage cimg(img_in);
    cvtColor(img_in, img_gray, CV_RGB2GRAY);

    if (!img_gray.data) {
        printf("Cannot load %s\n", path_in);
        exit(1);
    }

    int found_face, rotate_flag = ROTATE_NONE;
    float landmarks[2 * stasm_NLANDMARKS]; // x,y coords (note the 2)

    if (rotate) {
        for ( ; rotate_flag <= ROTATE_UPSIDE_DOWN; rotate_flag++) {
            img_rot = rotate_image(img_gray, rotate_flag);
            if (!stasm_search_single(&found_face, landmarks,
                                     (const char*) img_rot.data, img_rot.cols, img_rot.rows, path_in, "data")) {
                printf("Error in stasm_search_single: %s\n", stasm_lasterr());
                exit(1);
            }

            std::cout << "Rotate " << rotate_flag << std::endl;

            if (found_face)
                break;
        }
    } else {
            if (!stasm_search_single(&found_face, landmarks,
                                     (const char*) img_gray.data, img_gray.cols, img_gray.rows, path_in, "data")) {
                printf("Error in stasm_search_single: %s\n", stasm_lasterr());
                exit(1);
            }
    }

    if (!found_face) {
         printf("No face found in %s\n", path_in);
    } else {
        cv::Mat img_mask, img_out;
        cv::Rect bound_rect;

        cimg = rotate_image(cimg, rotate_flag);
        img_mask = cv::Mat::zeros(cimg.size(), CV_8UC3);
        CImage cimg_tmp(img_mask);

        // draw the landmarks on the image as white dots (image is monochrome)
        Shape shape(LandmarksAsShape(landmarks));
        DrawShape(cimg_tmp, shape, 0xffffff);

        cv::cvtColor(cimg_tmp, img_mask, cv::COLOR_BGR2GRAY);
        //cv::threshold(img_mask, img_mask, 254, 255, CV_THRESH_BINARY);
        cv::floodFill(img_mask, cv::Point(0, 0), cv::Scalar(255.0, 255.0, 255.0));

        //cv::medianBlur(img_mask, img_mask, 21);
        cv::threshold(img_mask, img_mask, 254, 255, CV_THRESH_BINARY_INV);

	    //img_mask.convertTo(img_mask, CV_32S);
	    //cv::cvtColor(img_mask, img_mask, cv::COLOR_GRAY2BGR);

        //bound_rect = boundingRect(img_mask);
        bound_rect = get_wrap_rect(img_mask);
        //std::cout << "X: " << bound_rect.x << " Y: " << bound_rect.y << std::endl;
        //std::cout << "WIDTH: " << bound_rect.width << " HEIGHT: " << bound_rect.height << std::endl;
        //stasm_force_points_into_image(landmarks, img.cols, img.rows);
        //for (int i = 0; i < stasm_NLANDMARKS; i++)
        //    img(cvRound(landmarks[i*2+1]), cvRound(landmarks[i*2])) = 255;

        cimg.copyTo(img_out, img_mask);
        cv::cvtColor(img_out, img_out, cv::COLOR_BGR2BGRA, 4);

        uint8_t *mask_data = (uint8_t *) img_mask.data;
	    for (int i = 0; i < img_out.rows; i++) {
            for (int j = 0; j < img_out.cols; j++) {
                cv::Vec4b& v = img_out.at<cv::Vec4b>(i,j);
		        if (!*mask_data++)
                    v[3] = 0;
            }
        }

        if (path_out) {
            //cv::imwrite(path_out, cimg);
            cv::imwrite(path_out, img_out(bound_rect));
            //cv::imwrite(path_out, img_out);
        } else {
            //cv::imshow("mask preview", cimg);
            cv::imshow("mask preview", img_out(bound_rect));
            //cv::imshow("mask preview", img_out);
            //cv::imshow("mask preview", img_mask);
            cv::waitKey();
        }
    }

    return 0;
}
